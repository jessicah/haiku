/*
 * Copyright 2011-2012 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _VCARD_TRANSLATOR_H
#define _VCARD_TRANSLATOR_H

#include <ContactDefs.h>
#include <Translator.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>

#include "BaseTranslator.h"
#include "TranslatorSettings.h"
#include "TranslatorWindow.h"
#include "VCardParserDefs.h"
#include "VCardView.h"

#define VCARD_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0,1,0)
#define IN_QUALITY 1
#define IN_CAPABILITY 1
#define OUT_QUALITY 1
#define OUT_CAPABILITY 1

class VCardTranslator : public BaseTranslator {
public:
	VCardTranslator();
	
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
		
	virtual BView* NewConfigView(TranslatorSettings* settings);

protected:
	virtual ~VCardTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	status_t TranslateContact(BMessage* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination);

	status_t TranslateVCard(BPositionIO* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination);

	status_t _IdentifyVCard(BPositionIO* inSource, translator_info* outInfo);

	void 	_WriteBegin(BPositionIO* dest);
	void 	_WriteEnd(BPositionIO* dest);
	void	_Write(BPositionIO* dest, BString& str, const BString& value);
	void	_TranslateIM(BContactField* field, BPositionIO* outDestination);

	bool fVCard;

	OutFieldsMap fFieldsMap;
	OutUsagesMap fUsagesMap;
};

#endif // #ifndef _VCARD_TRANSLATOR_H
