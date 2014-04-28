/*
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "efi_platform.h"
#include "efinet.h"

#include <boot/net/NetStack.h>
#include <boot/net/Ethernet.h>

#include <KernelExport.h>

#include <time.h>


#define MSG(format, args...) \
	dprintf("%s:%d: " format "\n", __FILE__, __LINE__ , ## args)


static EFI_GUID sSimpleNetworkGuid = EFI_SIMPLE_NETWORK_PROTOCOL;


class EFIEthernetInterface : public EthernetInterface {
public:
	EFIEthernetInterface();
	virtual ~EFIEthernetInterface();

	status_t Init();

	virtual mac_addr_t MACAddress() const;

	virtual	void *AllocateSendReceiveBuffer(size_t size);
	virtual	void FreeSendReceiveBuffer(void *buffer);

	virtual ssize_t Send(const void *buffer, size_t size);
	virtual ssize_t Receive(void *buffer, size_t size);

private:
	EFI_SIMPLE_NETWORK	*fNetwork;
	mac_addr_t	fMACAddress;
};


EFIEthernetInterface::EFIEthernetInterface()
	:
	EthernetInterface(),
	fNetwork(NULL),
	fMACAddress(kNoMACAddress)
{
}


EFIEthernetInterface::~EFIEthernetInterface()
{
	if (fNetwork != NULL) {
		fNetwork->Shutdown(fNetwork);
		fNetwork->Stop(fNetwork);
		fNetwork = NULL;
	}
}


status_t
EFIEthernetInterface::Init()
{
	EFI_STATUS status = kBootServices->LocateProtocol(&sSimpleNetworkGuid,
		NULL, (void **)&fNetwork);

	if (status != EFI_SUCCESS || fNetwork == NULL) {
		MSG("locate protocol failed: %x (%p)", status, fNetwork);
		return B_ERROR;
	}

	status = fNetwork->Start(fNetwork);
	if (status != EFI_SUCCESS && status != EFI_ALREADY_STARTED) {
		MSG("failed to start simple network protocol: %x", status);
		return B_ERROR;
	}
	status = fNetwork->Initialize(fNetwork, 0x1000, 0x1000);
	if (status != EFI_SUCCESS) {
		MSG("failed to initialize simple network protocol: %x", status);
		return B_ERROR;
	}

	// get MAC address
	EFI_MAC_ADDRESS macAddress = fNetwork->Mode->CurrentAddress;
	//ASSERT(fNetwork->Mode->HwAddressSize == ETH_ALEN);
	#define AT(i) fMACAddress[i]
	fMACAddress = macAddress.Addr;
	MSG("mac address: %2x:%2x:%2x:%2x:%2x:%2x", AT(0), AT(1), AT(2), AT(3), AT(4), AT(5));
	#undef AT
	return B_OK;
}


mac_addr_t
EFIEthernetInterface::MACAddress() const
{
	return fMACAddress;
}


void *
EFIEthernetInterface::AllocateSendReceiveBuffer(size_t size)
{
	return malloc(size);
}


void
EFIEthernetInterface::FreeSendReceiveBuffer(void *buffer)
{
	if (buffer != NULL)
		free(buffer);
}


ssize_t
EFIEthernetInterface::Send(const void *buffer, size_t size)
{
	EFI_STATUS status = fNetwork->Transmit(fNetwork, 0, size, (void *)buffer, NULL, NULL, NULL);

	if (status == EFI_SUCCESS) {
		// Need to wait for packet to be transmitted so we can recyle the transmit buffer
		void *txBuffer;
		do {
			if (fNetwork->GetStatus(fNetwork, NULL, &txBuffer) != EFI_SUCCESS)
				return B_ERROR;

			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 50 * 1000;
			nanosleep(&ts, NULL);
		} while (txBuffer != buffer);
	}

	return size;
}


ssize_t
EFIEthernetInterface::Receive(void *buffer, size_t size)
{
	EFI_STATUS status = fNetwork->Receive(fNetwork, NULL, &size, buffer, NULL, NULL, NULL);

	if (status == EFI_NOT_READY)
		return 0;

	if (status != EFI_SUCCESS)
		return B_ERROR;

	return size;
}

status_t
platform_net_stack_init()
{
	MSG("enter");
	if (NetStack::Default() == NULL) {
		MSG("no memory for default netstack");
		return B_NO_MEMORY;
	}

	EFIEthernetInterface *interface = new(std::nothrow) EFIEthernetInterface;
	if (!interface) {
		MSG("no memory for efi driver");
		return B_NO_MEMORY;
	}

	status_t error = interface->Init();
	if (error != B_OK) {
		MSG("init efi driver failed");
		delete interface;
		return error;
	}

	error = NetStack::Default()->AddEthernetInterface(interface);
	if (error != B_OK) {
		MSG("add efi driver to netstack failed");
		delete interface;
		return error;
	}

	MSG("exit");
	return B_OK;
}
