
#include <Application.h>
#include <Autolock.h>
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
	fUpdateSettings(false),
	fKeymapLock("synergy keymap lock")
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
	return system_time();
}

void
uSynergyTraceHaiku(uSynergyCookie cookie, const char *text)
{
	BNotification *notify = new BNotification(B_INFORMATION_NOTIFICATION);
	BString group("uSynergy");
	BString content(text);

	notify->SetGroup(group);
	notify->SetContent(content);
	notify->Send(5000);
}

void
uSynergyScreenActiveCallbackHaiku(uSynergyCookie cookie, uSynergyBool active)
{

}

void
uSynergyMouseCallbackHaiku(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle)
{
	uSynergyInputServerDevice	*inputDevice = (uSynergyInputServerDevice*)cookie;
	static uint32_t			 oldButtons = 0;
	uint32_t			 buttons = 0;
	static uint16_t			 oldX = 0, oldY = 0;
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

	if (wheelX != 0 && wheelY != 0) {
		BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
		if (message != NULL) {
			if (message->AddInt64("when", timestamp) == B_OK
			    && message->AddFloat("be:wheel_delta_x", wheelX) == B_OK
			    && message->AddFloat("be:wheel_delta_y", wheelY) == B_OK)
				inputDevice->EnqueueMessage(message);
			else
				delete message;
		}
	}
}

void
uSynergyInputServerDevice::_ProcessKeyboard(uint16_t keycode, uint16_t _modifiers, bool isKeyDown, bool isKeyRepeat)
{
	static uint8 activeDeadKey = 0;
	static uint32 lastKeyCode = 0;
	static uint32 repeatCount = 1;
	static uint8 states[16];

	int64 timestamp = system_time();

	TRACE("synergy: keycode = %04x, modifiers = %04x\n", keycode, _modifiers);

	if (isKeyDown && keycode == 0x68) {
		// MENU KEY for Tracker
		bool noOtherKeyPressed = true;
		for (int32 i = 0; i < 16; ++i) {
			if (states[i] != 0) {
				noOtherKeyPressed = false;
				break;
			}
		}

		if (noOtherKeyPressed) {
			BMessenger deskbar("application/x-bnd.Be-TSKB");
			if (deskbar.IsValid())
				deskbar.SendMessage('BeMn');
		}
	}

	if (keycode < 256) {
		if (isKeyDown)
			states[(keycode) >> 3] |= (1 << (7 - (keycode & 0x7)));
		else
			states[(keycode) >> 3] &= (!(1 << (7 - (keycode & 0x7))));
	}
#if false
	if (isKeyDown && keycode == 0x34 // DELETE KEY
		&& (states[fCommandKey >> 3] & (1 << (7 - (fCommandKey & 0x7))))
		&& (states[fControlKey >> 3] & (1 << (7 - (fControlKey & 0x7))))) {
		//LOG_EVENT("TeamMonitor called\n");

		// show the team monitor
		if (fOwner->fTeamMonitorWindow == NULL)
			fOwner->fTeamMonitorWindow = new(std::nothrow) TeamMonitorWindow();

		if (fOwner->fTeamMonitorWindow != NULL)
			fOwner->fTeamMonitorWindow->Enable();

		ctrlAltDelPressed = true;
	}

	if (ctrlAltDelPressed) {
		if (fOwner->fTeamMonitorWindow != NULL) {
			BMessage message(kMsgCtrlAltDelPressed);
			message.AddBool("key down", isKeyDown);
			fOwner->fTeamMonitorWindow->PostMessage(&message);
		}

		if (!isKeyDown)
			ctrlAltDelPressed = false;
	}
#endif
	BAutolock lock(fKeymapLock);

	uint32 modifiers = fKeymap.Modifier(keycode);
	bool isLock
		= (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) != 0;
	if (modifiers != 0 && (!isLock || isKeyDown)) {
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

			message->AddInt64("when", timestamp);
			message->AddInt32("be:old_modifiers", oldModifiers);
			message->AddInt32("modifiers", fModifiers);
			message->AddData("states", B_UINT8_TYPE, states, 16);

			if (EnqueueMessage(message) != B_OK)
				delete message;
		}
	}

	uint8 newDeadKey = 0;
	if (activeDeadKey == 0 || !isKeyDown)
		newDeadKey = fKeymap.ActiveDeadKey(keycode, fModifiers);

	char* string = NULL;
	char* rawString = NULL;
	int32 numBytes = 0, rawNumBytes = 0;
	if (newDeadKey == 0) {
		fKeymap.GetChars(keycode, fModifiers, activeDeadKey, &string,
			&numBytes);
	}
	fKeymap.GetChars(keycode, 0, 0, &rawString, &rawNumBytes);

	BMessage* msg = new BMessage;
	if (msg == NULL) {
		delete[] string;
		delete[] rawString;
		return;
	}

	if (numBytes > 0)
		msg->what = isKeyDown ? B_KEY_DOWN : B_KEY_UP;
	else
		msg->what = isKeyDown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;

	msg->AddInt64("when", timestamp);
	msg->AddInt32("key", keycode);
	msg->AddInt32("modifiers", fModifiers);
	msg->AddData("states", B_UINT8_TYPE, states, 16);
	if (numBytes > 0) {
		for (int i = 0; i < numBytes; i++)
			msg->AddInt8("byte", (int8)string[i]);
		msg->AddData("bytes", B_STRING_TYPE, string, numBytes + 1);

		if (rawNumBytes <= 0) {
			rawNumBytes = 1;
			delete[] rawString;
			rawString = string;
		} else
			delete[] string;

		if (isKeyDown && lastKeyCode == keycode) {
			repeatCount++;
			msg->AddInt32("be:key_repeat", repeatCount);
		} else
			repeatCount = 1;
	} else
		delete[] string;

	if (rawNumBytes > 0)
		msg->AddInt32("raw_char", (uint32)((uint8)rawString[0] & 0x7f));

	delete[] rawString;
#if 0
	if (newDeadKey == 0) {
		if (isKeyDown && !modifiers && activeDeadKey != 0) {
			// a dead key was completed
			activeDeadKey = 0;
			if (fInputMethodStarted) {
				_EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED,
					string, true, msg);
				_EnqueueInlineInputMethod(B_INPUT_METHOD_STOPPED);
				fInputMethodStarted = false;
				msg = NULL;
			}
		}
	} else if (isKeyDown
		&& _EnqueueInlineInputMethod(B_INPUT_METHOD_STARTED) == B_OK) {
		// start of a dead key
		char* string = NULL;
		int32 numBytes = 0;
		fKeymap.GetChars(keycode, fModifiers, 0, &string, &numBytes);

		if (_EnqueueInlineInputMethod(B_INPUT_METHOD_CHANGED, string)
				== B_OK)
			fInputMethodStarted = true;

		activeDeadKey = newDeadKey;
		delete[] string;
	}
#endif
	if (msg != NULL && EnqueueMessage(msg) != B_OK)
		delete msg;

	lastKeyCode = isKeyDown ? keycode : 0;
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
uSynergyClipboardCallbackHaiku(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size)
{

}


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new (std::nothrow)uSynergyInputServerDevice;
}

