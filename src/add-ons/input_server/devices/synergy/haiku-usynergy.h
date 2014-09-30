/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 */
#ifndef USYNERGY_H
#define USYNERGY_H

#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Locker.h>

#include <ObjectList.h>

#include "Keymap.h"
#include "uSynergy.h"


uSynergyBool      uSynergyConnectHaiku(uSynergyCookie cookie);
uSynergyBool      uSynergySendHaiku(uSynergyCookie cookie, const uint8_t *buffer, int length);
uSynergyBool      uSynergyReceiveHaiku(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength);
void              uSynergySleepHaiku(uSynergyCookie cookie, int timeMs);
uint32_t          uSynergyGetTimeHaiku();
void              uSynergyTraceHaiku(uSynergyCookie cookie, const char *text);
void              uSynergyScreenActiveCallbackHaiku(uSynergyCookie cookie, uSynergyBool active);
void              uSynergyMouseCallbackHaiku(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle);
void              uSynergyKeyboardCallbackHaiku(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat);
void              uSynergyJoystickCallbackHaiku(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY);
void              uSynergyClipboardCallbackHaiku(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size);

class uSynergyInputServerDevice : public BHandler, public BInputServerDevice {
	public:
					 uSynergyInputServerDevice();
		virtual			~uSynergyInputServerDevice();

		virtual status_t	 InitCheck();

		virtual	void		 MessageReceived(BMessage* message);
		virtual status_t	 Start(const char* name, void* cookie);
		virtual status_t	 Stop(const char* name, void* cookie);
		virtual status_t	 SystemShuttingDown();


		virtual status_t	 Control(const char* name, void* cookie, uint32 command, BMessage* message);
	private:
		bool			 threadActive;
		thread_id		 uSynergyThread;
		uSynergyContext		*uSynergyHaikuContext;

		BMessage*		_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons, float x, float y) const;
		void			_ProcessKeyboard(uint16_t scancode, uint16_t modifiers, bool isKeyDown, bool isKeyRepeat);
		void			_UpdateSettings();
		void			_PostClipboard(const BString &mimetype, const uint8_t *data, uint32_t size);
	public:
		struct sockaddr_in	 synergyServerData;
		int			 synergyServerSocket;
	private:
		static status_t		 uSynergyThreadLoop(void* arg);

		uint32		fModifiers;
		uint32		fCommandKey;
		uint32		fControlKey;
		char*		fFilename;
		bool		fEnableSynergy;
		BString		fServerAddress;

	volatile bool	fUpdateSettings;

		Keymap		fKeymap;
		BLocker		fKeymapLock;

	public:
		/* callbacks for uSynergy */
		friend uSynergyBool	 uSynergyConnectHaiku(uSynergyCookie cookie);
		friend uSynergyBool	 uSynergySendHaiku(uSynergyCookie cookie, const uint8_t *buffer, int length);
		friend uSynergyBool	 uSynergyReceiveHaiku(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength);
		friend void		 uSynergySleepHaiku(uSynergyCookie cookie, int timeMs);
		friend uint32_t		 uSynergyGetTimeHaiku();
		friend void		 uSynergyTraceHaiku(uSynergyCookie cookie, const char *text);
		friend void		 uSynergyScreenActiveCallbackHaiku(uSynergyCookie cookie, uSynergyBool active);
		friend void		 uSynergyMouseCallbackHaiku(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX, int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight, uSynergyBool buttonMiddle);
		friend void		 uSynergyKeyboardCallbackHaiku(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat);
		friend void		 uSynergyJoystickCallbackHaiku(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY);
		friend void		 uSynergyClipboardCallbackHaiku(uSynergyCookie cookie, enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size);
};


extern "C" BInputServerDevice* instantiate_input_device();

#endif /* USYNERGY_H */
