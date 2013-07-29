/*
 * Copyright 2011-2012 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _CONTACT_GROUP_H
#define _CONTACT_GROUP_H

#include <ContactDefs.h>
#include <ContactRef.h>
#include <Flattenable.h>
#include <Message.h>
#include <SupportDefs.h>

enum {
	B_CONTACT_GROUP_TYPE = 'CNGT'
};


class BContactGroup : public BFlattenable {
public:
							BContactGroup(
								uint32 groupID = B_CONTACT_GROUP_NONE,
								bool custom = false);

							~BContactGroup();

	virtual	bool			IsFixedSize() const;
	virtual	type_code		TypeCode() const;
	virtual	bool			AllowsTypeCode(type_code code) const;
	virtual	ssize_t			FlattenedSize() const;

			status_t 		Flatten(BPositionIO* flatData) const;
	virtual	status_t		Flatten(void* buffer, ssize_t size) const;
	virtual	status_t		Unflatten(type_code code, const void* buffer,
								ssize_t size);
			status_t		Unflatten(type_code code, BPositionIO* flatData);

			status_t		InitCheck() const;

	virtual status_t		AddContact(BContactRef contact);
	virtual status_t		RemoveContact(BContactRef contact);
	virtual int32			CountContacts() const;
	virtual BContactRef		ContactAt(int32 index) const;

			const BContactRefList AllContacts() const;

			/*BContactList*	ContactsByQuery(BContactQuery* query);
			const BContactRefList* ContactsByField(ContactFieldType type,
				const char* value = NULL) const;*/
protected:
			BContactRefList fList;
private:
			status_t		fInitCheck;
			uint32			fGroupID;
			bool			fCustom;
};

typedef BObjectList<BContactGroup> BContactGroupList;

#endif // _CONTACT_GROUP_H
