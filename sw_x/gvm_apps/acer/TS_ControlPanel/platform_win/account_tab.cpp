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
#include "account_tab.h"
#include "util_funcs.h"
#include "settings_form.h"
#include "ccd_manager.h"
#include "async_operations.h"
#include "localized_text.h"

using namespace System::Collections;
using namespace System::IO;

AccountTab::AccountTab(SettingsForm^ parent) : Panel()
{
    mParent = parent;

    // buttons

    mLogoutButton = gcnew Button();
    mLogoutButton->Location = System::Drawing::Point(9, 32);
    mLogoutButton->Size = System::Drawing::Size(75, 23);
    mLogoutButton->TabIndex = 1;
    mLogoutButton->Text = LocalizedText::Instance->LogoutButton;
    mLogoutButton->UseVisualStyleBackColor = true;
    mLogoutButton->Click += gcnew EventHandler(this, &AccountTab::OnClickLogout);

    // labels

    mLogoutLabel = gcnew Label();
    mLogoutLabel->AutoSize = true;
    mLogoutLabel->Location = System::Drawing::Point(6, 16);
    mLogoutLabel->Size = System::Drawing::Size(214, 13);
    mLogoutLabel->TabIndex = 0;

    // text boxes

    mDeviceNameTextBox = gcnew TextBox();
    mDeviceNameTextBox->AcceptsReturn = false;
    mDeviceNameTextBox->AcceptsTab = false;
    mDeviceNameTextBox->Location = System::Drawing::Point(6, 19);
    mDeviceNameTextBox->Size = System::Drawing::Size(334, 20);
    mDeviceNameTextBox->TabIndex = 1;
    mDeviceNameTextBox->MaxLength = 64;
    mDeviceNameTextBox->TextChanged += gcnew EventHandler(this, &AccountTab::OnChangeDeviceName);

    // group boxes

    mDeviceNameGroupBox = gcnew GroupBox();
    mDeviceNameGroupBox->Controls->Add(mDeviceNameTextBox);
    mDeviceNameGroupBox->Location = System::Drawing::Point(3, 72);
    mDeviceNameGroupBox->Size = System::Drawing::Size(346, 54);
    mDeviceNameGroupBox->TabIndex = 8;
    mDeviceNameGroupBox->TabStop = false;
    mDeviceNameGroupBox->Text = LocalizedText::Instance->DeviceNameGroupBox;

    mLogoutGroupBox = gcnew GroupBox();
    mLogoutGroupBox->Controls->Add(mLogoutButton);
    mLogoutGroupBox->Controls->Add(mLogoutLabel);
    mLogoutGroupBox->Location = System::Drawing::Point(3, 0);
    mLogoutGroupBox->Size = System::Drawing::Size(346, 66);
    mLogoutGroupBox->TabIndex = 0;
    mLogoutGroupBox->TabStop = false;
    mLogoutGroupBox->Text = LocalizedText::Instance->AccountInfoGroupBox;

    // async operations

    AsyncOperations::Instance->LogoutComplete += gcnew AsyncCompleteHandler(this, &AccountTab::LogoutComplete);

    // panel

    this->Controls->Add(mLogoutGroupBox);
    this->Controls->Add(mDeviceNameGroupBox);
    this->Location = System::Drawing::Point(117, 12);
    this->Size = System::Drawing::Size(352, 258);
    this->TabIndex = 10;

    this->Reset();
}

AccountTab::~AccountTab()
{
    // do nothing, memory is managed
}

void AccountTab::Apply()
{
    if (CcdManager::Instance->DeviceName != mDeviceNameTextBox->Text) {
        try {
            CcdManager::Instance->DeviceName = mDeviceNameTextBox->Text;
        } catch (Exception^) {
            mDeviceNameTextBox->Text = CcdManager::Instance->DeviceName;
        }
    }
}

void AccountTab::OnChangeDeviceName(Object^ sender, EventArgs^ e)
{
    String^ newName = UtilFuncs::StripChars(mDeviceNameTextBox->Text, L"<>:\"/\\|?*+[]\t\n\r");
    if (mDeviceNameTextBox->Text != newName) {
        mDeviceNameTextBox->Text = newName;
    }

    mParent->SetApplyButton(true);
}

void AccountTab::OnClickLogout(Object^ sender, EventArgs^ e)
{
    AsyncOperations::Instance->Logout();
}

void AccountTab::LogoutComplete(String^ result)
{
    if (result != L"") {
		//if( !result->Contains(L"14118") )
		{
			MessageBox::Show(result);
			return;
		}
    }

    mParent->ShowSetup();
}

void AccountTab::Reset()
{
    mLogoutLabel->Text = String::Format(
        LocalizedText::Instance->LogoutLabel,
        CcdManager::Instance->UserName);
    mDeviceNameTextBox->Text = CcdManager::Instance->DeviceName;
}
