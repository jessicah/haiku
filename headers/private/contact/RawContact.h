/*
 * Copyright 2011-2012 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef B_RAW_CONTACT_H
#define B_RAW_CONTACT_H

#include <Archivable.h>
#include <ContactDefs.h>
#include <Message.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

class BRawContact : public virtual BArchivable {
public:
					// Initialize the RawContact from
					// a BMessage if it's valid.
					BRawContact(BMessage* data);

					BRawContact(uint32 finalFormat = B_CONTACT_FORMAT,
						BPositionIO* destination = NULL);

					// TODO add a entry_ref constructor

					~BRawContact();

	status_t 		Archive(BMessage* data, bool deep) const;

	static BArchivable*	Instantiate(BMessage* data);

	status_t		InitCheck() const;

	status_t		Commit(BMessage* data);
	status_t		Read(BMessage* data);
	int32			FinalFormat() const;

	BPositionIO*	Destination() const;
	status_t		SetDestination(BPositionIO* destination,
						bool autoDelete = true);
private:
	void			_Init();
	void			_InitTranslator();
	bool			_CheckDestination(BPositionIO* destination);
	status_t		_FindFormat();

	BPositionIO*	fDest;

	status_t		fInitCheck;
	// translation stuff
	uint32			fFormat;
	// the id of the suitable translator
	translator_id	fTranslatorID;

	BTranslatorRoster* fRoster;
};

#endif // B_RAW_CONTACT_H
