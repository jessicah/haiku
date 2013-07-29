/*
 * Copyright 2011 - 2012 Dario Casalinuovo.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <ContactGroup.h>

#include <Contact.h>

//TODO define error codes

BContactGroup::BContactGroup(uint32 groupID, bool custom)
	:
	fList(true),
	fInitCheck(B_OK),
	fGroupID(groupID),
	fCustom(custom)
{

}


BContactGroup::~BContactGroup()
{
}



bool
BContactGroup::IsFixedSize() const
{ 
	return false;
}


type_code
BContactGroup::TypeCode() const
{
	return B_CONTACT_GROUP_TYPE;
}


bool
BContactGroup::AllowsTypeCode(type_code code) const
{
	if (code != B_CONTACT_GROUP_TYPE)
		return false;
	
	return true;
}


ssize_t
BContactGroup::FlattenedSize() const
{
	ssize_t size = sizeof(int);

	int count = fList.CountItems();
	size += sizeof(ssize_t)*count;

	for (int i = 0; i < count; i++)
		size += fList.ItemAt(i)->FlattenedSize();

	return size;
}


status_t
BContactGroup::Flatten(BPositionIO* flatData) const
{
	if (flatData == NULL)
		return B_BAD_VALUE;

	int count = fList.CountItems();
	flatData->Write(&count, sizeof(count));
	for (int i = 0; i < count; i++) {
		BContactRef* item = fList.ItemAt(i);
		ssize_t size = item->FlattenedSize();
		flatData->Write(&size, sizeof(size));
		if (size == 0)
			return B_OK;

		status_t ret = item->Flatten(flatData);
		if (ret != B_OK)
			return ret;
	}
	return B_OK;
}


status_t
BContactGroup::Unflatten(type_code code, BPositionIO* flatData)
{
	if (code != B_CONTACT_FIELD_TYPE)
		return B_BAD_VALUE;

	int count = 0;
	flatData->Read(&count, sizeof(count));
	for (int i = 0; i < count; i++) {
		ssize_t size = 0;
		flatData->Read(&size, sizeof(ssize_t));
 		if (size > 0) {
			BContactRef* item = new BContactRef();
			status_t ret = item->Unflatten(B_CONTACT_REF_TYPE, flatData);
			if (ret == B_OK)
				fList.AddItem(item);
			else {
				fInitCheck = ret;
				return ret;
			}
 		}
	}
	return B_OK;
}


status_t
BContactGroup::Flatten(void* buffer, ssize_t size) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Flatten(&flatData, size);
}


status_t
BContactGroup::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Unflatten(code, &flatData);
}


status_t
BContactGroup::InitCheck() const
{
	return fInitCheck;
}


status_t
BContactGroup::AddContact(BContactRef contact)
{
	BContactRef* ref = new BContactRef(contact);
	fList.AddItem(ref);
	return B_OK;
}


status_t
BContactGroup::RemoveContact(BContactRef contact)
{
	for (int i = 0; i < fList.CountItems(); i++) {
		if (fList.ItemAt(i)->IsEqual(contact))
			fList.RemoveItemAt(i);
	}
	return B_OK;
}


int32
BContactGroup::CountContacts() const
{
	return fList.CountItems();
}


BContactRef
BContactGroup::ContactAt(int32 index) const
{
	return *fList.ItemAt(index);
}


const
BContactRefList
BContactGroup::AllContacts() const
{
	return fList;
}


/*
const
BContactRefList&
BContactGroup::ContactsByField(ContactFieldType type, 
	const char* value) const
{
	BContactRefList* ret = new BContactRefList();

	return *ret;
}*/
