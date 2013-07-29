/*
 * Copyright 2011-2012 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _VCARD_PARSER_H
#define _VCARD_PARSER_H

#include <ContactDefs.h>
#include <ContactField.h>

#include <ObjectList.h>
#include <SupportDefs.h>

#include "cardparser.h"
#include "VCardParserDefs.h"

typedef BObjectList<BString> StringList;

class VCardParser {
public:
					VCardParser(BPositionIO* from, bool check = false);
	virtual			~VCardParser();

	status_t		Parse();

	bool			HasBegin();
	bool			HasEnd();

	const char*		Version();

	void 			PropHandler(const CARD_Char* propName,
						const CARD_Char** params);
	void 			DataHandler(const CARD_Char* data, int len);

	int32			CountProperties();
	BContactField*	PropertyAt(int32 i);
	// rename Fields
	BContactFieldList* Properties();
private:
	void			_TranslateUsage(BContactField* field);

	CARD_Parser 	fParser;
	BPositionIO*	fFrom;

	bool			fOnlyCheck;
	bool 			fCheck;
	bool 			fBegin;
	bool 			fEnd;
	BString 		fLatestProp;
	StringList		fLatestParams;

	BContactFieldList fList;

	InFieldsMap		fFieldsMap;
	InUsagesMap		fUsagesMap;
};

#endif // _H
