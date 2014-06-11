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
#include <windows.h>
#include "util_funcs.h"
#include "settings_form.h"
#include "async_operations.h"
#include "localized_text.h"
#include "Layout.h"
//#include "MessageProcForm.h"
//#include "WM_USER.h"
//#include "PlayerOptions.h"


SettingsForm::SettingsForm()
{
    // list boxes

    mTabListBox = gcnew ListBox();
    mTabListBox->Font = gcnew System::Drawing::Font(L"Lucida Sans Unicode", 9.75F,
        System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0));
    mTabListBox->FormattingEnabled = true;
    mTabListBox->ItemHeight = 16;
    mTabListBox->Items->AddRange(LocalizedText::Instance->TabList);
    mTabListBox->Location = System::Drawing::Point(12, 12);
    mTabListBox->Size = System::Drawing::Size(99, 270);
    mTabListBox->TabIndex = 9;
    mTabListBox->SelectedIndex = 0;
    mTabListBox->SelectedIndexChanged += gcnew EventHandler(this, &SettingsForm::OnChangeTab);

    // buttons

    //mApplyButton = gcnew Button();
    //mApplyButton->Enabled = false;
    //mApplyButton->Location = System::Drawing::Point(397, 291);
    //mApplyButton->Size = System::Drawing::Size(72, 23);
    //mApplyButton->TabIndex = 6;
    //mApplyButton->Text = LocalizedText::Instance->ApplyButton;
    //mApplyButton->UseVisualStyleBackColor = true;
    //mApplyButton->Click += gcnew EventHandler(this, &SettingsForm::OnClickApply);

    //mCancelButton = gcnew Button();
    //mCancelButton->Location = System::Drawing::Point(319, 291);
    //mCancelButton->Size = System::Drawing::Size(72, 23);
    //mCancelButton->TabIndex = 7;
    //mCancelButton->Text = LocalizedText::Instance->CancelButton;
    //mCancelButton->UseVisualStyleBackColor = true;
    //mCancelButton->Click += gcnew EventHandler(this, &SettingsForm::OnClickCancel);

    //mOkayButton = gcnew Button();
    //mOkayButton->Location = System::Drawing::Point(241, 291);
    //mOkayButton->Size = System::Drawing::Size(72, 23);
    //mOkayButton->TabIndex = 8;
    //mOkayButton->Text = LocalizedText::Instance->OkayButton;
    //mOkayButton->UseVisualStyleBackColor = true;
    //mOkayButton->Click += gcnew EventHandler(this, &SettingsForm::OnClickOkay);

    // tray icon

    mComponents = gcnew System::ComponentModel::Container();
    mIcon = gcnew TrayIcon(this, mComponents);

    // panels

    mAccountTab = gcnew AccountTab(this);
//    mGeneralTab = gcnew GeneralTab(this);
    mSyncTab = gcnew SyncTab(this);
    mPanel = mGeneralTab;

    // async operations

    AsyncOperations::Instance->ResetSettingsComplete +=
        gcnew ResetSettingsHandler(this, &SettingsForm::ResetSettingsComplete);
    AsyncOperations::Instance->ApplySettingsComplete +=
        gcnew AsyncCompleteHandler(this, &SettingsForm::ApplySettingsComplete);

    // forms

#pragma push_macro("ExtractAssociatedIcon")
#undef ExtractAssociatedIcon

    this->Icon = System::Drawing::Icon::ExtractAssociatedIcon(Application::ExecutablePath);	

#pragma pop_macro("ExtractAssociatedIcon")

	
	
    this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
    this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
    this->Controls->Add(mTabListBox);
    //this->Controls->Add(mOkayButton);
    //this->Controls->Add(mCancelButton);
    //this->Controls->Add(mApplyButton);
    this->Controls->Add(mGeneralTab);
    this->Controls->Add(mAccountTab);
    this->Controls->Add(mSyncTab);
    this->ClientSize = System::Drawing::Size(Panel_Width, Panel_Height);
    this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
    this->MaximizeBox = false;
    this->MinimizeBox = false;
    this->Text = LocalizedText::Instance->FormTitle;
    this->Opacity = 0;
    this->ShowInTaskbar = false;
    this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
    this->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 8.25F,
        System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    this->Closing += gcnew System::ComponentModel::CancelEventHandler(this, &SettingsForm::OnClose);
	this->Load += gcnew EventHandler(this, &SettingsForm::OnLoad); 

    mAccountTab->Hide();
    mSyncTab->Hide();

	
    mSetupForm = gcnew SetupForm();

    if (CcdManager::Instance->IsLoggedIn == false) 
	{
        this->ShowSetup();
    }

    mIsReset = false;



}

SettingsForm::~SettingsForm()
{
	if (mComponents) {
		delete mComponents;
	}
}

void SettingsForm::OnLoad(Object^ sender, EventArgs^ e)
{
	//A form to recieve windows mesasge.
	
}

void SettingsForm::HideForm()
{
	//this->Hide();
    this->Opacity = 0;	
    this->ShowInTaskbar = false;
}

void SettingsForm::Reset()
{
    if (mIsReset == true) 
	{
		mSetupForm->Show();
        return;
    }

    //mAccountTab->Reset();
    //mGeneralTab->Reset();
	
	if(!mSetupForm->IsSyncPanelState())
	{
		this->BringToFront();
		AsyncOperations::Instance->ResetSettings();
		mIsReset = true;
	}
	else
	{
		mSetupForm->Show();
	}
}


void SettingsForm::ApplySettingsComplete(String^ result)
{
    if (result != L"") {
        MessageBox::Show(result);
    }

	
	
	this->SetApplyButton(false);
	mIsReset = false;
	
}



void SettingsForm::ResetSettingsComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
	this->ShowSetup();	

    this->Focus();

    mIsReset = false;
}

void SettingsForm::ShowSetup()
{
    this->HideForm();
	if(!mSetupForm->Visible)
		mSetupForm->Visible= true;

	mSetupForm->BringToFront();
    //mSetupForm->Show();
}

void SettingsForm::OpenAcerTS()
{
	if (CcdManager::Instance->IsLoggedIn == false) 
	{
        ShowSetup();
    }
	else
	{
		Reset();
	}
}

void SettingsForm::SetApplyButton(bool enable)
{
    //mApplyButton->Enabled = enable;
}

void SettingsForm::OnChangeTab(Object^ sender, EventArgs^ e)
{
    int index = mTabListBox->SelectedIndex;

    if (mPanel != nullptr) {
        mPanel->Hide();
    }

    if (index == 0) {
        mPanel = mGeneralTab;
    } else if (index == 1) {
        mPanel = mAccountTab;
    } else if (index == 2) {
        mPanel = mSyncTab;
    } else {
        mPanel = nullptr;
    }

    if (mPanel != nullptr) {
        mPanel->Show();
    }
}

//void SettingsForm::OnClickApply(Object^ sender, EventArgs^ e)
//{
//    if (mIsReset == true) {
//        return;
//    }
//
//    mAccountTab->Apply();
//    mGeneralTab->Apply();
//    if (mSyncTab->HasChanged == true) {
//        AsyncOperations::Instance->ApplySettings(mSyncTab->Nodes, mSyncTab->Filters);
//        mIsReset = true;
//        mSyncTab->HasChanged = false;
//    } else {
//        this->SetApplyButton(false);
//        mIsReset = false;
//    }
//}

//void SettingsForm::OnClickCancel(Object^ sender, EventArgs^ e)
//{
//    // just hide form, dont close it
//	this->Hide();//Form();
//}

//void SettingsForm::OnClickOkay(Object^ sender, EventArgs^ e)
//{
//    this->OnClickApply(nullptr, nullptr);
//
//    // just hide form, dont close it
//    this->Hide();//Form();
//}

void SettingsForm::OnClose(Object^ sender, System::ComponentModel::CancelEventArgs^ e)
{
//    this->OnClickCancel(nullptr, nullptr);
    e->Cancel = true;
}


    //[SecurityPermission(SecurityAction::Demand, Flags=SecurityPermissionFlag::UnmanagedCode)]
//void SettingsForm::WndProc( Message% m )
//{
//	if(mSetupForm != nullptr)
//		mSetupForm->MessageProc(m);
//}
