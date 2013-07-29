/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */
#ifndef ADDRESS_VIEW_H
#define ADDRESS_VIEW_H

#include <CheckBox.h>
#include <Contact.h>
#include <ContactField.h>
#include <GridView.h>
#include <GroupView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ObjectList.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

enum {
	M_ADD_ADDRESS		= 'adar',
	M_REMOVE_ADDRESS	= 'rmad',
	M_SHOW_ADDRESS		= 'shad'
};


class AddressFieldView : public BGridView {
public:
							AddressFieldView(BAddressContactField* field,
								BMenuItem* item);
	virtual					~AddressFieldView();

			bool			HasChanged() const;
			void			Reload();
			void			UpdateAddressField();
			void			SetMenuItem(BMenuItem* field);

			BAddressContactField* Field();

			BMenuItem*		MenuItem();

private:
			BTextControl*	_AddControl(const char* label, const char* value);

			BTextControl*	fStreet;
			BTextControl*	fPostalBox;
			BTextControl*	fNeighbor;
			BTextControl*	fCity;
			BTextControl*	fRegion;
			BTextControl*	fPostalCode;
			BTextControl*	fCountry;

			BMenuItem*		fMenuItem;
			int32			fCount;

			//BCheckBox*		fIsDeliveryLabel;

BAddressContactField* fField;
};

class AddressView : public BGroupView {
public:
							AddressView(BContact* contact);

	virtual					~AddressView();

	virtual	void			MessageReceived(BMessage* msg);
	
			bool			HasChanged() const;
			void			Reload();
			void			UpdateAddressField();

			void			AddAddress(BAddressContactField* field);
			void			AddNewAddress(BAddressContactField* field);
			void			RemoveAddress();
			void			SelectView(AddressFieldView* view);
			void			SelectView(int index);

private:
			BMenuField*		fMenuField;
			BPopUpMenu* 	fLocationsMenu;

			BGroupView*		fFieldView;

			BObjectList<AddressFieldView> fFieldsList;
			AddressFieldView* fCurrentView;
			BContact* 		fContact;
			bool			fHasChanged;
};

#endif // ADDRESS_VIEW_H
