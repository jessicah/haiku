/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */

#ifndef GROUPS_WINDOW_H
#define GROUPS_WINDOW_H

#include <Contact.h>
#include <Rect.h>
#include <Window.h>

class GroupsWindow : public BWindow {
public:

								GroupsWindow(BContact* contact);
	virtual						~GroupsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
private:
			BContact*			fContact;
};


#endif
