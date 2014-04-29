/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "debug.h"

#include <string.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <boot/net/NetStack.h>
#include <boot/net/UDP.h>
#include <kernel.h>
#include <util/ring_buffer.h>

#include <algorithm>

#include "keyboard.h"
#include "serial.h"


//#define PRINT_TIME_STAMPS
	// Define to print a TSC timestamp before each line of output.
#define ENABLE_SYSLOG_NG
	// Define to print syslog messages over UDP.


static const char* const kDebugSyslogSignature = "Haiku syslog";

static char sBuffer[16384];
static uint32 sBufferPosition;

static ring_buffer* sDebugSyslogBuffer = NULL;
static bool sPostCleanup = false;


#ifdef PRINT_TIME_STAMPS
extern "C" uint64 rdtsc();
#endif


#ifdef ENABLE_SYSLOG_NG
static UDPSocket *sSyslogSocket = NULL;

static bool in_write = false;

void syslog_write(const char *, size_t);

static void
syslog_ng_write(const char *message, size_t length)
{
	if (in_write == true) return;
	in_write = true;
	if (NetStack::InitCheck() == B_NO_INIT) {
		in_write = false;
		return;
	}

	if (sSyslogSocket == NULL) {
		// Check if the network stack has been initialized yet
		if (NetStack::Default() != NULL) {
			sSyslogSocket = new(std::nothrow) UDPSocket;
			if (sSyslogSocket == NULL) {
				in_write = false;
				return;
			}
			sSyslogSocket->Bind(INADDR_ANY, 60514);
		} else {
			in_write = false;
			return;
		}
	}

	if (sSyslogSocket == NULL) {
		in_write = false;
		return;
	}

	// Strip trailing newlines
	while (length > 0) {
		if (message[length - 1] != '\n'
			&& message[length - 1] != '\r') {
			break;
		}
		length--;
	}
	if (length <= 0) {
		in_write = false;
		return;
	}

	char buffer[1500];
	const int facility = 0; // kernel
	int severity = 7; // debug
	int offset = snprintf(buffer, sizeof(buffer), "<%d>1 - - Haiku - - - \xEF\xBB\xBF",
		facility * 8 + severity);
	length = std::min(length, sizeof(buffer) - offset);
	memcpy(buffer + offset, message, length);

	sSyslogSocket->Send(INADDR_BROADCAST, 514, buffer, offset + length);
	in_write = false;
}
#endif


void
syslog_write(const char* buffer, size_t length)
{
	if (sPostCleanup && sDebugSyslogBuffer != NULL) {
		ring_buffer_write(sDebugSyslogBuffer, (const uint8*)buffer, length);
	} else if (sBufferPosition + length < sizeof(sBuffer)) {
		memcpy(sBuffer + sBufferPosition, buffer, length);
		sBufferPosition += length;
	}
}


static void
dprintf_args(const char *format, va_list args)
{
	char buffer[512];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	if (length == 0)
		return;

	if (length >= (int)sizeof(buffer))
		length = sizeof(buffer) - 1;

#ifdef PRINT_TIME_STAMPS
	static bool sNewLine = true;

	if (sNewLine) {
		char timeBuffer[32];
		snprintf(timeBuffer, sizeof(timeBuffer), "[%" B_PRIu64 "] ", rdtsc());
		syslog_write(timeBuffer, strlen(timeBuffer));
		serial_puts(timeBuffer, strlen(timeBuffer));
	}

	sNewLine = buffer[length - 1] == '\n';
#endif	// PRINT_TIME_STAMPS

	syslog_write(buffer, length);
	serial_puts(buffer, length);
#ifdef ENABLE_SYSLOG_NG
	syslog_ng_write(buffer, length);
#endif

	if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
		fprintf(stderr, "%s", buffer);
}


// #pragma mark -


/*!	This works only after console_init() was called.
*/
void
panic(const char *format, ...)
{
	va_list list;

	platform_switch_to_text_mode();

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	puts("\nPress key to reboot.");

	clear_key_buffer();
	wait_for_key();
	platform_exit();
}


void
dprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);
}


void
kprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);

	// print to console, if available
	if (stdout != NULL) {
		va_list copy;
		va_copy(copy, args);
		vfprintf(stdout, format, copy);
		va_end(copy);
	}

	// always print to serial line
	dprintf_args(format, args);

	va_end(args);
}


// #pragma mark -


void
debug_init_post_mmu(void)
{
/*
	// allocate 1 MB memory at 63 MB
	addr_t base = 63 * 1024 * 1024;
	size_t size = 1024 * 1024;
	if (!mmu_allocate_physical(base, size))
		return;

	void* buffer = (void*)mmu_map_physical_memory(base, size,
		kDefaultPageFlags);
	if (buffer == NULL)
		return;

	// check whether there's a previous syslog we can recover
	size_t signatureLength = strlen(kDebugSyslogSignature);
	bool recover = memcmp(buffer, kDebugSyslogSignature, signatureLength) == 0;

	size -= signatureLength;
	buffer = (uint8*)buffer + ROUNDUP(signatureLength, sizeof(void*));

	sDebugSyslogBuffer = create_ring_buffer_etc(buffer, size,
		recover ? RING_BUFFER_INIT_FROM_BUFFER : 0);

	gKernelArgs.debug_output = sDebugSyslogBuffer;
	gKernelArgs.debug_size = sDebugSyslogBuffer->size;
*/
}


void
debug_cleanup(void)
{
/*
	if (sDebugSyslogBuffer != NULL) {
		size_t signatureLength = strlen(kDebugSyslogSignature);
		void* buffer
			= (void*)ROUNDDOWN((addr_t)sDebugSyslogBuffer, B_PAGE_SIZE);

		if (gKernelArgs.keep_debug_output_buffer) {
			// copy the output gathered so far into the ring buffer
			ring_buffer_clear(sDebugSyslogBuffer);
			ring_buffer_write(sDebugSyslogBuffer, (uint8*)sBuffer,
				sBufferPosition);

			memcpy(buffer, kDebugSyslogSignature, signatureLength);
		} else {
			// clear the signature
			memset(buffer, 0, signatureLength);
		}
	} else
		gKernelArgs.keep_debug_output_buffer = false;

	if (!gKernelArgs.keep_debug_output_buffer) {
		gKernelArgs.debug_output = kernel_args_malloc(sBufferPosition);
		if (gKernelArgs.debug_output != NULL) {
			memcpy(gKernelArgs.debug_output, sBuffer, sBufferPosition);
			gKernelArgs.debug_size = sBufferPosition;
		}
	}

	sPostCleanup = true;
*/
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	if (_size != NULL)
		*_size = sizeof(sBuffer);

	return sBuffer;
}
