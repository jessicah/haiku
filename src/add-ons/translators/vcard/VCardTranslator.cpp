#include "VCardTranslator.h"

#include <shared/AutoDeleter.h>
#include <ObjectList.h>

#include <new>
#include <stdio.h>
#include <syslog.h>

#include "VCardParser.h"


const char* kTranslatorName = "VCardTranslator";
const char* kTranslatorInfo = "Translator for VCard files";

#define VCARD_MIME_TYPE "text/x-vCard"
#define CONTACT_MIME_TYPE "application/x-hcontact"

static const translation_format sInputFormats[] = {
		{
		B_CONTACT_FORMAT,
		B_TRANSLATOR_CONTACT,
		IN_QUALITY,
		IN_CAPABILITY,
		CONTACT_MIME_TYPE,
		"Haiku binary contact"
	},
	{
		B_VCARD_FORMAT ,
		B_TRANSLATOR_CONTACT,
		IN_QUALITY,
		IN_CAPABILITY,
		VCARD_MIME_TYPE,
		"vCard Contact file"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
		{
		B_CONTACT_FORMAT,
		B_TRANSLATOR_CONTACT,
		OUT_QUALITY,
		OUT_CAPABILITY,
		CONTACT_MIME_TYPE,
		"Haiku binary contact"
	},
	{
		B_VCARD_FORMAT ,
		B_TRANSLATOR_CONTACT,
		OUT_QUALITY,
		OUT_CAPABILITY,
		VCARD_MIME_TYPE,
		"vCard Contact file"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
};

const uint32 kNumInputFormats = sizeof(sInputFormats) 
	/ sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats)
	/ sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings)
	/ sizeof(TranSetting);

// required by the BaseTranslator class
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new (std::nothrow) VCardTranslator();

	return NULL;
}


VCardTranslator::VCardTranslator()
	:
	BaseTranslator(kTranslatorName, kTranslatorInfo, VCARD_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats, sOutputFormats, kNumOutputFormats,
		"VCardTranslatorSettings", sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_CONTACT, B_VCARD_FORMAT)
{
	for (int i = 0; gFieldsMap[i].key != NULL; i++)
		fFieldsMap.Put(HashKey32<field_type>(gFieldsMap[i].type),
			BString(gFieldsMap[i].key));

	for (int i = 0; gUsagesMap[i].key != NULL; i++)
		fFieldsMap.Put(HashKey32<field_usage>(gUsagesMap[i].usage),
			BString(gUsagesMap[i].key));
}


VCardTranslator::~VCardTranslator()
{
}


status_t
VCardTranslator::Identify(BPositionIO* inSource,
	const translation_format* inFormat, BMessage* ioExtension,
	translator_info* outInfo, uint32 outType)
{
	if (!outType)
		outType = B_CONTACT_FORMAT;

	if (outType != B_CONTACT_FORMAT && outType != B_VCARD_FORMAT)
		return B_NO_TRANSLATOR;

	BMessage msg;
 	if (outType == B_VCARD_FORMAT && msg.Unflatten(inSource) == B_OK) {
 		msg.PrintToStream();
		outInfo->type = B_CONTACT_FORMAT;
		outInfo->group = B_TRANSLATOR_CONTACT;
		outInfo->quality = IN_QUALITY;
		outInfo->capability = IN_CAPABILITY;
		snprintf(outInfo->name, sizeof(outInfo->name), kTranslatorName);
		strcpy(outInfo->MIME, CONTACT_MIME_TYPE);
		return B_OK;
 	} else if (outType == B_CONTACT_FORMAT)
 		return _IdentifyVCard(inSource, outInfo);

	return B_NO_TRANSLATOR;
}


status_t
VCardTranslator::Translate(BPositionIO* inSource, const translator_info* info,
	BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
{
	if (!outType)
		outType = B_CONTACT_FORMAT;

	if (outType != B_CONTACT_FORMAT && outType != B_VCARD_FORMAT)
		return B_NO_TRANSLATOR;

	// add no translation
	BMessage msg;
 	if (outType == B_VCARD_FORMAT && msg.Unflatten(inSource) == B_OK)
		return TranslateContact(&msg, ioExtension, outDestination);
 	else if (outType == B_CONTACT_FORMAT)
		return TranslateVCard(inSource, ioExtension, outDestination);

	return B_ERROR;
}


status_t
VCardTranslator::TranslateContact(BMessage* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination)
{
	int32 count = 0;
	type_code code = B_CONTACT_FIELD_TYPE;

	_WriteBegin(outDestination);

	status_t ret = inSource->GetInfo(CONTACT_FIELD_IDENT, &code, &count);
	if (ret != B_OK)
		return ret;

	for (int i = 0; i < count; i++) {
		const void* data;
		ssize_t size;

		ret = inSource->FindData(CONTACT_FIELD_IDENT, code,
			i, &data, &size);

		BContactField* field = BContactField::UnflattenChildClass(data, size);
		if (field == NULL)
			return B_ERROR;

		field_type type = field->FieldType();
		BString divider = ";";
		if (type == B_CONTACT_IM) {
			_TranslateIM(field, outDestination);
			delete field;
			continue;
		}

		BString out
			= fFieldsMap.Get(HashKey32<field_type>());

		printf("%s\n", out.String());
		for (int i = 0; i < field->CountUsages(); i++) {
			HashKey32<field_usage> usage(field->GetUsage(i));
			out += ";";
			out += fFieldsMap.Get(usage);
		}
		_Write(outDestination, out, field->Value());
		delete field;
	}
	_WriteEnd(outDestination);
	return B_OK;
}


status_t
VCardTranslator::TranslateVCard(BPositionIO* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination)
{
	VCardParser parser(inSource);

	if (parser.Parse() != B_OK)
		return B_ERROR;

	BObjectList<BContactField>* list = parser.Properties();

	BMessage msg;
	int count = list->CountItems();
	for (int32 i = 0; i < count; i++) {
		BContactField* object = list->ItemAt(i);
		ssize_t size = object->FlattenedSize();
		void* buffer = new char[size];
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter deleter(buffer);

		status_t ret = object->Flatten(buffer, size);
		if (ret != B_OK)
			return ret;

		ret = msg.AddData(CONTACT_FIELD_IDENT,
			B_CONTACT_FIELD_TYPE, buffer, size, false);
	}

	outDestination->Seek(0, SEEK_SET);
	return msg.Flatten(outDestination);
}


status_t
VCardTranslator::_IdentifyVCard(BPositionIO* inSource,
	translator_info* outInfo)
{
	// TODO change it to specifically check
	// for BEGIN:VCARD and BEGIN:VCARD fields.
	// I.E. implement a custom ParseAndIdentify() method
	// that just don't use the parser and gain performances.

	// Using this initialization
	// the parse will only verify
	// that it's a VCard file
	// without saving any property
	// but still will waste resource (see the TODO)
	VCardParser parser(inSource, true);
	
	if (parser.Parse() != B_OK)
		return B_ERROR;

	if (parser.HasBegin() && parser.HasEnd()) {
		outInfo->type = B_VCARD_FORMAT;
		outInfo->group = B_TRANSLATOR_CONTACT;
		outInfo->quality = IN_QUALITY;
		outInfo->capability = IN_CAPABILITY;
		snprintf(outInfo->name, sizeof(outInfo->name), kTranslatorName);
		strcpy(outInfo->MIME, VCARD_MIME_TYPE);
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}


BView*
VCardTranslator::NewConfigView(TranslatorSettings *settings)
{
	return 	new VCardView(BRect(0, 0, 225, 175), 
		"VCardTranslator Settings", B_FOLLOW_ALL,
		B_WILL_DRAW, settings);
}


void
VCardTranslator::_WriteBegin(BPositionIO* dest)
{
	// TODO VCard 3/4 support
	dest->Seek(0, SEEK_SET);
	const char data[] = "BEGIN:VCARD\nVERSION:2.1\n";
	dest->Write(data, sizeof(data)-1);
}

void
VCardTranslator::_WriteEnd(BPositionIO* dest)
{
	const char data[] = "END:VCARD\n";
	dest->Write(data, sizeof(data)-1);	
}


void
VCardTranslator::_Write(BPositionIO* dest, BString& str, const BString& value)
{
	if (str.Length() > 0) {
		str << ":" << value << "\n";
		dest->Write(str.String(), str.Length());
	}
}


void
VCardTranslator::_TranslateIM(BContactField* field,
	BPositionIO* outDestination)
{
	BString out;
	for (int i = 0; i < field->CountUsages(); i++) {
		HashKey32<field_usage> usage(field->GetUsage(i));
		out += fFieldsMap.Get(usage);
		out += ":";
		_Write(outDestination, out, field->Value());
	}
}
