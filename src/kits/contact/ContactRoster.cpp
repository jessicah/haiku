/*
 * Copyright 2010 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <ContactRoster.h>

#include <FindDirectory.h>

BContactRoster::BContactRoster()
{
}


status_t
BContactRoster::RegisterContact(BContactRef& contact)
{

	return B_ERROR;
}


status_t
BContactRoster::UnregisterContact(BContactRef& contact)
{

	return B_ERROR;
}


BContact*
BContactRoster::InstantiateContact(BContactRef& ref)
{
	
	return NULL;
}


BContactGroup*
BContactRoster::InstantiateGroup(uint32 id)
{
	
	return NULL;
}


BContactGroupList*
BContactRoster::RegisteredGroups()
{

	return NULL;
}


BContactGroupList*
BContactRoster::GroupsForRef(BContactRef& ref)
{

	return NULL;
}


BAddressBook*
BContactRoster::AddressBook()
{
	return new BAddressBook();
}
