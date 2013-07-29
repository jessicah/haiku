/*
 * Copyright 2011-2012 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VCardParser.h"

#include <stdio.h>


// c++ bindings to the c API
void HandleProp(void* userData,
	const CARD_Char* propName, const CARD_Char** params)
{
	VCardParser* owner = (VCardParser*) userData;
	owner->PropHandler(propName, params);
}


void
HandleData(void* userData, const CARD_Char* data, int len)
{
	VCardParser* owner = (VCardParser*) userData;
	owner->DataHandler(data, len);
}


VCardParser::VCardParser(BPositionIO* from, bool onlyCheck)
	:
	fFrom(from),
	fOnlyCheck(onlyCheck),
	fCheck(false),
	fBegin(false),
	fEnd(false),
	fLatestParams(true),
	fList(true),
	fFieldsMap(),
	fUsagesMap()
{
	// intialize the parser
	fParser = CARD_ParserCreate(NULL);
	CARD_SetUserData(fParser, this);
	CARD_SetPropHandler(fParser, HandleProp);
	CARD_SetDataHandler(fParser, HandleData);

	// fill the map with the values to translate from VCard to BContact
	for (int i = 0; gFieldsMap[i].key != NULL; i++)
		fFieldsMap.Put(HashString(gFieldsMap[i].key), gFieldsMap[i].type);

	for (int i = 0; gUsagesMap[i].key != NULL; i++)
		fUsagesMap.Put(HashString(gUsagesMap[i].key), gUsagesMap[i].usage);
}


VCardParser::~VCardParser()
{
	// free the object
	CARD_ParserFree(fParser);
}


status_t
VCardParser::Parse()
{
	if (fFrom == NULL)
		return B_ERROR;

	char buf[512];
	ssize_t read;
	read = fFrom->Read(buf, sizeof(buf));

	while (read > 0) {
		int err = CARD_Parse(fParser, buf, read, false);
		if (err == 0)
			return B_ERROR;
		read = fFrom->Read(buf, sizeof(buf));
	}
	// end of the parse
	CARD_Parse(fParser, NULL, 0, true);
	return B_OK;
}


bool
VCardParser::HasBegin()
{
	return fBegin && fCheck;
}


bool
VCardParser::HasEnd()
{
	return fEnd && fCheck;
}


int32
VCardParser::CountProperties()
{
	return fList.CountItems();
}


BContactField*
VCardParser::PropertyAt(int32 i)
{
	return fList.ItemAt(i);
}

	
BContactFieldList*
VCardParser::Properties()
{
	return &fList;
}


void
VCardParser::PropHandler(const CARD_Char* propName, const CARD_Char** params)
{
	if (!fBegin && strcasecmp(propName, "BEGIN") == 0) {
		fBegin = true;
        return;
	}

	// if we don't have a BEGIN:VCARD field
	// we just don't accept the following data.
    if (!fBegin)
        return;

	if (strcasecmp(propName, "END") == 0) {
		fEnd = true;
		return;
	}
	
	if (!fCheck)
		return;

	if (fOnlyCheck)
		return;

	printf("-----%s\n", propName);

	fLatestProp.SetTo(propName);

	fLatestParams.MakeEmpty();
	for (int i = 0; params[i] != NULL; i++) {	
		fLatestParams.AddItem(new BString(params[i]));
		if (params[i+1] == NULL)
			i++;
	}
}


void
VCardParser::DataHandler(const CARD_Char* data, int len)
{
	BString str(data, len);

	if (fBegin && !fCheck) {
		if (len > 0) {
			if (str.ICompare("VCARD") == 0)
				fCheck = true;
			return;
		}
	} else if (fEnd) {
		if (len > 0) {
			if (str.ICompare("VCARD") == 0)
				fCheck = true;
			else
				fCheck = false;
		}
		return;
	}

	if (fOnlyCheck)
		return;

	if (len == 0)
		return;

	str = "";
	for (int i = 0; i < len; i++) {
		CARD_Char c = data[i];
		if (c == '\r' || c == '\n')
			continue;
		else if (c >= ' ' && c <= '~')
			str.Append((char)c, 1);
	}

	field_type type = fFieldsMap.Get(HashString(fLatestProp));
	BContactField* field = BContactField::InstantiateChildClass(type);

	if (field != NULL) {
		_TranslateUsage(field);
		fList.AddItem(field);
		field->SetValue(str);
		//printf("data %s\n", field->Value().String());
	}
}


void
VCardParser::_TranslateUsage(BContactField* field) {
	int count = fLatestParams.CountItems();
	for (int i = 0; i < count; i++) {
		BString param = fLatestParams.ItemAt(i)->String();
		field_usage usage = fUsagesMap.Get(HashString(param));
		field->AddUsage(usage);
		//printf("----Param : %s\n", fLatestParams.ItemAt(i)->String());
	}
}
