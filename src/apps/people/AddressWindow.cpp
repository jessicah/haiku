/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */

#include <LayoutBuilder.h>

#include "AddressView.h"
#include "AddressWindow.h"

#include <stdio.h>

AddressWindow::AddressWindow(BContact* contact)
	:
	BWindow(BRect(200, 200, 500, 500), "Locations",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS, B_WILL_DRAW)
{
	fAddressView = new AddressView(contact);
	if (contact == NULL) {
		_CreateEmptyAddress();
		return;
	}

	int32 count = contact->CountFields();
	for (int i = 0; i < count; i++) {
		BAddressContactField* field =
			dynamic_cast<BAddressContactField*>(contact->FieldAt(i));

		if (field != NULL)
			fAddressView->AddAddress(field);
	}
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fAddressView);
}


AddressWindow::~AddressWindow()
{
}


void
AddressWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SHOW_ADDRESS:
		case M_ADD_ADDRESS:
		case M_REMOVE_ADDRESS:
			fAddressView->MessageReceived(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
AddressWindow::QuitRequested()
{
	if (IsHidden() || IsMinimized())
		return true;

	Hide();
	return false;
}


bool
AddressWindow::HasChanged()
{
	return fAddressView->HasChanged();
}


void
AddressWindow::Reload()
{
	fAddressView->Reload();
}


void
AddressWindow::UpdateAddressField()
{
	fAddressView->UpdateAddressField();
}


void
AddressWindow::_CreateEmptyAddress()
{
	BAddressContactField* field = new BAddressContactField();
	fAddressView->AddAddress(field);
}
