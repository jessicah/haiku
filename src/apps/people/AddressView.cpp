/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */
#include "AddressView.h"

#include <stdio.h>

#include <Box.h>
#include <ControlLook.h>
#include <ContactDefs.h>
#include <LayoutBuilder.h>

const uint32 kIsDeliveryLabel = 'ISdL';


AddressFieldView::AddressFieldView(BAddressContactField* field,
	BMenuItem* item)
	:
	BGridView(B_VERTICAL),
	fStreet(NULL),
	fPostalBox(NULL),
	fNeighbor(NULL),
	fCity(NULL),
	fRegion(NULL),
	fPostalCode(NULL),
	fCountry(NULL),
	fMenuItem(item),
	fCount(0),
	fField(field)
{
	if (field != NULL) {
		//fIsDeliveryLabel = new BCheckBox("isLabel", "Delivery label", NULL);
		//fIsDeliveryLabel->SetValue(fField->IsDeliveryLabel());
		//GridLayout()->AddView(fIsDeliveryLabel, 1, 1);

		fStreet = _AddControl("Street", field->Street());
		fPostalBox = _AddControl("PostalBox", field->PostalBox());
		fNeighbor = _AddControl("Neighborhood", field->Neighborhood());
		fCity = _AddControl("City", field->City());
		fRegion = _AddControl("Region", field->Region());
		fPostalCode = _AddControl("Postal Code", field->PostalCode());
		fCountry = _AddControl("Country", field->Country());
	}
}


AddressFieldView::~AddressFieldView()
{
}


bool
AddressFieldView::HasChanged() const
{
	if (fField == NULL)
		return false;

	if (fField->Street() != fStreet->Text())
		return true;

	if (fField->PostalBox() != fPostalBox->Text())
		return true;

	if (fField->Neighborhood() != fNeighbor->Text())
		return true;

	if (fField->City() != fCity->Text())
		return true;

	if (fField->Region() != fRegion->Text())
		return true;

	if (fField->PostalCode() != fPostalCode->Text())
		return true;

	if (fField->Country() != fCountry->Text())
		return true;

	//if (fField->IsDeliveryLabel() != fIsDeliveryLabel->Value())
	//	return true;

	return false;
}


void
AddressFieldView::Reload()
{
	if (fField == NULL)
		return;

	fStreet->SetText(fField->Street());
	fPostalBox->SetText(fField->PostalBox());
	fNeighbor->SetText(fField->Neighborhood());
	fCity->SetText(fField->City());
	fRegion->SetText(fField->Region());
	fPostalCode->SetText(fField->PostalCode());
	fCountry->SetText(fField->Country());
	//fIsDeliveryLabel->SetValue(fField->IsDeliveryLabel());
}


void
AddressFieldView::UpdateAddressField()
{
	if (fField == NULL)
		return;

	fField->SetStreet(fStreet->Text());
	fField->SetPostalBox(fPostalBox->Text());
	fField->SetNeighborhood(fNeighbor->Text());
	fField->SetCity(fCity->Text());
	fField->SetRegion(fRegion->Text());
	fField->SetPostalCode(fPostalCode->Text());
	fField->SetCountry(fCountry->Text());
//	fField->SetDeliveryLabel(fIsDeliveryLabel->Value());
}


void
AddressFieldView::SetMenuItem(BMenuItem* field)
{
	fMenuItem = field;
}


BMenuItem*
AddressFieldView::MenuItem()
{
	return fMenuItem;
}


BAddressContactField*
AddressFieldView::Field()
{
	return fField;
}


BTextControl*
AddressFieldView::_AddControl(const char* label, const char* value)
{
	BTextControl* control = new BTextControl(label, value, NULL);

	BGridLayout* layout = GridLayout();
	int count = layout->CountRows();

	BLayoutItem* item = control->CreateLabelLayoutItem();
	item->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP));
	layout->AddItem(item, 0, count);
	layout->AddItem(control->CreateTextViewLayoutItem(), 1, count);

	return control;
}


AddressView::AddressView(BContact* contact)
	:
	BGroupView(B_VERTICAL),
	fCurrentView(NULL),
	fContact(contact),
	fHasChanged(false)
{
	SetFlags(Flags() | B_WILL_DRAW);

	float spacing = be_control_look->DefaultItemSpacing();
	GroupLayout()->SetInsets(spacing, spacing, spacing, spacing);

	fLocationsMenu = new BPopUpMenu("Add a new location");
	fMenuField = new BMenuField("Locations", "Locations :",
		fLocationsMenu);

	BMenuItem* newLocation = new BMenuItem("Add a new location",
		new BMessage(M_ADD_ADDRESS));
	BMenuItem* remLocation = new BMenuItem("Remove current location",
		new BMessage(M_REMOVE_ADDRESS));

	fLocationsMenu->AddItem(newLocation);
	fLocationsMenu->AddItem(remLocation);

	fLocationsMenu->AddSeparatorItem();

	fFieldView = new BGroupView(B_VERTICAL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(10, 10, 10, 10)
		.Add(fMenuField)
		.AddGlue()
		.Add(fFieldView);
}


AddressView::~AddressView()
{
}


void
AddressView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SHOW_ADDRESS:
		{
			AddressFieldView* view = NULL;
			if (msg->FindPointer("viewpointer", (void**)&view) != B_OK)
				return;
			if (view != NULL)
				SelectView(view);
			break;
		}

		case M_ADD_ADDRESS:
		 {
			BAddressContactField* field = new BAddressContactField();
			AddNewAddress(field);		 	
			break;
		 }

		case M_REMOVE_ADDRESS:
			RemoveAddress();
			break;

		default:
			BView::MessageReceived(msg);
	}
}


bool
AddressView::HasChanged() const
{
	if (fHasChanged)
		return fHasChanged;

	int32 count = fFieldsList.CountItems();
	for (int32 i = 0; i < count; i++) {
		AddressFieldView* view = fFieldsList.ItemAt(i);
		if (view != NULL && view->HasChanged())
			return true;
	}
	return false;
}


void
AddressView::Reload()
{
	int32 count = fFieldsList.CountItems();
	for (int32 i = 0; i < count; i++) {
		AddressFieldView* view = fFieldsList.ItemAt(i);
		if (view != NULL)
			view->Reload();
	}
	fHasChanged = false;
}


void
AddressView::UpdateAddressField()
{
	int32 count = fFieldsList.CountItems();
	for (int32 i = 0; i < count; i++) {
		AddressFieldView* view = fFieldsList.ItemAt(i);
		if (view != NULL)
			view->UpdateAddressField();
	}
	fHasChanged = false;
}


void
AddressView::AddNewAddress(BAddressContactField* field)
{
	fContact->AddField(field);
	AddAddress(field);
	fHasChanged = true;
}


void
AddressView::AddAddress(BAddressContactField* field)
{
	if (field == NULL)
		return;

	BString label;
	if (field->Value().Length() > 25) {
		label.Append(field->Value(), 23);
		label << "...";
		label.ReplaceAll(";", ",");
	} else
		label.SetTo(field->Value());

	BMessage* msg = new BMessage(M_SHOW_ADDRESS);
	BMenuItem* fieldItem = new BMenuItem(label, msg);

	AddressFieldView* view = new AddressFieldView(field, fieldItem);
	fFieldsList.AddItem(view);
	msg->AddPointer("viewpointer", view);

	fieldItem->SetMarked(true);
	fLocationsMenu->AddItem(fieldItem);

	fFieldView->GroupLayout()->AddView(view);
	view->Hide();

	SelectView(view);
}


void
AddressView::RemoveAddress()
{
	if (fCurrentView != NULL) {
		fLocationsMenu->RemoveItem(fCurrentView->MenuItem());
		fFieldView->RemoveChild(fCurrentView);
		fFieldsList.RemoveItem(fCurrentView);
		fContact->RemoveField(fCurrentView->Field());
		fHasChanged = true;
	}
	SelectView(0);
}


void
AddressView::SelectView(int index)
{
	AddressFieldView* view = fFieldsList.ItemAt(index);
	if (view != NULL) {
		if (fCurrentView != NULL)
			fCurrentView->Hide();

		view->Show();
		view->MenuItem()->SetMarked(true);
		fCurrentView = view;
	} else {
		// NOTE the index '0' of SelectView
		// is the first address, but really
		// the index 0 of the menu is the "Add new address"
		// BMenufield.
		BMenuItem* field = fLocationsMenu->ItemAt(0);
		field->SetMarked(true);
	}
}


void
AddressView::SelectView(AddressFieldView* view)
{
	if (view != NULL) {
		if (fCurrentView != NULL)
			fCurrentView->Hide();

		view->Show();
		fCurrentView = view;
	} else
		SelectView(0);
}
