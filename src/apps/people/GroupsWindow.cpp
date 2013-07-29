/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */

#include "GroupsWindow.h"


GroupsWindow::GroupsWindow(BContact* contact)
	:
	BWindow(BRect(200, 200, 500, 500), "Groups", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS,
		B_WILL_DRAW),
	fContact(contact)
{
}


GroupsWindow::~GroupsWindow()
{
}


void
GroupsWindow::MessageReceived(BMessage* msg)
{

	switch (msg->what) {

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
GroupsWindow::QuitRequested()
{
	return true;
}
