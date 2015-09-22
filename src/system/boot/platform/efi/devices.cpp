/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */


#include "efi_platform.h"
#include "efigpt.h"

#include <KernelExport.h>
#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>

#include <string.h>

//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define UUID_LEN	16


static uint8 sDriveIdentifier = 0;
static EFI_GUID sBlockIOGuid = BLOCK_IO_PROTOCOL;
static EFI_GUID sDevicePathGuid = DEVICE_PATH_PROTOCOL;


class EFIBlockDevice : public Node {
	public:
		EFIBlockDevice(EFI_BLOCK_IO *blockIO);
		virtual ~EFIBlockDevice();

		status_t InitCheck() const;

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

		uint32 BlockSize() const { return fBlockSize; }
		uint64 DeviceOffset() const { return fDeviceOffset; }

		disk_identifier &Identifier() { return fIdentifier; }
		uint8 DriveID() const { return fDriveID; }

		const uint8* UUID() const { return fUUID; }

		void SetParameters(uint64 deviceOffset, uint8 *partitionUUID, uint8 *diskUUID);

	protected:
		EFI_BLOCK_IO	*fBlockIO;
		uint8			fDriveID;
		uint64			fSize;
		uint32			fBlockSize;
		uint64			fDeviceOffset;
		uint8			fUUID[UUID_LEN];
		disk_identifier	fIdentifier;
};


static bool sBlockDevicesAdded = false;

static const char* media_device_subtypes[] = {
	"unknown",
	"harddrive",
	"cdrom",
	"vendor",
	"filepath",
};


static void
dump_device_path(EFI_DEVICE_PATH *devicePath)
{
	dprintf("device path: type = %02x, subtype = %02x, node length = %d, is the end = %s\n",
		DevicePathType(devicePath), DevicePathSubType(devicePath),
		DevicePathNodeLength(devicePath), IsDevicePathEnd(devicePath) ? "yes" : "no");

	if (DevicePathType(devicePath) == MESSAGING_DEVICE_PATH
			&& DevicePathSubType(devicePath) == MSG_SATA_DP) {
		SATA_DEVICE_PATH* sata = (SATA_DEVICE_PATH*)devicePath;
		dprintf("connected to SATA bus: hba = %d (%x), multiplier = %d (%x), lun = %d (%x)\n\n",
			sata->HBAPortNumber, sata->HBAPortNumber,
			sata->PortMultiplierPortNumber, sata->PortMultiplierPortNumber,
			sata->LogicalUnitNumber, sata->LogicalUnitNumber);
	}

	if (DevicePathType(devicePath) == MEDIA_DEVICE_PATH) {
		dprintf("media device path: subtype = %s\n",
			media_device_subtypes[DevicePathSubType(devicePath)]);
		switch (DevicePathSubType(devicePath)) {
			case MEDIA_HARDDRIVE_DP:
				{
				HARDDRIVE_DEVICE_PATH* harddrive = (HARDDRIVE_DEVICE_PATH*)devicePath;
				dprintf("\thard drive: partition %d, start = %ld, size = %ld, is gpt = %s\n",
					harddrive->PartitionNumber, harddrive->PartitionStart,
					harddrive->PartitionSize, harddrive->SignatureType == SIGNATURE_TYPE_GUID ? "yes" : "no");
				break;
				}
			default:
				dprintf("\t%s: don't care about this type\n",
					media_device_subtypes[DevicePathSubType(devicePath)]);
				break;
		}
	}
}


typedef struct _sata_device : public DoublyLinkedListLinkImpl<_sata_device> {
	uint16	port;
	uint16	multiplier;
	uint16	lun;
	uint8	uuid[UUID_LEN];
} sata_device;

typedef DoublyLinkedList<sata_device> SataDeviceList;
typedef SataDeviceList::Iterator SataDeviceIterator;


static status_t
add_block_devices(NodeList *devicesList, bool identifierMissing) // should bool be a reference?
{
	if (sBlockDevicesAdded)
		return B_OK;

	EFI_BLOCK_IO *blockIO;
	EFI_DEVICE_PATH *devicePath, *node;
	EFI_HANDLE *handles, handle;
	EFI_STATUS status;
	UINTN size;

	SataDeviceList sataDevices;
	SataDeviceIterator iterator = sataDevices.GetIterator();

	size = 0;
	handles = NULL;

	status = kBootServices->LocateHandle(ByProtocol, &sBlockIOGuid, 0, &size, 0);
	if (status == EFI_BUFFER_TOO_SMALL) {
		handles = (EFI_HANDLE *)malloc(size);
		status = kBootServices->LocateHandle(ByProtocol, &sBlockIOGuid, 0, &size, handles);
		if (status != EFI_SUCCESS)
			free(handles);
	}
	if (status != EFI_SUCCESS)
		return B_ERROR;

	for (unsigned int n = 0; n < size / sizeof(EFI_HANDLE); n++) {
		status = kBootServices->HandleProtocol(handles[n], &sDevicePathGuid, (void **)&devicePath);
		if (status != EFI_SUCCESS)
			continue;

		node = devicePath;

		dprintf("next handle...\n");
		dump_device_path(devicePath);

		sata_device *sataDevice = NULL;

		int i = 1;
		while (!IsDevicePathEnd(NextDevicePathNode(node))) {
			node = NextDevicePathNode(node);
			dprintf("node %d\n", i++);
			if (DevicePathType(node) == MESSAGING_DEVICE_PATH
				&& DevicePathSubType(node) == MSG_SATA_DP) {
				// Add to sataDevices, if it's not there already...
				SATA_DEVICE_PATH *current = (SATA_DEVICE_PATH*)node;

				iterator.Rewind();
				while ((sataDevice = iterator.Next()) != NULL) {
					if (sataDevice->port == current->HBAPortNumber
							&& sataDevice->multiplier == current->PortMultiplierPortNumber
							&& sataDevice->lun == current->LogicalUnitNumber) {
						dprintf("Already contains this sata device path\n");
						break;
					}
				}
				if (sataDevice == NULL) {
					sataDevice = (sata_device*)malloc(sizeof(sata_device));
					sataDevice->port = current->HBAPortNumber;
					sataDevice->multiplier = current->PortMultiplierPortNumber;
					sataDevice->lun = current->LogicalUnitNumber;
					memset(sataDevice->uuid, 0, UUID_LEN);

					sataDevices.Insert(sataDevice);
					dprintf("Added sata device path\n");
				}
			}

			dump_device_path(node);
		}

		status = kBootServices->HandleProtocol(handles[n], &sBlockIOGuid, (void **)&blockIO);
		if (status != EFI_SUCCESS) {
			continue;
		}
		if (blockIO->Media->LogicalPartition == false) {
			if (!blockIO->Media->RemovableMedia && blockIO->Media->MediaPresent) {
				EFI_PARTITION_TABLE_HEADER *header =
					(EFI_PARTITION_TABLE_HEADER*)malloc(blockIO->Media->BlockSize);
				status = blockIO->ReadBlocks(blockIO, blockIO->Media->MediaId, 1,
						blockIO->Media->BlockSize, header);
				if (status == EFI_SUCCESS) {
					dprintf("guid: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
						header->DiskGUID.Data1, header->DiskGUID.Data2, header->DiskGUID.Data3,
						header->DiskGUID.Data4[0], header->DiskGUID.Data4[1],
						header->DiskGUID.Data4[2], header->DiskGUID.Data4[3],
						header->DiskGUID.Data4[4], header->DiskGUID.Data4[5],
						header->DiskGUID.Data4[6], header->DiskGUID.Data4[7]);
					if (sataDevice == NULL) {
						dprintf("humm, this shouldn't happen...\n");
						continue;
					}
					memcpy(sataDevice->uuid, (uint8*)&header->DiskGUID, UUID_LEN);
					dprintf("uuid stored in sata device: ");
					for (int32 i = 0; i < UUID_LEN; ++i)
						dprintf("%02x", sataDevice->uuid[i]);
					dprintf("\n");
				}
			}
			continue;
		}

		/* If we come across a logical partition of subtype CDROM
		 * it doesn't refer to the CD filesystem itself, but rather
		 * to any usable El Torito boot image on it. In this case
		 * we try to find the parent device and add that instead as
		 * that will be the CD fileystem.
		 */
		if (DevicePathType(node) == MEDIA_DEVICE_PATH &&
				DevicePathSubType(node) == MEDIA_CDROM_DP) {
			node->Type = END_DEVICE_PATH_TYPE;
			node->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
			// was testing under QEMU, caused VM panic!
			//status = kBootServices->LocateDevicePath(&sBlockIOGuid, &devicePath, &handle);
			// TODO: actually care about CD-ROMs
			continue;
		}

		if (DevicePathType(node) != MEDIA_DEVICE_PATH ||
				DevicePathSubType(node) != MEDIA_HARDDRIVE_DP)
			continue;

		// This is a partition we want to add :-)
		HARDDRIVE_DEVICE_PATH* harddrive = (HARDDRIVE_DEVICE_PATH*)node;
		EFIBlockDevice *device = new(std::nothrow) EFIBlockDevice(blockIO);

		if (device->InitCheck() != B_OK) {
			delete device;
			continue;
		}

		if (sataDevice == NULL) {
			dprintf("sata device is NULL?\n");
			device->SetParameters(harddrive->PartitionSize, harddrive->Signature, harddrive->Signature);
		} else
			device->SetParameters(harddrive->PartitionSize, harddrive->Signature, sataDevice->uuid);

		devicesList->Add(device);
	}

	iterator.Rewind();
	sata_device *p;
	while ((p = iterator.Next()) != NULL)
		free(p);

	sBlockDevicesAdded = true;
	return B_OK;
}


//	#pragma mark -


EFIBlockDevice::EFIBlockDevice(EFI_BLOCK_IO *blockIO)
	:
	fBlockIO(blockIO),
	fDriveID(++sDriveIdentifier),
	fSize(0)
{
	TRACE(("drive ID %u\n", fDriveID));

	if (fBlockIO->Media->MediaPresent == false)
		return;

	fBlockSize = fBlockIO->Media->BlockSize;
	fSize = (fBlockIO->Media->LastBlock + 1) * fBlockSize;
}


EFIBlockDevice::~EFIBlockDevice()
{
}


status_t
EFIBlockDevice::InitCheck() const
{
	return fSize > 0 ? B_OK : B_ERROR;
}


ssize_t
EFIBlockDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	uint32 offset = pos % fBlockSize;
	pos /= fBlockSize;

	uint32 numBlocks = (offset + bufferSize + fBlockSize) / fBlockSize;
	char readBuffer[numBlocks * fBlockSize];

	EFI_STATUS status = fBlockIO->ReadBlocks(fBlockIO, fBlockIO->Media->MediaId,
		pos, sizeof(readBuffer), readBuffer);

	if (status != EFI_SUCCESS)
		return B_ERROR;

	memcpy(buffer, readBuffer + offset, bufferSize);

	return bufferSize;
}


ssize_t
EFIBlockDevice::WriteAt(void* cookie, off_t pos, const void* buffer,
	size_t bufferSize)
{
	return B_UNSUPPORTED;
}


off_t
EFIBlockDevice::Size() const
{
	return fSize;
}


void
EFIBlockDevice::SetParameters(uint64 deviceOffset, uint8 *partitionUUID, uint8 *diskUUID)
{
	fDeviceOffset = deviceOffset;
	memcpy(fUUID, partitionUUID, UUID_LEN);
	// Could probably actually retrieve these with the device path API
	fIdentifier.bus_type = UNKNOWN_BUS;
	fIdentifier.device_type = UNKNOWN_DEVICE;
	fIdentifier.device.unknown.size = Size();

	// This should be the UUID of the parent disk system
	memcpy(fIdentifier.device.unknown.uuid, diskUUID, UUID_LEN);
	fIdentifier.device.unknown.use_uuid = true;
}


//	#pragma mark -


status_t
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	add_block_devices(devicesList, false);
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
	NodeList *list, boot::Partition **_partition)
{
	add_block_devices(list, false);
	return B_ENTRY_NOT_FOUND;
}


status_t
platform_add_block_devices(stage2_args *args, NodeList *devicesList)
{
	dprintf("platform_add_block_devices\n");
	return add_block_devices(devicesList, false);
}


status_t
platform_register_boot_device(Node *device)
{
	EFIBlockDevice *drive = (EFIBlockDevice *)device;

	dprintf("register_boot_device: partition uuid = ");
	for (int i = 0; i < UUID_LEN; ++i) {
		dprintf("%02x", drive->UUID()[i]);
	}
	dprintf("\n");

	if (gBootVolume.SetData(BOOT_VOLUME_PARTITION_UUID, B_RAW_TYPE,
		drive->UUID(), UUID_LEN) != B_OK) {
		dprintf("register_boot_device: SetData(BOOT_VOLUME_PARTITION_UUID) failed!\n");
	}
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&drive->Identifier(), sizeof(disk_identifier));

	return B_OK;
}

