/*
 * Copyright 2011-2012 Casalinuovo Dario
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AddressBook.h"

#include <shared/AutoDeleter.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include <stdio.h>

// This class should be refactored

BAddressBook::BAddressBook()
	:
	BContactGroup(B_CONTACT_GROUP_ADDRESS_BOOK, true)
{
	fInitCheck = _FindDir();
}


BAddressBook::~BAddressBook()
{
}


status_t
BAddressBook::InitCheck()
{
	if (fInitCheck == B_OK)
		return BContactGroup::InitCheck();
	else
		return fInitCheck;
}


BPath
BAddressBook::GetPath()
{
	return fAddrBook;
}


status_t
BAddressBook::AddContact(BContactRef contact)
{
/*	if (fInitCheck != B_OK)
		return fInitCheck;

	BPath path = fAddrBook;

	if (filename == NULL) {
		BStringContactField* field 
			= (BStringContactField*)contact->FieldAt(B_CONTACT_NAME);

		if (field != NULL) {
			path.Append(field->Value());
		} else {
			return B_ERROR;
		}
	} else {
		path.Append(filename);
	}

	BContact* dest = new BContact(new BRawContact(B_PERSON_FORMAT,
		new BFile(path.Path(), B_READ_WRITE)));

	//ObjectDeleter<BContact> deleter(dest);

	int32 count = contact->CountFields();	
	for (int i = 0; i < count; i++)
		dest->AddField(contact->FieldAt(i));

	dest->Commit();

	if (fAddrList)
		fAddrList->AddItem(dest);
*/

	BContactGroup::AddContact(contact);
	return B_OK;

}


status_t
BAddressBook::RemoveContact(BContactRef contact)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	BContactGroup::RemoveContact(contact);
	return B_OK;
}


status_t
BAddressBook::_FindDir()
{
	status_t ret = find_directory(B_USER_DIRECTORY,
		&fAddrBook, false, NULL);
	if (ret != B_OK)
		return ret;

	ret = fAddrBook.InitCheck();
	if (ret != B_OK)
		return ret;

	fAddrBook.Append("people");

	printf("%s\n", fAddrBook.Path());
	return B_OK;
}
