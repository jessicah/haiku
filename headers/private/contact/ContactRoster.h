/*
 * Copyright 2011 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef CONTACT_ROSTER_H
#define CONTACT_ROSTER_H

#include <AddressBook.h>
#include <Contact.h>
#include <ContactGroup.h>

// Basically, i think this will supply support functions
// and support to register BContacts over the system.

// The AddressBook will be a special group, that have their objects
// automatically published in /boot/home/peole. I don't think any other
// group should be allowed here (you can add a contact to multiple groups anyway)

class BContactRoster {
public:
						BContactRoster();

	status_t			RegisterContact(BContactRef& contact);
	status_t			UnregisterContact(BContactRef& contact);

	// if a contact isn't registered the following will return NULL.
	BContact*			InstantiateContact(BContactRef& ref);
	BContactGroup*		InstantiateGroup(uint32 id);

	BContactGroupList*	GroupsForRef(BContactRef& ref);

	BContactGroupList*	RegisteredGroups();

//	BContactRefList*	ContactsByQuery(BContactQuery* query);

	BAddressBook*		AddressBook();

private:

};

#endif // CONTACT_ROSTER_H
