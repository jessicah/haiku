/*
 * Copyright 2011 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <ContactField.h>

#include <AutoDeleter.h>
#include <ContactDefs.h>
#include <DataIO.h>
#include <String.h>

#include <assert.h>
#include <stdio.h>

#include "ContactPrivate.h"

// TODO add decent debug to whole kit
struct EqualityVisitorBase : public BContactFieldVisitor {
	EqualityVisitorBase() : result(false) {}
	bool result;

	virtual void Visit(BStringContactField* field) {}
	virtual void Visit(BAddressContactField* field) {}
	virtual void Visit(BPhotoContactField* field) {}
};

struct CopyVisitorBase : public BContactFieldVisitor {
	CopyVisitorBase() : error(B_ERROR) {}
	status_t error;

	virtual void Visit(BStringContactField* field) {}
	virtual void Visit(BAddressContactField* field) {}
	virtual void Visit(BPhotoContactField* field) {}
};


BContactField::BContactField(field_type type, bool autoLabel)
	:
	fType(type),
	fUsages(true)
{
	if (autoLabel)
		fLabel = SimpleLabel(type);
}


BContactField::~BContactField()
{
}


bool
BContactField::IsEqual(BContactField* field)
{
	if (FieldType() != field->FieldType())
		return false;

	if (Label().Compare(field->Label()) != 0)
		return false;

	/*for (int i = 0; i < CountUsages(); i++)
		AddUsage(field->GetUsage(i));*/
	return true;
}


field_type
BContactField::FieldType() const
{
	return fType;
}


field_usage
BContactField::GetUsage(int32 i) const
{
	return *fUsages.ItemAt(i);
}


void
BContactField::AddUsage(field_usage usage)
{
	field_usage* newUsage = new field_usage;
	*newUsage = usage;
	fUsages.AddItem(newUsage);
}


int32
BContactField::CountUsages() const
{
	return fUsages.CountItems();
}


// This is basically a common way to get a generic label, without
// actually having a BContactField instantiated.
const char*
BContactField::SimpleLabel(field_type type)
{
	BString label = "Unknown";
	for (int i = 0; gStandardFields[i].type != 0; i++) {
		if (gStandardFields[i].type == type) {
			label.SetTo(gStandardFields[i].label);
			break;
		}
	}
	return label.String();
}


// this is used to load a label that explains field usage
const char*
BContactField::ExtendedLabel(BContactField* field)
{
	field_type type = field->FieldType();
	BString label = SimpleLabel(type);

	for (int i = 0; i < field->CountUsages(); i++) {
		field_usage usage = field->GetUsage(i);
		_UpdateLabel(usage, label);
	}
	return label.String();
}


const BString&
BContactField::Label() const
{
	return fLabel;
}


void
BContactField::SetLabel(const BString& label)
{
	fLabel.SetTo(label);
}


bool
BContactField::IsFixedSize() const
{ 
	return false;
}


type_code
BContactField::TypeCode() const
{
	return B_CONTACT_FIELD_TYPE;
}


bool
BContactField::AllowsTypeCode(type_code code) const
{
	if (code != B_CONTACT_FIELD_TYPE)
		return false;

	return true;
}


ssize_t
BContactField::FlattenedSize() const
{
	ssize_t size = sizeof(type_code);
	size += sizeof(int32);

	return size + sizeof(field_usage)*fUsages.CountItems();
}


status_t
BContactField::Flatten(BPositionIO* flatData) const
{
	if (flatData == NULL)
		return B_BAD_VALUE;

	// TODO ADD ENDIANESS CODE

	// NOTE this is needed because we need to know
	// whitch derived class we need to instantiate
	// in BContactField::UnflattenChildClass()
	flatData->Write(&fType, sizeof(type_code));

	int32 count = fUsages.CountItems();
	flatData->Write(&count, sizeof(count));

	for (int i = 0; i < count; i++)
		flatData->Write(fUsages.ItemAt(i), sizeof(field_usage));

	return B_OK;
}


status_t
BContactField::Flatten(void* buffer, ssize_t size) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Flatten(&flatData, size);
}


status_t
BContactField::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Unflatten(code, &flatData);
}


status_t
BContactField::Unflatten(type_code code, BPositionIO* flatData)
{
	if (code != B_CONTACT_FIELD_TYPE)
		return B_BAD_VALUE;

	// read the type of the field	
	flatData->Read(&fType, sizeof(type_code));

	int32 count = 0;
	flatData->Read(&count, sizeof(int32));

	if (count == 0)
		return B_OK;

	for (int32 i = 0; i < count; i++) {
		field_usage* type = new field_usage;
		flatData->Read(type, sizeof(field_usage));
		fUsages.AddItem(type);
	}
	return B_OK;
}


// You cannot instantiate a pure BContactField
// so this method help to unflatten the correct 
// derived class
BContactField*
BContactField::UnflattenChildClass(const void* from, ssize_t size)
{
	BMemoryIO data(from, size);

	type_code childType;
	data.Read(&childType, sizeof(childType));
	ObjectDeleter<BContactField> deleter;
	BContactField* child = NULL;

	child = BContactField::InstantiateChildClass(childType);

	if (child == NULL)
		return NULL;

	status_t ret = child->Unflatten(B_CONTACT_FIELD_TYPE, from, size);
	if (ret == B_OK)
		return child;

	deleter.SetTo(child);
	return NULL;
}


status_t
BContactField::CopyDataFrom(BContactField* field)
{
	if (FieldType() != field->FieldType())
		return B_ERROR;

	SetLabel(field->Label());

	for (int i = 0; i < field->CountUsages(); i++)
		AddUsage(field->GetUsage(i));

	return B_OK;
}


BContactField*
BContactField::Duplicate(BContactField* from)
{
	BContactField* child = NULL;
	type_code childType = from->FieldType();
	ObjectDeleter<BContactField> deleter;

	child = BContactField::InstantiateChildClass(childType);

	if (child != NULL && child->CopyDataFrom(from) == B_OK)
		return child;

	deleter.SetTo(child);
	return NULL;
}


BContactField*
BContactField::InstantiateChildClass(type_code type)
{
	BContactField* child = NULL;
	switch (type) {
		case B_CONTACT_ASSISTANT:
		case B_CONTACT_BIRTHDAY:
		case B_CONTACT_EMAIL:
		case B_CONTACT_FORMATTED_NAME:
		case B_CONTACT_GENDER:
		case B_CONTACT_GEO:
		case B_CONTACT_GROUP:
		case B_CONTACT_IM:
		case B_CONTACT_MANAGER:
		case B_CONTACT_NAME:
		case B_CONTACT_NICKNAME:
		case B_CONTACT_NOTE:
		case B_CONTACT_ORGANIZATION:
		case B_CONTACT_PHONE:
		case B_CONTACT_PROTOCOLS:
		case B_CONTACT_SIMPLE_GROUP:
		case B_CONTACT_SOUND:
		case B_CONTACT_SPOUSE:
		case B_CONTACT_TIME_ZONE:
		case B_CONTACT_TITLE:
		case B_CONTACT_UID:
		case B_CONTACT_URL:
		case B_CONTACT_REV:
			child = new BStringContactField(type);
			break;

		case B_CONTACT_DELIVERY_LABEL:
		case B_CONTACT_ADDRESS:
			child = new BAddressContactField(type);
			break;

		case B_CONTACT_PHOTO:
			child = new BPhotoContactField();
			break;

		//case B_CONTACT_CUSTOM:
		//	child = new BCustomContactField();
		//	break;

		default:
			return NULL;
	}

	if (child == NULL)
		return NULL;

	return child;
}


bool
BContactField::IsHidden() const
{
	return false;
}


ssize_t
BContactField::_AddStringToBuffer(BPositionIO* buffer, const BString& str) const
{
	ssize_t valueLength = str.Length();
	if (valueLength > 0) {
		ssize_t ret = buffer->Write(&valueLength, sizeof(valueLength));
		return ret + buffer->Write(str, valueLength);
	} else
		return buffer->Write(0, sizeof(ssize_t));

	return -1;
}


BString
BContactField::_ReadStringFromBuffer(BPositionIO* buffer, ssize_t len)
{
	BString ret;
	if (len == -1)
		buffer->Read(&len, sizeof(len));

	char* valueBuffer;
	if (len > 0 && len < 1024) {
		ArrayDeleter<char> deleter(valueBuffer = new char[len]);
		buffer->Read(valueBuffer, len);
		ret = BString(valueBuffer, len);
	} else
		ret = BString();

	return ret;
}


void
BContactField::_UpdateLabel(field_usage usage, BString& str)
{
	for (int i = 0; gStandardUsages[i].usage != 0; i++) {
		standardUsagesMap item = gStandardUsages[i];
		if (usage == item.usage) {
			switch (item.stringOp) {
				case 0:
				break;

				case 1:
					str.Prepend(item.label);				
				break;

				case 2:
					str.Append(item.label);
				break;
				case 3:
					if (strlen(item.replaceString) < 1)
						break;

					str.Replace(item.replaceString, item.label, 1);
				break;	
			}
		}
	}
}

// BStringContactField

BStringContactField::BStringContactField(type_code type, const BString& str)   
	:
	BContactField(type),
	fValue(str)
{
}


BStringContactField::BStringContactField(type_code type, const char* str) 
	:
	BContactField(type),
	fValue(str)
{
}


BStringContactField::~BStringContactField()
{	
}


struct BStringContactField::CopyVisitor : public CopyVisitorBase {

	BStringContactField* fOwner;

	CopyVisitor(BStringContactField* owner)
		:
		CopyVisitorBase(),
		fOwner(owner)
		{
		}

	virtual void Visit(BStringContactField* field)
	{
		fOwner->SetValue(field->Value());
		error = B_OK;
	}

	virtual void Visit(BAddressContactField* field)
	{
	}

	virtual void Visit(BPhotoContactField* field)
	{
	}
};


struct BStringContactField::EqualityVisitor : public EqualityVisitorBase {

	BStringContactField* fOwner;

	EqualityVisitor(BStringContactField* owner)
		:
		EqualityVisitorBase(),
		fOwner(owner) 
		{
		}

	virtual void Visit(BStringContactField* field)
	{
		if (field->Value().Compare(fOwner->Value()) == 0)
			result = true;
	}

	virtual void Visit(BAddressContactField* field)
	{
	}

	virtual void Visit(BPhotoContactField* field)
	{
	}
};


void
BStringContactField::Accept(BContactFieldVisitor* v)
{ 
	v->Visit(this);
}


bool
BStringContactField::IsEqual(BContactField* field)
{
	if (BContactField::IsEqual(field) == false)
		return false;

	BStringContactField::EqualityVisitor equalityChecker(this);
	field->Accept(&equalityChecker);
	return equalityChecker.result;
}


void
BStringContactField::SetValue(const BString& value)
{
	fValue.SetTo(value);	
}


const BString&
BStringContactField::Value() const
{
	return fValue;
}


// this method should take a BStringContactField
status_t
BStringContactField::CopyDataFrom(BContactField* field)
{
	status_t ret = BContactField::CopyDataFrom(field);
	if (ret != B_OK)
		return ret;

	BStringContactField::CopyVisitor copier(this);
	field->Accept(&copier);
	return copier.error;
}


ssize_t
BStringContactField::FlattenedSize() const
{
	ssize_t size = BContactField::FlattenedSize();
	size += sizeof(ssize_t);
	size += fValue.Length();

	return size;
}


status_t
BStringContactField::Flatten(void* buffer, ssize_t size) const
{
	if (size < FlattenedSize())
		return B_ERROR;

	BMemoryIO flatData(buffer, size);
	status_t ret = BContactField::Flatten(&flatData);
	if (ret != B_OK)
		return ret;

	_AddStringToBuffer(&flatData, fValue);

	return B_OK;
}


status_t
BStringContactField::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	BMemoryIO data(buffer, size);
	status_t ret = BContactField::Unflatten(code, &data);
	if (ret != B_OK)
		return ret;

	fValue = _ReadStringFromBuffer(&data);

	return B_OK;
}

/** BAddressContactField **/

struct BAddressContactField::CopyVisitor : public CopyVisitorBase {

	BAddressContactField* fOwner;

	CopyVisitor(BAddressContactField* owner)
		:
		CopyVisitorBase(),
		fOwner(owner)
		{
		}

	virtual void Visit(BStringContactField* field)
	{
	}

	virtual void Visit(BAddressContactField* field)
	{
		fOwner->SetValue(field->Value());
		error = B_OK;
	}

	virtual void Visit(BPhotoContactField* field)
	{
	}
};


struct BAddressContactField::EqualityVisitor : public EqualityVisitorBase {

	BAddressContactField* fOwner;

	EqualityVisitor(BAddressContactField* owner)
		:
		EqualityVisitorBase(),
		fOwner(owner) 
		{
		}

	virtual void Visit(BStringContactField* field)
	{
	}

	virtual void Visit(BAddressContactField* field)
	{
		if (field->Value().Compare(fOwner->Value()) == 0)
			result = true;
	}

	virtual void Visit(BPhotoContactField* field)
	{
	}
};


BAddressContactField::BAddressContactField(field_type type, BString address)
  	:
	BContactField(type),
	fDivider(";")
{
	if (type == B_CONTACT_DELIVERY_LABEL)
		fDivider = ",";

	_SplitValue(address);
}


BAddressContactField::~BAddressContactField()
{
}


void
BAddressContactField::Accept(BContactFieldVisitor* v)
{
	v->Visit(this);
}


bool
BAddressContactField::IsEqual(BContactField* field)
{
	if (BContactField::IsEqual(field) == false)
		return false;

	BAddressContactField::EqualityVisitor equalityChecker(this);
	field->Accept(&equalityChecker);
	return equalityChecker.result;
}


void
BAddressContactField::SetValue(const BString& value)
{
	_SplitValue(value);
}


const BString&
BAddressContactField::Value() const
{
	// this is returning the address
	// as described in the vcard-21 spec

	if (!fValue.IsEmpty())
		return fValue;

	fValue << fPostalBox << ";" << fNeighbor << ";";
	fValue << fStreet << ";" << fCity << ";" << fRegion;
	fValue << ";" << fPostalCode << ";" << fCountry << ";"; 

	return fValue;
}


status_t
BAddressContactField::CopyDataFrom(BContactField* field)
{
	status_t ret = BContactField::CopyDataFrom(field);
	if (ret != B_OK)
		return ret;

	BAddressContactField::CopyVisitor copier(this);
	field->Accept(&copier);
	return copier.error;
}


bool
BAddressContactField::IsDeliveryLabel() const
{
	return fType == B_CONTACT_DELIVERY_LABEL;	
}

/*
// TODO maybe choose a better name for this method.
void
BAddressContactField::SetDeliveryLabel(bool isLabel)
{
	if (isLabel == true) {
		fType = B_CONTACT_DELIVERY_LABEL;
		fDivider = ",";
	} else {
		fDivider = ";";
		fType = B_CONTACT_ADDRESS;
	}
}*/


const BString&
BAddressContactField::Street() const
{
	return fStreet;
}


const BString&
BAddressContactField::PostalBox() const
{
	return fPostalBox;
}


const BString&
BAddressContactField::Neighborhood() const
{
	return fNeighbor;
}


const BString&
BAddressContactField::City() const
{
	return fCity;
}


const BString&
BAddressContactField::Region() const
{
	return fRegion;
}


const BString&
BAddressContactField::PostalCode() const
{
	return fPostalCode;
}


const BString&
BAddressContactField::Country() const
{
	return fCountry;
}


void
BAddressContactField::SetStreet(const BString& street)
{
	fValue = "";
	fStreet.SetTo(street);
}


void
BAddressContactField::SetPostalBox(const BString& postBox)
{
	fValue = "";
	fPostalBox.SetTo(postBox);
}


void
BAddressContactField::SetNeighborhood(const BString& neighbor)
{
	fValue = "";
	fNeighbor.SetTo(neighbor);
}


void
BAddressContactField::SetCity(const BString& city)
{
	fValue = "";
	fCity.SetTo(city);
}


void
BAddressContactField::SetRegion(const BString& region)
{
	fValue = "";
	fRegion.SetTo(region);
}


void
BAddressContactField::SetPostalCode(const BString& postalCode)
{
	fValue = "";
	fPostalCode.SetTo(postalCode);
}


void
BAddressContactField::SetCountry(const BString& country)
{
	fValue = "";
	fCountry.SetTo(country);
}


ssize_t
BAddressContactField::FlattenedSize() const
{
	ssize_t size = BContactField::FlattenedSize();
	size += sizeof(ssize_t);
	return size + Value().Length();
}


status_t
BAddressContactField::Flatten(void* buffer, ssize_t size) const
{
	if (size < FlattenedSize())
		return B_ERROR;

	BMemoryIO flatData(buffer, size);
	status_t ret = BContactField::Flatten(&flatData);
	if (ret != B_OK)
		return ret;

	_AddStringToBuffer(&flatData, Value());

	return B_OK;
}


status_t
BAddressContactField::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	BMemoryIO data(buffer, size);
	status_t ret = BContactField::Unflatten(code, &data);
	if (ret != B_OK)
		return ret;

	SetValue(_ReadStringFromBuffer(&data));

	return B_OK;
}


bool
BAddressContactField::_SplitValue(const BString& str)
{
	BString value(str);
	_PopValue(value, fPostalBox);
	_PopValue(value, fNeighbor);
	_PopValue(value, fStreet);
	_PopValue(value, fCity);
	_PopValue(value, fRegion);
	_PopValue(value, fPostalCode);
	_PopValue(value, fCountry);
	return true;
}


void
BAddressContactField::_PopValue(BString& str, BString& value)
{
	int32 index = str.FindFirst(fDivider, 0);
	printf("%s\n", str.String());
	if (index == B_ERROR && str.Length() < 1) {
		value.SetTo("");
		return;
	}
	str.MoveInto(value, 0, index);
	str.Remove(0,1);
}

/** BPhotoContactField */

// TODO add support for refs and urls photos, then
// fix the visitors, to allow fields compare and copy

struct BPhotoContactField::CopyVisitor : public CopyVisitorBase {

	BPhotoContactField* fOwner;

	CopyVisitor(BPhotoContactField* owner)
		:
		CopyVisitorBase(),
		fOwner(owner)
		{
		}

	virtual void Visit(BStringContactField* field)
	{
	}

	virtual void Visit(BAddressContactField* field)
	{
	}

	virtual void Visit(BPhotoContactField* field)
	{
		error = B_OK;
	}
};


struct BPhotoContactField::EqualityVisitor : public EqualityVisitorBase {

	BPhotoContactField* fOwner;

	EqualityVisitor(BPhotoContactField* owner)
		:
		EqualityVisitorBase(),
		fOwner(owner) 
		{
		}

	virtual void Visit(BStringContactField* field)
	{
	}

	virtual void Visit(BAddressContactField* field)
	{
	}


	virtual void Visit(BPhotoContactField* field)
	{
		/*if (fOwner->RefToPhoto() == field->RefToPhoto())
			result = true;*/

		/*if (field->Value().Compare(fOwner->Value()) == 0)
			result = true;*/
		result = B_OK;
	}
};


BPhotoContactField::BPhotoContactField(BBitmap* bitmap)   
	:
	BContactField(B_CONTACT_PHOTO),
	fBitmap(bitmap),
	fUrl(),
	fPhotoType(CONTACT_PHOTO_BITMAP),
	fPictureType(0)
{
}

/*
BPhotoContactField::BPhotoContactField(const char* url)   
	:
	BContactField(B_CONTACT_PHOTO),
	fPhotoType(CONTACT_PHOTO_URL)
{

}
*/

BPhotoContactField::~BPhotoContactField()
{
	_CleanUp();	
}


void
BPhotoContactField::Accept(BContactFieldVisitor* v)
{ 
	v->Visit(this);
}


bool
BPhotoContactField::IsEqual(BContactField* field)
{
	BPhotoContactField::EqualityVisitor equalityChecker(this);
	field->Accept(&equalityChecker);
	return equalityChecker.result;
}


BBitmap*
BPhotoContactField::Photo() const
{
	if (fBitmap)
		return fBitmap;

	return NULL;
}


void
BPhotoContactField::SetPhoto(BBitmap* photo)
{
	_CleanUp();
	fBitmap = photo;
}


void
BPhotoContactField::SetValue(const BString& value)
{
	fUrl.SetTo(value);	
}


const BString&
BPhotoContactField::Value() const
{
	return fUrl;
}


uint32
BPhotoContactField::PictureType() const
{
	return fPictureType;
}


void
BPhotoContactField::SetPictureType(uint32 type)
{
	fPictureType = type;
}


const BString&
BPhotoContactField::PictureMIME() const
{
	return fPictureMIME;
}


void
BPhotoContactField::SetPictureMIME(const BString& mime)
{
	fPictureMIME.SetTo(mime);
}


status_t
BPhotoContactField::CopyDataFrom(BContactField* field)
{
	status_t ret = BContactField::CopyDataFrom(field);
	if (ret != B_OK)
		return ret;

	BPhotoContactField::CopyVisitor copier(this);
	field->Accept(&copier);
	return copier.error;
}


ssize_t
BPhotoContactField::FlattenedSize() const
{
	ssize_t size = BContactField::FlattenedSize();

	if (fBitmap) {
		BMessage msg;
		fBitmap->Archive(&msg);
		size += msg.FlattenedSize();
		size += sizeof(ssize_t);
	}
	return size;
}


status_t
BPhotoContactField::Flatten(void* buffer, ssize_t size) const
{
	if (size < FlattenedSize())
		return B_ERROR;

	BMemoryIO flatData(buffer, size);
	status_t ret = BContactField::Flatten(&flatData);
	if (ret != B_OK)
		return ret;

	ssize_t destSize;

	if (fBitmap != NULL) {
		BMessage msg;
		BMallocIO dest;

		fBitmap->Archive(&msg);
		destSize = msg.FlattenedSize();
		msg.Flatten(&dest);

		flatData.Write(&destSize, sizeof(destSize));
		flatData.Write(dest.Buffer(), destSize);
	} else {
		size = 0;
		flatData.Write(&size, sizeof(destSize));
	}

	return B_OK;
}


status_t
BPhotoContactField::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	BMemoryIO data(buffer, size);
	status_t ret = BContactField::Unflatten(code, &data);
	if (ret != B_OK)
		return ret;

	BMessage msg;
	ssize_t destSize;
	data.Read(&destSize, sizeof(destSize));

	if (destSize < 1) {
		fBitmap = NULL;
		return B_OK;
	}

	void* buf = malloc(destSize);
	MemoryDeleter deleter(buf);
	data.Read(buf, destSize);

	BMemoryIO dest(buf, destSize);
	msg.Unflatten(&dest);

	fBitmap = new BBitmap(&msg);

	return fBitmap->InitCheck();
}


void
BPhotoContactField::_CleanUp()
{
	delete fBitmap;
}
