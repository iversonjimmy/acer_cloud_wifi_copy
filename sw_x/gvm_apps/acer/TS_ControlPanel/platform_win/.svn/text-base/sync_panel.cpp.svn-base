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
#include "sync_panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "setup_form.h"
#include "async_operations.h"
#include "logger.h"
#include "localized_text.h"

#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Collections;
using namespace System::IO;

SyncPanel::SyncPanel(SetupForm^ parent) : Panel()
{
    mParent = parent;

    // labels

    mTitleLabel = gcnew Label();
    mTitleLabel->AutoSize = true;
    mTitleLabel->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 12,
        System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mTitleLabel->Location = System::Drawing::Point(127, 16);
    mTitleLabel->Size = System::Drawing::Size(225, 20);
    mTitleLabel->TabIndex = 1;
    mTitleLabel->Text = LocalizedText::Instance->FilterTitle;

    mTreeViewLabel = gcnew Label();
    mTreeViewLabel->AutoSize = true;
    mTreeViewLabel->Location = System::Drawing::Point(18, 0 + 16 + 20 + 6);
    mTreeViewLabel->Size = System::Drawing::Size(207, 13);
    mTreeViewLabel->TabIndex = 0;
    mTreeViewLabel->Text = String::Format(
        LocalizedText::Instance->FilterLabel,
        CcdManager::Instance->Location);

    // tree views

    mTreeView = gcnew FilterTreeView();
    mTreeView->CheckBoxes = true;
    mTreeView->Location = System::Drawing::Point(18, 16 + 16 + 20 + 6);
    mTreeView->Size = System::Drawing::Size(396, 252);
    mTreeView->TabIndex = 1;
    mTreeView->ImageList = gcnew ImageList();
    mTreeView->ImageList->Images->Add(Image::FromFile(L"images\\folder.png"));
    mTreeView->ImageList->Images->Add(Image::FromFile(L"images\\generic_file.png"));

    // async operations

    AsyncOperations::Instance->ApplySettingsComplete += gcnew AsyncCompleteHandler(this, &SyncPanel::ApplyComplete);

    // panel

    this->Controls->Add(mTreeView);
    this->Controls->Add(mTreeViewLabel);
    this->Controls->Add(mTitleLabel);
    this->Location = System::Drawing::Point(14, 14);
    this->Size = System::Drawing::Size(436, 315);
    this->TabIndex = 0;
}

SyncPanel::~SyncPanel()
{
    // do nothing, memory is managed
}

void SyncPanel::Apply()
{
    if (CcdManager::Instance->IsLoggedIn == false) {
        return;
    }

    AsyncOperations::Instance->ApplySettings(mTreeView->NodeList, mTreeView->Filters);
}

void SyncPanel::ApplyComplete(String^ result)
{
    if (result != L"") {
        MessageBox::Show(result);
    }

    mParent->Advance(nullptr, nullptr);
}

void SyncPanel::Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    mTreeView->Reset(nodes, filters);
    mTreeViewLabel->Text = String::Format(
        LocalizedText::Instance->FilterLabel,
        CcdManager::Instance->Location);
}
