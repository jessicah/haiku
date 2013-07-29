/*
 * Copyright 2010-2012 Casalinuovo Dario
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _VCARD_PARSER_DEFS_H
#define _VCARD_PARSER_DEFS_H

#include <ContactDefs.h>
#include <ContactField.h>
#include <HashMap.h>
#include <HashString.h>

using BPrivate::HashMap;
using BPrivate::HashString;

// VCard 2.1 standard fields

#define VCARD_ADDRESS              "ADR"
#define VCARD_BIRTHDAY             "BDAY"
#define VCARD_DELIVERY_LABEL       "LABEL"
#define VCARD_EMAIL                "EMAIL"
#define VCARD_FORMATTED_NAME       "FN"
#define VCARD_REVISION             "REV"
#define VCARD_SOUND                "SOUND"
#define VCARD_TELEPHONE            "TEL"
#define VCARD_TIME_ZONE            "TZ"
#define VCARD_TITLE                "TITLE"
#define VCARD_URL                  "URL"
#define VCARD_GEOGRAPHIC_POSITION  "GEO"
#define VCARD_NAME                 "N"
#define VCARD_NICKNAME             "NICKNAME"
#define VCARD_NOTE                 "NOTE"
#define VCARD_ORGANIZATION         "ORG"
#define VCARD_PHOTO                "PHOTO"
#define VCARD_VERSION              "VERSION"

// Currently unsupported standard fields

#define VCARD_AGENT                "AGENT"
#define VCARD_CATEGORIES           "CATEGORIES"
#define VCARD_CLASS                "CLASS"
#define VCARD_ROLE                 "ROLE"
#define VCARD_SORT_STRING          "SORT-STRING"
#define VCARD_KEY                  "KEY"
#define VCARD_LOGO                 "LOGO"
#define VCARD_MAILER               "MAILER"
#define VCARD_PRODUCT_IDENTIFIER   "PRODID"

// Custom Haiku OS extensions

#define X_VCARD_IM					"X-HAIKU-IM"
#define X_VCARD_PROTOCOLS			"X-HAIKU-PROTOCOLS"
#define X_VCARD_SIMPLE_GROUP		"X-HAIKU-SIMPLEGROUP"
#define X_VCARD_GROUP				"X-HAIKU-GROUP"
#define X_VCARD_UID					"X-HAIKU-UID"

// Non-Haiku custom fields supported

#define X_VCARD_GENDER				"X-GENDER"
#define X_VCARD_ASSISTANT			"X-ASSISTANT"
#define X_VCARD_MANAGER				"X-MANAGER"
#define X_VCARD_SPOUSE				"X-SPOUSE"

#define X_VCARD_AIM					"X-AIM"
#define X_VCARD_ICQ					"X-ICQ"
#define X_VCARD_MSN					"X-MSN"
#define X_VCARD_JABBER				"X-JABBER"
#define X_VCARD_YAHOO				"X-YAHOO"
#define X_VCARD_TWITTER				"X-TWITTER"
#define X_VCARD_SKYPE				"X-SKYPE"
#define X_VCARD_GADUGADU			"X-GADUGADU"
#define X_VCARD_GROUPWISE			"X-GROUPWISE"

// Unsupported non-Haiku custom fields

#define X_VCARD_SKYPE_USERNAME		"X-SKYPE-USERNAME"

#define X_VCARD_PHONETIC_LAST_NAME	"X-PHONETIC-LAST-NAME"
#define X_VCARD_PHONETIC_FIRST_NAME "X-PHONETIC-FIRST-NAME"

// VCard <-> BContact defs
struct fieldMap {
	const char* key;
	field_type type;
	int	op;
};

struct usageMap {
	const char* key;
	field_usage usage;
};
// This is a translation table
// that will be used to fill a map with the purpose to convert
// fields from BContact to VCard.
static fieldMap gFieldsMap[] = {
		// Standard VCard 2.0 fields
		{ VCARD_ADDRESS, B_CONTACT_ADDRESS },
		{ X_VCARD_ASSISTANT, B_CONTACT_ASSISTANT },
		{ VCARD_BIRTHDAY, B_CONTACT_BIRTHDAY },
 		{ VCARD_DELIVERY_LABEL, B_CONTACT_DELIVERY_LABEL },
 		{ VCARD_EMAIL, B_CONTACT_EMAIL },
		{ VCARD_FORMATTED_NAME, B_CONTACT_FORMATTED_NAME },
		{ X_VCARD_GENDER, B_CONTACT_GENDER },
		{ VCARD_GEOGRAPHIC_POSITION, B_CONTACT_GEO },
		{ X_VCARD_MANAGER, B_CONTACT_MANAGER },
		{ VCARD_NAME, B_CONTACT_NAME },
		{ VCARD_NICKNAME, B_CONTACT_NICKNAME },
		{ VCARD_NOTE, B_CONTACT_NOTE },
		{ VCARD_ORGANIZATION, B_CONTACT_ORGANIZATION },
		{ VCARD_TELEPHONE, B_CONTACT_PHONE },
		{ VCARD_PHOTO, B_CONTACT_PHOTO },
		{ X_VCARD_SPOUSE, B_CONTACT_SPOUSE },
		{ VCARD_SOUND, B_CONTACT_SOUND },
		{ VCARD_TIME_ZONE, B_CONTACT_TIME_ZONE },
		{ VCARD_TITLE, B_CONTACT_TITLE },
		{ VCARD_URL, B_CONTACT_URL },
		{ VCARD_REVISION, B_CONTACT_REV },

		// Custom VCard Haiku fields
		{ X_VCARD_IM, B_CONTACT_IM },
		{ X_VCARD_PROTOCOLS, B_CONTACT_PROTOCOLS },
		{ X_VCARD_SIMPLE_GROUP, B_CONTACT_SIMPLE_GROUP },
		{ X_VCARD_GROUP, B_CONTACT_GROUP },
		{ X_VCARD_UID, B_CONTACT_UID },

		{ NULL, 0 }
};

static usageMap gUsagesMap[] = {
		// Standard vcard 2.0 usages
		//{ "",   CONTACT_DATA_CUSTOM },
		//{ "" ,  CONTACT_DATA_OTHER },
		{ "HOME", CONTACT_DATA_HOME },
		{ "WORK", CONTACT_DATA_WORK },
		{ "PREF", CONTACT_DATA_PREFERRED },

		{ "" , CONTACT_NAME_FAMILY },
		{ "" , CONTACT_NAME_GIVEN },
		{ "" , CONTACT_NAME_MIDDLE },
		{ "" , CONTACT_NAME_SUFFIX },

		{ "" , CONTACT_NICKNAME_DEFAULT },
		{ "" , CONTACT_NICKNAME_MAIDEN },
		{ "" , CONTACT_NICKNAME_SHORT_NAME },
		{ "" , CONTACT_NICKNAME_INITIALS },

		{ "" , CONTACT_EMAIL_MOBILE },
		{ "INTERNET" , CONTACT_EMAIL_INTERNET },

		{ "CELL", CONTACT_PHONE_MOBILE },
		{ "FAX", CONTACT_PHONE_FAX },
		{ "PAGER" , CONTACT_PHONE_PAGER },
		{ "" , CONTACT_PHONE_CALLBACK },
		{ "CAR" , CONTACT_PHONE_CAR },
		{ "ISDN" , CONTACT_PHONE_ISDN },
		{ "" , CONTACT_PHONE_RADIO },
		{ "" , CONTACT_PHONE_TELEX },
		{ "" , CONTACT_PHONE_TTY_TDD },
		{ "MODEM" , CONTACT_PHONE_MODEM },
		{ "" , CONTACT_PHONE_ASSISTANT },
		{ "VIDEO" , CONTACT_PHONE_VIDEO },
		{ "VOICE" , CONTACT_PHONE_VOICE },
		{ "MSG", CONTACT_PHONE_MSG },

		{ "" , CONTACT_PHOTO_BITMAP },

		{ "DOM" , CONTACT_ADDRESS_DOM },
		{ "INTL" , CONTACT_ADDRESS_INTL },
		{ "POSTAL" , CONTACT_ADDRESS_POSTAL },
		{ "PARCEL" , CONTACT_ADDRESS_PARCEL },

		{ "WAVE" , CONTACT_SOUND_WAVE },
		{ "PCM" , CONTACT_SOUND_PCM },
		{ "AIFF" , CONTACT_SOUND_AIFF },
		{ "" , CONTACT_SOUND_MP3 },
		{ "" , CONTACT_SOUND_OGG },

		{ "X-VCARD-AIM" , CONTACT_IM_AIM },
		{ "X-VCARD-ICQ" , CONTACT_IM_ICQ },
		{ "X-VCARD-MSN" , CONTACT_IM_MSN },
		{ "X-VCARD-JABBER" , CONTACT_IM_JABBER },
		{ "X-VCARD-YAHOO" , CONTACT_IM_YAHOO },
		{ "X-VCARD-TWITTER" , CONTACT_IM_TWITTER },
		{ "X-VCARD-SKYPE" , CONTACT_IM_SKYPE },
		{ "X-VCARD-GADUGADU" , CONTACT_IM_GADUGADU },
		{ "X-VCARD-GROUPWISE" , CONTACT_IM_GROUPWISE },
		{ NULL, 0 }
	};

// Translation map typedefs
typedef HashMap<HashKey32<field_type>, BString> OutFieldsMap;
typedef HashMap<HashKey32<field_usage>, BString> OutUsagesMap;

typedef HashMap<HashString, field_type> InFieldsMap;
typedef HashMap<HashString, field_usage> InUsagesMap;

#endif
