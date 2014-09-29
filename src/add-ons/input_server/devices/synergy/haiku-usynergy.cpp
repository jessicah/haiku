#include <Notification.h>
#include <Screen.h>
#include <OS.h>
#include <cstdlib>
#include <strings.h>
#include <errno.h>
#include <arpa/inet.h>

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

status_t
uSynergyInputServerDevice::uSynergyThreadLoop(void* arg)
{
	uSynergyContext *uSynergyHaikuContext = (uSynergyContext*)arg;
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)uSynergyHaikuContext->m_cookie;

	while (inputDevice->threadActive) {
		uSynergyUpdate(uSynergyHaikuContext);
	}

	return B_OK;
}


extern "C" {
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
	static int16_t			 oldWheelX = 0, oldWheelY = 0;
	float				 xVal = (float)x / (float)inputDevice->uSynergyHaikuContext->m_clientWidth;
	float				 yVal = (float)y / (float)inputDevice->uSynergyHaikuContext->m_clientHeight;

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
		BMessage* message = inputDevice->_BuildMouseMessage(pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP, system_time(), buttons, xVal, yVal);
		if (message != NULL)
			inputDevice->EnqueueMessage(message);
		oldButtons = buttons;
	}

	if ((x != oldX) || (y != oldY)) {
		BMessage* message = inputDevice->_BuildMouseMessage(B_MOUSE_MOVED, system_time(), buttons, xVal, yVal);
		if (message != NULL)
			inputDevice->EnqueueMessage(message);
		oldX = x;
		oldY = y;
	}

	if ((oldWheelX != wheelX) || (oldWheelY = wheelY)) {
		BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
		if (message != NULL) {
			if (message->AddInt64("when", system_time()) == B_OK
			    && message->AddFloat("be:wheel_delta_x", wheelX) == B_OK
			    && message->AddFloat("be:wheel_delta_y", wheelY) == B_OK)
				inputDevice->EnqueueMessage(message);
			else
				delete message;
		}
		oldWheelX = wheelX;
		oldWheelY = wheelY;
	}
}

void
uSynergyKeyboardCallbackHaiku(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat)
{

}

void
uSynergyJoystickCallbackHaiku(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY)
{

}

void
uSynergyClipboardCallbackHaiku(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size)
{

}

}

extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new (std::nothrow)uSynergyInputServerDevice;
}

