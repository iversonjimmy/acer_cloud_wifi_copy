//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "sync_tab.h"
#include "settings_form.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "logger.h"
#include "localized_text.h"

using namespace System::Collections;
using namespace System::IO;

SyncTab::SyncTab(SettingsForm^ parent) : Panel()
{
    mParent = parent;
    mHasChanged = false;

    // labels

    mLabel = gcnew Label();
    mLabel->AutoSize = true;
    mLabel->Location = System::Drawing::Point(3, 0);
    mLabel->Size = System::Drawing::Size(207, 13);
    mLabel->TabIndex = 0;
    mLabel->Text = String::Format(
        LocalizedText::Instance->FilterLabel,
        CcdManager::Instance->Location);

    // tree views

    mTreeView = gcnew FilterTreeView();
    mTreeView->CheckBoxes = true;
    mTreeView->Location = System::Drawing::Point(3, 16);
    mTreeView->Size = System::Drawing::Size(346, 252);
    mTreeView->TabIndex = 1;
    mTreeView->AfterCheck += gcnew TreeViewEventHandler(this, &SyncTab::OnCheck);
    mTreeView->ImageList = gcnew ImageList();
    mTreeView->ImageList->Images->Add(Image::FromFile(L"images\\folder.png"));
    mTreeView->ImageList->Images->Add(Image::FromFile(L"images\\generic_file.png"));

    // panel

    this->Controls->Add(mTreeView);
    this->Controls->Add(mLabel);
    this->Location = System::Drawing::Point(117, 12);
    this->Size = System::Drawing::Size(352, 270);
    this->TabIndex = 10;
}

SyncTab::~SyncTab()
{
    // do nothing, memory is managed
}

List<JsonFilter^>^ SyncTab::Filters::get()
{
    return mTreeView->Filters;
}

bool SyncTab::HasChanged::get()
{
    return mHasChanged;
}

void SyncTab::HasChanged::set(bool value)
{
    mHasChanged = value;
}

List<TreeNode^>^ SyncTab::Nodes::get()
{
    return mTreeView->NodeList;
}

void SyncTab::OnCheck(Object^ sender, TreeViewEventArgs^ e)
{
    if (e->Action != TreeViewAction::ByMouse && e->Action != TreeViewAction::ByKeyboard) {
        return;
    }

    mHasChanged = true;
    mParent->SetApplyButton(true);
}

void SyncTab::Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    mHasChanged = false;
    mTreeView->Reset(nodes, filters);
    mLabel->Text = String::Format(
        LocalizedText::Instance->FilterLabel,
        CcdManager::Instance->Location);
}
