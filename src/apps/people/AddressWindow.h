/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */

#ifndef ADDRESS_WINDOW_H
#define ADDRESS_WINDOW_H


#include <Contact.h>
#include <ObjectList.h>
#include <Window.h>

class AddressView;


class AddressWindow : public BWindow {
public:

								AddressWindow(BContact* contact);
	virtual						~AddressWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			bool				HasChanged();
			void				Reload();
			void				UpdateAddressField();
private:
			void				_CreateEmptyAddress();
			AddressView*		fAddressView;
};


#endif // ADDRESS_WINDOW_H
