/*
 * Copyright 2011-2012 Casalinuovo Dario
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <HashMap.h>

struct standardFieldsMap {
	field_type type;
	const char* label;
};

static standardFieldsMap gStandardFields[] = {
		{ B_CONTACT_ADDRESS, "Address" },
		{ B_CONTACT_ASSISTANT, "Assistant" },
		{ B_CONTACT_BIRTHDAY, "Birthday" },
		{ B_CONTACT_CUSTOM, "Custom" },
 		{ B_CONTACT_DELIVERY_LABEL, "Delivery Label" },
 		{ B_CONTACT_EMAIL, "Email" },
		{ B_CONTACT_FORMATTED_NAME, "Formatted name" },
		{ B_CONTACT_GENDER, "Gender" },
		{ B_CONTACT_GEO, "Geographic Position" },
		{ B_CONTACT_GROUP, "Group" },
		{ B_CONTACT_IM, "Instant Messaging" },
		{ B_CONTACT_MANAGER, "Manager" },
		{ B_CONTACT_NAME, "Name" },
		{ B_CONTACT_NICKNAME, "Nickname" },
		{ B_CONTACT_NOTE, "Note" },
		{ B_CONTACT_ORGANIZATION, "Organization" },
		{ B_CONTACT_PHONE, "Phone" },
		{ B_CONTACT_PHOTO, "Photo" },
		{ B_CONTACT_PROTOCOLS, "Protocols" },
		{ B_CONTACT_SIMPLE_GROUP, "Simple Group" },
		{ B_CONTACT_SOUND, "Sound" },
		{ B_CONTACT_SPOUSE, "Spouse" },
		{ B_CONTACT_TIME_ZONE, "Time Zone" },
		{ B_CONTACT_TITLE, "Title" },
		{ B_CONTACT_URL, "Url" },
		{ B_CONTACT_REV, "Revision" },
		{ B_CONTACT_UID, "Haiku UID" },
		{ 0, 0 }
};

struct standardUsagesMap {
	field_usage usage;
	const char* label;
	int32 		stringOp;
	// this is used to identify 
	// the operation that should be done
	// when creating the label
	// the value can be :
	// 1 - Prepend
	// 2 - Append
	// 3 - Replace
	const char* replaceString;
};

static standardUsagesMap gStandardUsages[] = {
		{ CONTACT_DATA_HOME, "Home ", 1 },
		{ CONTACT_DATA_WORK, "Work ", 1 },
		{ CONTACT_DATA_CUSTOM, "Custom ", 1 },
		{ CONTACT_DATA_OTHER, "Other ", 1 },
		{ CONTACT_DATA_PREFERRED, " (preferred)", 2 },

		{ CONTACT_NAME_FAMILY, "Family ", 1 },
		{ CONTACT_NAME_GIVEN, "Given ", 1  },
		{ CONTACT_NAME_MIDDLE, "Middle ", 1 },
		{ CONTACT_NAME_SUFFIX, "(suffix)", 2 },

		{ CONTACT_NICKNAME_DEFAULT, " (default)", 2 },
		{ CONTACT_NICKNAME_MAIDEN, " (maiden)", 2 },
		{ CONTACT_NICKNAME_SHORT_NAME, " (short name)", 2 },
		{ CONTACT_NICKNAME_INITIALS, " (initials)", 2 },

		{ CONTACT_EMAIL_MOBILE," (mobile)", 2 },
		{ CONTACT_EMAIL_INTERNET," (internet)", 2 },

		{ CONTACT_PHONE_ASSISTANT, "Assistant ", 2 },
		{ CONTACT_PHONE_CALLBACK," (callback)", 2 },
		{ CONTACT_PHONE_CAR," (car)", 2 },
		{ CONTACT_PHONE_FAX, "Fax", 3, "Phone"},
		{ CONTACT_PHONE_ISDN, " (ISDN)", 2 },
		{ CONTACT_PHONE_MMS, " (mms)", 2 },
		{ CONTACT_PHONE_MODEM, " (modem)", 2 },
		{ CONTACT_PHONE_MOBILE, "Mobile ", 1 },
		{ CONTACT_PHONE_MSG, " (msg)", 2 },
		{ CONTACT_PHONE_PAGER, " (pager)", 2 },
		{ CONTACT_PHONE_RADIO, " (radio)", 2 },
		{ CONTACT_PHONE_TELEX, " (telex)", 2 },
		{ CONTACT_PHONE_TTY_TDD, " (tty/tdd)", 2 },
		{ CONTACT_PHONE_VIDEO, " (video)", 2 },
		{ CONTACT_PHONE_VOICE, " (voice)", 2 },

		{ CONTACT_PHOTO_BITMAP, " (bitmap)", 2 },

		{ CONTACT_IM_AIM, " (AIM)", 2 },
		{ CONTACT_IM_ICQ, " (ICQ)", 2 },
		{ CONTACT_IM_MSN, " (MSN)", 2 },
		{ CONTACT_IM_JABBER, " (Jabber)", 2 },
		{ CONTACT_IM_YAHOO, " (Yahoo)", 2 },
		{ CONTACT_IM_TWITTER, " (Twitter)", 2 },
		{ CONTACT_IM_SKYPE, " (Skype)", 2 },
		{ CONTACT_IM_GADUGADU, " (GaduGadu)", 2 },
		{ CONTACT_IM_GROUPWISE, " (Groupwise)", 2 },

		{ 0, 0 }
};
