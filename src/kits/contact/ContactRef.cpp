/*
 * Copyright 2011 - 2012 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <ContactRef.h>

#include <string.h>


BContactRef::BContactRef(int32 id, uint32 groupID, bool autoFill)
	:
	fContactID(id),
	fGroupID(groupID)
{
}


BContactRef::BContactRef(const BContactRef& ref)
	:
	fName(ref.fName),
	fNickname(ref.fNickname),
	fEmail(ref.fEmail),
	fContactID(ref.fContactID),
	fGroupID(ref.fGroupID)
{
}


BContactRef::~BContactRef()
{
}


bool
BContactRef::IsFixedSize() const
{ 
	return false;
}


type_code
BContactRef::TypeCode() const
{
	return B_CONTACT_REF_TYPE;
}


bool
BContactRef::AllowsTypeCode(type_code code) const
{
	if (code != B_CONTACT_REF_TYPE)
		return false;
	
	return true;
}


ssize_t
BContactRef::FlattenedSize() const
{
	size_t size = fName.Length();
	size += fNickname.Length();
	size += fEmail.Length();
	size += sizeof(size_t)*3;

	size += sizeof(fContactID);
	size += sizeof(fGroupID);

	return 0;
}


status_t
BContactRef::Flatten(BPositionIO* flatData) const
{
	if (flatData == NULL)
		return B_BAD_VALUE;

	// TODO ADD ENDIANESS CODE

	_AddStringToBuffer(flatData, fName.String());
	_AddStringToBuffer(flatData, fNickname.String());
	_AddStringToBuffer(flatData, fEmail.String());
	flatData->Write(&fContactID, sizeof(fContactID));
	flatData->Write(&fGroupID, sizeof(fGroupID));

	return B_OK;
}


status_t
BContactRef::Flatten(void* buffer, ssize_t size) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Flatten(&flatData, size);
}


status_t
BContactRef::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Unflatten(code, &flatData);
}


status_t
BContactRef::Unflatten(type_code code, BPositionIO* flatData)
{
	if (code != B_CONTACT_REF_TYPE)
		return B_BAD_VALUE;

	fName = _ReadStringFromBuffer(flatData);
	fNickname = _ReadStringFromBuffer(flatData);
	fEmail = _ReadStringFromBuffer(flatData);
	flatData->Read(&fContactID, sizeof(fContactID));
	flatData->Read(&fGroupID, sizeof(fGroupID));

	return B_OK;
}


ssize_t
BContactRef::_AddStringToBuffer(BPositionIO* buffer, const char* str) const
{
	ssize_t valueLength = strlen(str);
	if (valueLength > 0) {
		ssize_t ret = buffer->Write(&valueLength, sizeof(valueLength));
		return ret + buffer->Write(str, valueLength);
	} else
		return buffer->Write(0, sizeof(ssize_t));

	return -1;
}


const char*
BContactRef::_ReadStringFromBuffer(BPositionIO* buffer, ssize_t len)
{
	if (len == -1)
		buffer->Read(&len, sizeof(len));

	char* valueBuffer;
	valueBuffer = new char[len];
	if (len != 0)
		buffer->Read(valueBuffer, len);

	return valueBuffer; 
}


bool
BContactRef::IsEqual(const BContactRef& ref) const
{
	if (fContactID != ref.fContactID || fGroupID != ref.fGroupID)
		return false;

	if (fName != ref.fName || fNickname != ref.fNickname
		|| fEmail != ref.fEmail)
		return false;

	return true;
}
