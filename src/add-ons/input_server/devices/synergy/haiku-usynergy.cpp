
#include <Application.h>
#include <Autolock.h>
#include <Clipboard.h>
#include <Notification.h>
#include <Screen.h>
#include <OS.h>
#include <cstdlib>
#include <strings.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <keyboard_mouse_driver.h>

#include "ATKeymap.h"


#include "haiku-usynergy.h"

#define TRACE_SYNERGY_DEVICE
#ifdef TRACE_SYNERGY_DEVICE

#	define CALLED(x...) \
		debug_printf("%s:%s\n", "SynergyDevice", __FUNCTION__)
#	define TRACE(x...) \
		do { debug_printf(x); } while (0)
#	define LOG_EVENT(text...) debug_printf(text)
#	define LOG_ERR(text...) TRACE(text)
#else
#	define TRACE(x...) do {} while (0)
#	define CALLED(x...) TRACE(x)
#	define LOG_ERR(text...) debug_printf(text)
#	define LOG_EVENT(text...) TRACE(x)
#endif


const static uint32 kSynergyThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;

uSynergyInputServerDevice::uSynergyInputServerDevice()
	:
	BHandler("uSynergy Handler"),
	fUpdateSettings(false),
	fKeymapLock("synergy keymap lock"),
	fClipboard(new BClipboard("system"))
{
	CALLED();
	uSynergyHaikuContext = (uSynergyContext*)malloc(sizeof(uSynergyContext));
	uSynergyInit(uSynergyHaikuContext);

	uSynergyHaikuContext->m_connectFunc		= uSynergyConnectHaiku;
	uSynergyHaikuContext->m_receiveFunc		= uSynergyReceiveHaiku;
	uSynergyHaikuContext->m_sendFunc		= uSynergySendHaiku;
	uSynergyHaikuContext->m_getTimeFunc		= uSynergyGetTimeHaiku;
	uSynergyHaikuContext->m_screenActiveCallback	= uSynergyScreenActiveCallbackHaiku;
	uSynergyHaikuContext->m_mouseCallback		= uSynergyMouseCallbackHaiku;
	uSynergyHaikuContext->m_keyboardCallback	= uSynergyKeyboardCallbackHaiku;
	uSynergyHaikuContext->m_sleepFunc		= uSynergySleepHaiku;
	uSynergyHaikuContext->m_traceFunc		= uSynergyTraceHaiku;
	uSynergyHaikuContext->m_joystickCallback	= uSynergyJoystickCallbackHaiku;
	uSynergyHaikuContext->m_clipboardCallback	= uSynergyClipboardCallbackHaiku;
	uSynergyHaikuContext->m_clientName		= "haiku";
	uSynergyHaikuContext->m_cookie			= (uSynergyCookie)this;

	BRect screenRect = BScreen().Frame();
	uSynergyHaikuContext->m_clientWidth		= (uint16_t)screenRect.Width() + 1;
	uSynergyHaikuContext->m_clientHeight		= (uint16_t)screenRect.Height() + 1;

	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}

uSynergyInputServerDevice::~uSynergyInputServerDevice()
{
	free(uSynergyHaikuContext);
}

status_t
uSynergyInputServerDevice::InitCheck()
{
	CALLED();
	input_device_ref *devices[3];

	input_device_ref mouse = { "uSynergy Mouse", B_POINTING_DEVICE, (void *)this };
	input_device_ref keyboard = { "uSynergy Keyboard", B_KEYBOARD_DEVICE, (void *)this };

	devices[0] = &mouse;
	devices[1] = &keyboard;
	devices[2] = NULL;

	RegisterDevices(devices);

	return B_OK;
}

void
uSynergyInputServerDevice::MessageReceived(BMessage* message)
{
	if (message->what != B_CLIPBOARD_CHANGED) {
		BHandler::MessageReceived(message);
		return;
	}

	const char *text = NULL;
	int32 len = 0;
	BMessage *clip = NULL;
	if (fClipboard->Lock()) {
		if ((clip = fClipboard->Data()) == B_OK) {
			clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &len);
		}
		fClipboard->Unlock();
	}
	if (len > 0 && text != NULL) {
		uSynergySendClipboard(this->uSynergyHaikuContext, text);
	}
}

status_t
uSynergyInputServerDevice::Start(const char* name, void* cookie)
{
	CALLED();
	status_t status = B_OK;
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "uSynergy haiku");

	thread_info info;
	if ((get_thread_info(uSynergyThread, &info) == B_OK) || threadActive)
		goto clean;

	uSynergyThread = spawn_thread(uSynergyThreadLoop, threadName, kSynergyThreadPriority, (void*)this->uSynergyHaikuContext);

	if (uSynergyThread < 0)
		status = uSynergyThread;
	else {
		threadActive = true;
		fClipboard->StartWatching(this);
		status = resume_thread(uSynergyThread);
	}
clean:
	return status;
}

status_t
uSynergyInputServerDevice::Stop(const char* name, void* cookie)
{
	CALLED();
	threadActive = false;
	// this will stop the thread as soon as it reads the next packet
	fClipboard->StopWatching(this);

	if (uSynergyThread >= 0) {
		// unblock the thread, which might wait on a semaphore.
		suspend_thread(uSynergyThread);
		resume_thread(uSynergyThread);
		status_t dummy;
		wait_for_thread(uSynergyThread, &dummy);
	}

	return B_OK;
}

status_t
uSynergyInputServerDevice::Control(const char* name, void* cookie, uint32 command, BMessage* message)
{
	if (command == B_KEY_MAP_CHANGED)
		fUpdateSettings = true;

	return B_OK;
}

BMessage*
uSynergyInputServerDevice::_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons, float x, float y) const
{
	BMessage* message = new BMessage(what);
	if (message == NULL)
		return NULL;

	if (message->AddInt64("when", when) < B_OK
	    || message->AddInt32("buttons", buttons) < B_OK
	    || message->AddFloat("x", x) < B_OK
	    || message->AddFloat("y", y) < B_OK) {
		delete message;
		return NULL;
	}

	return message;
}

void
uSynergyInputServerDevice::_UpdateSettings()
{
	BAutolock lock(fKeymapLock);
	fKeymap.RetrieveCurrent();
	fModifiers = fKeymap.Map().lock_settings;
	fControlKey = fKeymap.KeyForModifier(B_LEFT_CONTROL_KEY);
	fCommandKey = fKeymap.KeyForModifier(B_LEFT_COMMAND_KEY);
}

status_t
uSynergyInputServerDevice::uSynergyThreadLoop(void* arg)
{
	uSynergyContext *uSynergyHaikuContext = (uSynergyContext*)arg;
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)uSynergyHaikuContext->m_cookie;

	while (inputDevice->threadActive) {
		uSynergyUpdate(uSynergyHaikuContext);

		if (inputDevice->fUpdateSettings) {
			inputDevice->_UpdateSettings();
			inputDevice->fUpdateSettings = false;
		}
	}

	return B_OK;
}


uSynergyBool
uSynergyConnectHaiku(uSynergyCookie cookie)
{
	CALLED();
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)cookie;

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(24800);
	inet_aton("10.20.30.18", &server.sin_addr);

	inputDevice->synergyServerSocket = socket(PF_INET, SOCK_STREAM, 0);

	if (inputDevice->synergyServerSocket < 0) {
		TRACE("synergy: socket couldn't be created\n");
		snooze(1000000);
		return USYNERGY_FALSE;
	}

	if (connect(inputDevice->synergyServerSocket, (struct sockaddr*)&server, sizeof(struct sockaddr)) < 0 ) {
		TRACE("synergy: %s: %d\n", "failed to connect to remote host", errno);
		close(inputDevice->synergyServerSocket);
		snooze(1000000); // temporary workaround to avoid filling up log too quickly
		return USYNERGY_FALSE;
	}
	else {
		TRACE("synergy: connected to remote host!!!\n");
		return USYNERGY_TRUE;
	}
}

uSynergyBool
uSynergySendHaiku(uSynergyCookie cookie, const uint8_t *buffer, int length)
{
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)cookie;

	if (send(inputDevice->synergyServerSocket, buffer, length, 0) != length)
		return USYNERGY_FALSE;
	return USYNERGY_TRUE;
}

uSynergyBool
uSynergyReceiveHaiku(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength)
{
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)cookie;

	if ((*outLength = recv(inputDevice->synergyServerSocket, buffer, maxLength, 0)) == -1)
		return USYNERGY_FALSE;
	return USYNERGY_TRUE;
}

void
uSynergySleepHaiku(uSynergyCookie cookie, int timeMs)
{
	snooze(timeMs * 1000);
}

uint32_t
uSynergyGetTimeHaiku()
{
	return system_time() / 1000; // return milliseconds, not microseconds
}

void
uSynergyTraceHaiku(uSynergyCookie cookie, const char *text)
{
	BNotification *notify = new BNotification(B_INFORMATION_NOTIFICATION);
	BString group("uSynergy");
	BString content(text);

	notify->SetGroup(group);
	notify->SetContent(content);
	notify->Send(2000000);
}

void
uSynergyScreenActiveCallbackHaiku(uSynergyCookie cookie, uSynergyBool active)
{

}

void
uSynergyMouseCallbackHaiku(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle)
{
	uSynergyInputServerDevice	*inputDevice = (uSynergyInputServerDevice*)cookie;
	static uint32_t			 oldButtons = 0, oldPressedButtons = 0;
	uint32_t			 buttons = 0;
	static uint16_t			 oldX = 0, oldY = 0, clicks = 0;
	static int16_t			oldWheelX = 0, oldWheelY = 0;
	static uint64			oldWhen = system_time();
	float				 xVal = (float)x / (float)inputDevice->uSynergyHaikuContext->m_clientWidth;
	float				 yVal = (float)y / (float)inputDevice->uSynergyHaikuContext->m_clientHeight;

	int64 timestamp = system_time();

	if (buttonLeft == USYNERGY_TRUE) {
		buttons |= 1 << 0;
	}
	if (buttonRight == USYNERGY_TRUE) {
		buttons |= 1 << 1;
	}
	if (buttonMiddle == USYNERGY_TRUE) {
		buttons |= 1 << 2;
	}

	if (buttons != oldButtons) {
		bool pressedButton = buttons > 0;
		BMessage* message = inputDevice->_BuildMouseMessage(pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP, timestamp, buttons, xVal, yVal);
		if (pressedButton) {
			if ((buttons == oldPressedButtons) && ((timestamp - oldWhen) < 500000))
				clicks++;
			else
				clicks = 1;
			message->AddInt32("clicks", clicks);
			oldWhen = timestamp;
			oldPressedButtons = buttons;
		}
		else
			clicks = 1;

		if (message != NULL)
			inputDevice->EnqueueMessage(message);

		oldButtons = buttons;
	}

	if ((x != oldX) || (y != oldY)) {
		BMessage* message = inputDevice->_BuildMouseMessage(B_MOUSE_MOVED, timestamp, buttons, xVal, yVal);
		if (message != NULL)
			inputDevice->EnqueueMessage(message);
		oldX = x;
		oldY = y;
	}

	if (wheelX != 0 || wheelY != 0) {
		BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
		if (message != NULL) {
			if (message->AddInt64("when", timestamp) == B_OK
			    && message->AddFloat("be:wheel_delta_x", (oldWheelX - wheelX) / 120) == B_OK
			    && message->AddFloat("be:wheel_delta_y", (oldWheelY - wheelY) / 120) == B_OK)
				inputDevice->EnqueueMessage(message);
			else
				delete message;
		}
		oldWheelX = wheelX;
		oldWheelY = wheelY;
	}
}

/* Synergy modifier definitions */
#define	SYNERGY_SHIFT		0x0001
#define	SYNERGY_CONTROL		0x0002
#define SYNERGY_ALT		0x0004
#define SYNERGY_META		0x0008
#define SYNERGY_SUPER		0x0010
#define SYNERGY_ALTGR		0x0020
#define SYNERGY_LEVEL5LOCK	0x0040
#define SYNERGY_CAPSLOCK	0x1000
#define SYNERGY_NUMLOCK		0x2000
#define SYNERGY_SCROLLLOCK	0x4000
#define EXTENDED_KEY		0xe000

void
uSynergyInputServerDevice::_ProcessKeyboard(uint16_t scancode, uint16_t _modifiers, bool isKeyDown, bool isKeyRepeat)
{
	static uint32 lastScanCode = 0;
	static uint32 repeatCount = 1;
	static uint8 states[16];

	bool isExtended = false;

	if (scancode & EXTENDED_KEY != 0) {
		isExtended = true;
		TRACE("synergy: extended key\n");
	}

	int64 timestamp = system_time();

	//TRACE("synergy: scancode = 0x%02x\n", scancode);
	uint32_t keycode = 0;
	if (scancode > 0 && scancode < sizeof(kATKeycodeMap)/sizeof(uint32))
		keycode = kATKeycodeMap[scancode - 1];
	//TRACE("synergy: keycode = 0x%x\n", keycode);

	if (keycode < 256) {
		if (isKeyDown)
			states[(keycode) >> 3] |= (1 << (7 - (keycode & 0x7)));
		else
			states[(keycode) >> 3] &= (!(1 << (7 - (keycode & 0x7))));
	}

	if (isKeyDown && keycode == 0x34 // DELETE KEY
		&& (states[fCommandKey >> 3] & (1 << (7 - (fCommandKey & 0x7))))
		&& (states[fControlKey >> 3] & (1 << (7 - (fControlKey & 0x7))))) {
		TRACE("synergy: TeamMonitor called\n");
	}

	uint32 modifiers = 0;
	TRACE("synergy: modifiers: ");
	if (_modifiers & SYNERGY_SHIFT) {
		TRACE("SHIFT ");
		modifiers |= B_SHIFT_KEY;
	}
	if (_modifiers & SYNERGY_CONTROL) {
		TRACE("CONTROL ");
		modifiers |= B_CONTROL_KEY;
	}
	if (_modifiers & SYNERGY_ALT) {
		TRACE("COMMAND(ALT) ");
		modifiers |= B_COMMAND_KEY;
	}
	if (_modifiers & SYNERGY_META) {
		TRACE("MENU(META) ");
		modifiers |= B_MENU_KEY;
	}
	if (_modifiers & SYNERGY_SUPER) {
		TRACE("OPTION(SUPER) ");
		modifiers |= B_OPTION_KEY;
	}
	if (_modifiers & SYNERGY_ALTGR) {
		TRACE("RIGHT_OPTION(ALTGR) ");
		modifiers |= B_RIGHT_OPTION_KEY;
	}
	if (_modifiers & SYNERGY_CAPSLOCK)
		modifiers |= B_CAPS_LOCK;
	if (_modifiers & SYNERGY_NUMLOCK)
		modifiers |= B_NUM_LOCK;
	if (_modifiers & SYNERGY_SCROLLLOCK)
		modifiers |= B_SCROLL_LOCK;
	TRACE("\n");

	bool isLock
		= (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) != 0;
	if (!isKeyRepeat) {
		uint32 oldModifiers = fModifiers;

		if ((isKeyDown && !isLock)
			|| (isKeyDown && !(fModifiers & modifiers)))
			fModifiers |= modifiers;
		else {
			fModifiers &= ~modifiers;

			// ensure that we don't clear a combined B_*_KEY when still
			// one of the individual B_{LEFT|RIGHT}_*_KEY is pressed
			if (fModifiers & (B_LEFT_SHIFT_KEY | B_RIGHT_SHIFT_KEY))
				fModifiers |= B_SHIFT_KEY;
			if (fModifiers & (B_LEFT_COMMAND_KEY | B_RIGHT_COMMAND_KEY))
				fModifiers |= B_COMMAND_KEY;
			if (fModifiers & (B_LEFT_CONTROL_KEY | B_RIGHT_CONTROL_KEY))
				fModifiers |= B_CONTROL_KEY;
			if (fModifiers & (B_LEFT_OPTION_KEY | B_RIGHT_OPTION_KEY))
				fModifiers |= B_OPTION_KEY;
		}

		if (fModifiers != oldModifiers) {
			BMessage* message = new BMessage(B_MODIFIERS_CHANGED);
			if (message == NULL)
				return;

			TRACE("synergy: modifiers changed: 0x%04lx => 0x%04lx\n", oldModifiers, fModifiers);

			message->AddInt64("when", timestamp);
			message->AddInt32("be:old_modifiers", oldModifiers);
			message->AddInt32("modifiers", fModifiers);
			message->AddData("states", B_UINT8_TYPE, states, 16);

			if (EnqueueMessage(message) != B_OK)
				delete message;
		}
	}

	if (scancode == 0 || scancode == EXTENDED_KEY) {
		TRACE("empty scancode\n");
		return;
	}

	BMessage* msg = new BMessage;
	if (msg == NULL)
		return;

	char* string = NULL;
	char* rawString = NULL;
	int32 numBytes = 0, rawNumBytes = 0;
	fKeymap.GetChars(keycode, fModifiers, 0, &string, &numBytes);
	fKeymap.GetChars(keycode, 0, 0, &rawString, &rawNumBytes);

	if (numBytes > 0)
		msg->what = isKeyDown ? B_KEY_DOWN : B_KEY_UP;
	else
		msg->what = isKeyDown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;

	msg->AddInt64("when", timestamp);
	msg->AddInt32("key", keycode);
	msg->AddInt32("modifiers", fModifiers);
	msg->AddData("states", B_UINT8_TYPE, states, 16);
	if (numBytes > 0) {
		for (int i = 0; i < numBytes; i++) {
			TRACE("%02x:", (int8)string[i]);
			msg->AddInt8("byte", (int8)string[i]);
		}
		TRACE("\n");
		msg->AddData("bytes", B_STRING_TYPE, string, numBytes + 1);

		if (rawNumBytes <= 0) {
			rawNumBytes = 1;
			delete[] rawString;
			rawString = string;
		} else
			delete[] string;

		if (isKeyDown && isKeyRepeat) {
			repeatCount++;
			msg->AddInt32("be:key_repeat", repeatCount);
		} else
			repeatCount = 1;
	} else
		delete[] string;

	if (rawNumBytes > 0)
		msg->AddInt32("raw_char", (uint32)((uint8)rawString[0] & 0x7f));

	delete[] rawString;

	if (msg != NULL && EnqueueMessage(msg) != B_OK)
		delete msg;

	lastScanCode = isKeyDown ? scancode : 0;
}

void
uSynergyKeyboardCallbackHaiku(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat)
{
	((uSynergyInputServerDevice*)cookie)->_ProcessKeyboard(key, modifiers, down, repeat);
}

void
uSynergyJoystickCallbackHaiku(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY)
{

}

void
uSynergyInputServerDevice::_PostClipboard(const BString &mimetype, const uint8_t *data, uint32_t size)
{
	if (fClipboard->Lock()) {
		fClipboard->Clear();
		BMessage *clip = fClipboard->Data();
		clip->AddData(mimetype, B_MIME_TYPE, data, size);
		status_t result = fClipboard->Commit();
		if (result != B_OK) {
			TRACE("synergy: failed to commit data to clipboard\n");
		}

		fClipboard->Unlock();
	} else {
		TRACE("syenrgy: could not lock clipboard\n");
	}
}

void
uSynergyClipboardCallbackHaiku(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size)
{
	if (format == USYNERGY_CLIPBOARD_FORMAT_TEXT)
		((uSynergyInputServerDevice*)cookie)->_PostClipboard("text/plain", data, size);
}


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new (std::nothrow)uSynergyInputServerDevice;
}

