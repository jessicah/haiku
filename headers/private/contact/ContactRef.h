/*
 * Copyright 2011 - 2012 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _CONTACT_REF_H
#define _CONTACT_REF_H

#include <ContactDefs.h>
#include <DataIO.h>
#include <Flattenable.h>
#include <ObjectList.h>
#include <String.h>

enum {
	B_CONTACT_REF_TYPE = 'CRft'
};

class BContactRef : public virtual BFlattenable {
public:
						BContactRef(int32 id = -1,
							uint32 groupID = B_CONTACT_GROUP_NONE,
							bool autoFill = false);

						BContactRef(const BContactRef& ref);

						~BContactRef();

	virtual	bool		IsFixedSize() const;
	virtual	type_code	TypeCode() const;
	virtual	bool		AllowsTypeCode(type_code code) const;
	virtual	ssize_t		FlattenedSize() const;

			status_t 	Flatten(BPositionIO* flatData) const;
	virtual	status_t	Flatten(void* buffer, ssize_t size) const;
	virtual	status_t	Unflatten(type_code code, const void* buffer,
							ssize_t size);
			status_t	Unflatten(type_code code, BPositionIO* flatData);

			void		SetName(const char* name);
			void		SetNickname(const char* nickname);
			void		SetEmail(const char* email);

			const char* GetName();
			const char* GetNickname();
			const char* GetEmail();
			int32		GetID();
			uint32		GetGroupID();

			bool		IsEqual(const BContactRef& ref) const;
private:
			ssize_t		_AddStringToBuffer(BPositionIO* buffer,
							const char* str) const;
			const char* _ReadStringFromBuffer(BPositionIO* buffer,
							ssize_t len = -1);

			BString		fName;
			BString		fNickname;
			BString		fEmail;
			int32 		fContactID;
			uint32		fGroupID;
};

// TODO compositing instead of inheriting

typedef BObjectList<BContactRef> BContactRefList;

#endif
