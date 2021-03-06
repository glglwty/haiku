/*
 * Copyright 2014-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "DNSSettingsView.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <StringView.h>

#include "IPAddressControl.h"


static const int32 kMsgAddServer = 'adds';
static const int32 kMsgDeleteServer = 'dels';
static const int32 kMsgMoveUp = 'mvup';
static const int32 kMsgMoveDown = 'mvdn';
static const int32 kMsgApply = 'aply';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DNSSettingsView"


DNSSettingsView::DNSSettingsView(BNetworkSettingsItem* item)
	:
	BView("dns", 0),
	fItem(item)
{
	fServerListView = new BListView("nameservers");
	fTextControl = new IPAddressControl(AF_UNSPEC, B_TRANSLATE("Server"),
		"server");
	fTextControl->SetExplicitMinSize(BSize(fTextControl->StringWidth("5") * 18,
		B_SIZE_UNSET));

	fAddButton = new BButton(B_TRANSLATE("Add"), new BMessage(kMsgAddServer));
	fAddButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fUpButton = new BButton(B_TRANSLATE("Move up"), new BMessage(kMsgMoveUp));
	fUpButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fDownButton = new BButton(B_TRANSLATE("Move down"),
		new BMessage(kMsgMoveDown));
	fDownButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fRemoveButton = new BButton(B_TRANSLATE("Remove"), new BMessage(kMsgDeleteServer));
	fRemoveButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(kMsgApply));

	BBox* box = new BBox("dns");
	box->SetLabel(B_TRANSLATE("DNS servers"));
	box->AddChild(BLayoutBuilder::Grid<>()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fTextControl, 0, 0)
		.Add(fAddButton, 1, 0)
		.Add(new BScrollView("scroll", fServerListView, 0, false, true),
			0, 1, 1, 3)
		.Add(fUpButton, 1, 1)
		.Add(fDownButton, 1, 2)
		.Add(fRemoveButton, 1, 3)
		.View());

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(box)
		.Add(fDomain = new BTextControl(B_TRANSLATE("Domain"), "", NULL))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fApplyButton);

	_LoadDNSConfiguration();
}


DNSSettingsView::~DNSSettingsView()
{
}


status_t
DNSSettingsView::Revert()
{
	int i;
	for (i = 0; i < fRevertList.CountStrings(); i++) {
		BStringItem* item = static_cast<BStringItem*>(
			fServerListView->ItemAt(i));
		if (item == NULL) {
			item = new BStringItem("");
			fServerListView->AddItem(item);
		}

		item->SetText(fRevertList.StringAt(i));
	}

	// Now remove any extra item
	for (; i < fServerListView->CountItems(); i++)
		delete fServerListView->RemoveItem(i);

	return B_OK;
}


bool
DNSSettingsView::IsRevertable() const
{
	// TODO
	return false;
}


void
DNSSettingsView::AttachedToWindow()
{
	fAddButton->SetTarget(this);
	fRemoveButton->SetTarget(this);
	fUpButton->SetTarget(this);
	fDownButton->SetTarget(this);

	fTextControl->SetTarget(this);

	fApplyButton->SetTarget(this);
}


void
DNSSettingsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAddServer:
		{
			const char* address = fTextControl->Text();
			fServerListView->AddItem(new BStringItem(address));
			break;
		}
		case kMsgDeleteServer:
			delete fServerListView->RemoveItem(
				fServerListView->CurrentSelection());
			break;

		case kMsgMoveUp:
		{
			int index = fServerListView->CurrentSelection();
			if (index > 0)
				fServerListView->SwapItems(index, index - 1);
			break;
		}
		case kMsgMoveDown:
		{
			int index = fServerListView->CurrentSelection();
			if (index < fServerListView->CountItems() - 1)
				fServerListView->SwapItems(index, index + 1);
			break;
		}
		case kMsgApply:
			if (_SaveDNSConfiguration() == B_OK)
				fItem->NotifySettingsUpdated();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


status_t
DNSSettingsView::_LoadDNSConfiguration()
{
	if (res_init() != 0)
		return B_ERROR;

	res_state state = __res_state();

	if (state != NULL) {
		for (int i = 0; i < state->nscount; i++) {
			char* address = inet_ntoa(state->nsaddr_list[i].sin_addr);
			fServerListView->AddItem(new BStringItem(address));
			fRevertList.Add(address);
		}

		fDomain->SetText(state->dnsrch[0]);
		return B_OK;
	}

	return B_ERROR;
}


status_t
DNSSettingsView::_SaveDNSConfiguration()
{
	BPath path;
	status_t status;
	status = find_directory(B_SYSTEM_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("network/resolv.conf");

	BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK) {
		fprintf(stderr, "failed to open %s for writing: %s\n", path.Path(),
			strerror(file.InitCheck()));
		return file.InitCheck();
	}

	BString content("# Generated by Network preferences\n");

	for (int i = 0; i < fServerListView->CountItems(); i++) {
		BString item = ((BStringItem*)fServerListView->ItemAt(i))->Text();
		if (item.Length() > 0)
			content << "nameserver\t" << item.String() << "\n";
	}

	if (strlen(fDomain->Text()) > 0)
		content << "domain\t" << fDomain->Text() << "\n";

	return file.Write(content.String(), content.Length());
}
