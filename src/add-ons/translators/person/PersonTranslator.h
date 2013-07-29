/*
 * Copyright 2010 Dario Casalinuovo <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _PERSON_TRANSLATOR_H
#define _PERSON_TRANSLATOR_H

#include <ContactDefs.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>

#include "BaseTranslator.h"
#include "PersonView.h"
#include "TranslatorSettings.h"
#include "TranslatorWindow.h"

#define PERSON_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0,1,0)
#define IN_QUALITY 1
#define IN_CAPABILITY 1
#define OUT_QUALITY 1
#define OUT_CAPABILITY 1

#define B_PEOPLE_NAME "META:name"
#define B_PEOPLE_NICKNAME "META:nickname"
#define B_PEOPLE_COMPANY "META:company"
#define B_PEOPLE_ADDRESS "META:address"
#define B_PEOPLE_CITY "META:city"
#define B_PEOPLE_STATE "META:state"
#define B_PEOPLE_ZIP "META:zip"
#define B_PEOPLE_COUNTRY "META:country"
#define B_PEOPLE_HPHONE "META:hphone"
#define B_PEOPLE_WPHONE "META:wphone"
#define B_PEOPLE_FAX "META:fax"
#define B_PEOPLE_EMAIL "META:email"
#define B_PEOPLE_URL "META:url"
#define B_PEOPLE_GROUP "META:group"

class BFile;
class BContactField;

class PersonTranslator : public BaseTranslator {
public:
	PersonTranslator();
	
	virtual status_t Identify(BPositionIO* inSource,
		const translation_format* inFormat, BMessage* ioExtension,
		translator_info* outInfo, uint32 outType);
		// determines whether or not this translator can convert the
		// data in inSource to the type outType

	virtual status_t Translate(BPositionIO* inSource,
		const translator_info* inInfo, BMessage* ioExtension,
		uint32 outType, BPositionIO* outDestination);
		// this function is the whole point of the Translation Kit,
		// it translates the data in inSource to outDestination
		// using the format outType

			status_t TranslateContact(BMessage* inSource, 
				BMessage* ioExtension, BFile* outDestination);

			status_t TranslatePerson(BPositionIO* inSource, 
				BMessage* ioExtension, BPositionIO* outDestination);

	virtual BView* 	NewConfigView(TranslatorSettings* settings);

protected:
	virtual 		~PersonTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
			status_t _IdentifyPerson(BPositionIO* inSource,
						translator_info* outInfo);

			BFile*	 _PositionToFile(BPositionIO* destination);
			status_t _AddField(BContactField* field, BMessage* msg);

			status_t _InitializeAttributes(BFile* file);
			status_t _CheckPerson(BPositionIO* people);
			status_t _AddPicture(BFile* file, BMessage* msg);

		//	status_t _InitAttr();
};

#endif // #ifndef _Person_TRANSLATOR_H
