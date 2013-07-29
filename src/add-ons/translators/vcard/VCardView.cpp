/*
 * Copyright 2002-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! A view with information about the VCardTranslator. */


#include "VCardView.h"
#include "VCardTranslator.h"

#include <GroupView.h>
#include <StringView.h>

#include <stdio.h>


VCardView::VCardView(const BRect &frame, const char *name, uint32 resizeMode,
		uint32 flags, TranslatorSettings *settings)
	:
	BView(frame, name, resizeMode, flags)
{
	// TODO use layouts
	fSettings = settings;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupView* view = new BGroupView(B_VERTICAL);

	BStringView* stringView = new BStringView("title",
		"VCard files translator");

	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	
	view->AddChild(stringView);
	char version[256];

	snprintf(version, sizeof(version), "Version %d.%d.%d, %s",
		int(VCARD_TRANSLATOR_VERSION),
		int(VCARD_TRANSLATOR_VERSION),
		int(VCARD_TRANSLATOR_VERSION),
		__DATE__);

	stringView = new BStringView("version", version);
	stringView->ResizeToPreferred();
	view->AddChild(stringView);

	stringView = new BStringView("Copyright",
		B_UTF8_COPYRIGHT "2011-2012 Haiku Inc.");

	stringView->ResizeToPreferred();
	view->AddChild(stringView);

	AddChild(view);
}


VCardView::~VCardView()
{
	fSettings->Release();
}
