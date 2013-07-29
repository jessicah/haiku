/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Dario Casalinuovo
 * 
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef CONTACT_FIELD_TEXT_CONTROL_H
#define CONTACT_FIELD_TEXT_CONTROL_H

#include <ContactDefs.h>
#include <ContactField.h>
#include <String.h>
#include <TextControl.h>

class ContactFieldTextControl : public BTextControl {
public:
							ContactFieldTextControl(BContactField* field);
							~ContactFieldTextControl();

	virtual	void			MouseDown(BPoint position);

			bool			HasChanged();
			void			Reload();
			void			UpdateField();

			BString			Value() const;

			BContactField*	Field() const;
private:
			void			_ShowPopUpMenu(BPoint screen);

			BContactField*	fField;
};

#endif // CONTACT_FIELD_TEXT_CONTROL_H
