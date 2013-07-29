/*
 * Copyright 2010 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <RawContact.h>

#include <File.h>
#include <TranslatorFormats.h>

#include <stdio.h>
#include <string.h>

BRawContact::BRawContact(BMessage* data)
	:
	fDest(NULL),
	fInitCheck(B_OK)
{
	status_t ret = data->FindUInt32("FinalFormat", &fFormat);
	if (ret != B_OK)
		fInitCheck = ret;

	_Init();
	_InitTranslator();
}


BRawContact::BRawContact(uint32 finalFormat, BPositionIO* destination)
	:
	fDest(destination),
	fInitCheck(B_OK),
	fFormat(finalFormat)
{
	fRoster = BTranslatorRoster::Default();

	_Init();
	_InitTranslator();
}


BRawContact::~BRawContact()
{
	delete fDest;
}


void
BRawContact::_Init()
{
	printf("BRawContact::_Init()\n");
	if (fFormat == B_CONTACT_ANY && fDest != NULL) {
		if (_FindFormat() == B_OK)
			return;
		else
			fFormat = B_CONTACT_FORMAT;
	}

	if (fDest == NULL && fFormat != B_PERSON_FORMAT) {
		if (fFormat == B_CONTACT_ANY)
			fFormat = B_CONTACT_FORMAT;

		fDest = new BMallocIO();
		return;
	}

	if (!_CheckDestination(fDest)) {
		printf("checkdest failed\n");
		fDest = NULL;
		fInitCheck = B_BAD_VALUE;
	}
}


status_t
BRawContact::_FindFormat()
{
	translator_info info;
	status_t ret = fRoster->Identify(fDest, NULL, &info,
		0, NULL, 0);

	if (ret != B_OK)
		return ret;

	fFormat = info.type;
	fTranslatorID = info.translator;

	return B_OK;
}


void
BRawContact::_InitTranslator()
{
	// NOTA magari si potrebbe usare BContactRoster
	// per questa funzione fornendo un metodo 
	// che dato un formato restituisce l'id del translator
	printf("BRawContact::InitTranslator\n");
	fTranslatorID = 0;
	int32 num_translators;
	translator_id* translators;

	fRoster->GetAllTranslators(&translators, &num_translators);
	for (int i = 0; i < num_translators; i++) {
		const translation_format* format;
		int32 noutFormat;
		fRoster->GetOutputFormats(translators[i], &format, &noutFormat);
		for (int j = 0; j < noutFormat; j++) {
			if (format[j].group == B_TRANSLATOR_CONTACT 
				&& format[j].type == fFormat) {
				fTranslatorID = translators[i];
				printf("BRawContact: Got a suitable Translator %d %s\n",
					(int)fTranslatorID, format[j].MIME);
				break;
			}
		}
	}

	delete[] translators;

	if (fTranslatorID == 0)
		fInitCheck = B_NO_TRANSLATOR;
}


bool
BRawContact::_CheckDestination(BPositionIO* destination)
{
	if (fFormat != B_PERSON_FORMAT)
		return true;

	// this is an exception
	// because we can't write attributes
	// to a BPositionIO, so we need it
	// at least to be a BFile, otherwise
	// you will not be able to translate
	// something using the people
	// translator
	if (fFormat == B_PERSON_FORMAT) {
		if (destination == NULL)
			return false;
		BFile* file = dynamic_cast<BFile*>(destination);
		if (file == NULL)
			return false;
		else if (file->InitCheck() == B_OK)
			return true;
	}
	return true;
}


status_t
BRawContact::Archive(BMessage* archive, bool deep) const
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	BArchivable::Archive(archive, deep);
	archive->AddUInt32("FinalFormat", fFormat);

	return B_OK;
}


BArchivable*
BRawContact::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BRawContact"))
		return new BRawContact(data);

	return NULL;
}


status_t
BRawContact::InitCheck() const
{
	return fInitCheck;	
}


status_t
BRawContact::Read(BMessage* data)
{
	if (fInitCheck != B_OK)
		return fInitCheck;
	if (fTranslatorID == 0)
		return B_NO_TRANSLATOR;
	if (fDest == NULL)
		return B_ERROR;

	if (data == NULL)
		return B_BAD_VALUE;

	status_t ret;
	BMallocIO flat;

	ret = fRoster->Translate(fTranslatorID, fDest,
		NULL, &flat, B_CONTACT_FORMAT);

	flat.Seek(0, SEEK_SET);
	return data->Unflatten((const char*)flat.Buffer());
}


status_t
BRawContact::Commit(BMessage* data)
{
	if (fInitCheck != B_OK)
		return fInitCheck;
	if (fTranslatorID == 0)
		return B_NO_TRANSLATOR;
	if (fDest == NULL)
		return B_ERROR;
	if (data == NULL)
		return B_BAD_VALUE;

	status_t ret;
	BMallocIO flat;
	ret = data->Flatten(&flat);

	flat.Seek(0, SEEK_SET);
	if (ret == B_OK)
		return fRoster->Translate(fTranslatorID, &flat,
			NULL, fDest, fFormat);

	return ret;
}


int32
BRawContact::FinalFormat() const
{
	return fFormat;
}


BPositionIO*
BRawContact::Destination() const
{
	return fDest;
}


status_t
BRawContact::SetDestination(BPositionIO* destination, bool del)
{
	if (del)
		delete fDest;

	if (destination != NULL)
		if (_CheckDestination(destination)) {
			fDest = destination;
			return B_OK;
		}

	return B_ERROR;
}
