/*
 * Copyright 2005-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Casalinuovo Dario
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#include "PeopleApp.h"

#include <Alert.h>
#include <AutoDeleter.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Contact.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <Locale.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "PersonWindow.h"
#include "PersonIcons.h"

#include <string.h>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"

TPeopleApp::TPeopleApp()
	:
	BApplication(APP_SIG),
	fWindowCount(0)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("People");

	fPosition.Set(6, TITLE_BAR_HEIGHT, 70 + WIND_WIDTH,
		TITLE_BAR_HEIGHT + WIND_HEIGHT);
	BPoint pos = fPosition.LeftTop();

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir(path.Path());
	BEntry entry;
	if (dir.FindEntry("People_data", &entry) == B_OK) {
		fPrefs = new BFile(&entry, B_READ_WRITE);
		if (fPrefs->InitCheck() == B_NO_ERROR) {
			fPrefs->Read(&pos, sizeof(BPoint));
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(pos))
				fPosition.OffsetTo(pos);
		}
	} else {
		fPrefs = new BFile();
		if (dir.CreateFile("People_data", fPrefs) != B_OK) {
			delete fPrefs;
			fPrefs = NULL;
		}
	}
}


TPeopleApp::~TPeopleApp()
{
	delete fPrefs;
}


void
TPeopleApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage message(B_REFS_RECEIVED);

	for (int32 i = 1; i < argc; i++) {
		BEntry entry(argv[i]);
		entry_ref ref;
		if (entry.Exists() && entry.GetRef(&ref) == B_OK)
			message.AddRef("refs", &ref);
	}

	RefsReceived(&message);
}


void
TPeopleApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_NEW:
		case B_SILENT_RELAUNCH:
			_NewWindow();
			break;

		case M_WINDOW_QUITS:
			_SavePreferences(message);
			fWindowCount--;
			if (fWindowCount < 1)
				PostMessage(B_QUIT_REQUESTED);
			break;

		case M_CONFIGURE_ATTRIBUTES:
		{
			const char* arguments[] = { "-type", B_PERSON_MIMETYPE, 0 };
			status_t ret = be_roster->Launch(
				"application/x-vnd.Haiku-FileTypes",
				sizeof(arguments) / sizeof(const char*) - 1,
				const_cast<char**>(arguments));
			if (ret != B_OK && ret != B_ALREADY_RUNNING) {
				BString errorMsg(B_TRANSLATE("Launching the FileTypes "
					"preflet to configure Person attributes has failed."
					"\n\nError: "));
				errorMsg << strerror(ret);
				BAlert* alert = new BAlert(B_TRANSLATE("Error"),
					errorMsg.String(), B_TRANSLATE("OK"), NULL, NULL,
					B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go(NULL);
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
	}
}


void
TPeopleApp::RefsReceived(BMessage* message)
{
	int32 index = 0;
	while (message->HasRef("refs", index)) {
		entry_ref ref;
		message->FindRef("refs", index++, &ref);
		PersonWindow* window = _FindWindow(ref);
		if (window != NULL) {
			window->Activate(true);
		} else {
			BFile* file = new BFile(&ref, B_READ_WRITE);
			if (file->InitCheck() == B_OK)
				_NewWindow(&ref, file);
		}
	}
}


void
TPeopleApp::ReadyToRun()
{
	if (fWindowCount < 1)
		_NewWindow();
}


bool
TPeopleApp::QuitRequested()
{
	return true;
}


PersonWindow*
TPeopleApp::_NewWindow(const entry_ref* ref, BFile* file)
{
	BRawContact* rawContact;
	if (file == NULL)
		rawContact = new BRawContact(B_CONTACT_FORMAT);
	else
		rawContact = new BRawContact(B_CONTACT_ANY, file);

	BContact* contact = new BContact(rawContact);

	if (contact->InitCheck() != B_OK) {
		BAlert *alert = new BAlert("Alert",
			B_TRANSLATE("Contact initialization failed!."),"OK");
		alert->Go();
		return NULL;
	}

	PersonWindow* window = new PersonWindow(fPosition,
		B_TRANSLATE("New contact"), ref, file, contact);

	window->Show();
	window->Activate(true);

	fWindowCount++;

	// Offset the position for the next window which will be opened and
	// reset it if it would open outside the screen bounds.
	fPosition.OffsetBy(20, 20);
	BScreen screen(window);
	if (fPosition.bottom > screen.Frame().bottom)
		fPosition.OffsetTo(fPosition.left, TITLE_BAR_HEIGHT);
	if (fPosition.right > screen.Frame().right)
		fPosition.OffsetTo(6, fPosition.top);

	return window;
}


PersonWindow*
TPeopleApp::_FindWindow(const entry_ref& ref) const
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		PersonWindow* personWindow = dynamic_cast<PersonWindow*>(window);
		if (personWindow == NULL)
			continue;
		if (personWindow->RefersContactFile(ref))
			return personWindow;
	}
	return NULL;
}


void
TPeopleApp::_SavePreferences(BMessage* message) const
{
	BRect frame;
	if (message->FindRect("frame", &frame) != B_OK)
		return;

	BPoint leftTop = frame.LeftTop();

	if (fPrefs != NULL) {
		fPrefs->Seek(0, 0);
		fPrefs->Write(&leftTop, sizeof(BPoint));
	}
}

