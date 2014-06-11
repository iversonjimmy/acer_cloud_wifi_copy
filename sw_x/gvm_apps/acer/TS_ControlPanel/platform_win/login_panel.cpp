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
#include "login_panel.h"
#include "util_funcs.h"
#include "setup_form.h"
#include "ccd_manager.h"
#include "async_operations.h"
#include "localized_text.h"

#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Collections;
using namespace System::IO;

LoginPanel::LoginPanel(SetupForm^ parent) : Panel()
{
    mParent = parent;

    // check boxes (HACK)

    mCheckBox = gcnew CheckBox();
    mCheckBox->AutoSize = true;
    mCheckBox->Location = System::Drawing::Point(80, 250);
    mCheckBox->Size = System::Drawing::Size(112, 17);
    mCheckBox->TabIndex = 0;
    mCheckBox->Text = LocalizedText::Instance->SyncServerLabel;
    mCheckBox->UseVisualStyleBackColor = true;
    mCheckBox->Checked = false;

    // labels

    mDeviceNameLabel = gcnew Label();
    mDeviceNameLabel->AutoSize = true;
    mDeviceNameLabel->Location = System::Drawing::Point(63, 160);
    mDeviceNameLabel->Size = System::Drawing::Size(99, 15);
    mDeviceNameLabel->TabIndex = 22;
    mDeviceNameLabel->Text = LocalizedText::Instance->DeviceNameLabel;

    mPasswordLabel = gcnew Label();
    mPasswordLabel->AutoSize = true;
    mPasswordLabel->Location = System::Drawing::Point(101, 100);
    mPasswordLabel->Size = System::Drawing::Size(61, 15);
    mPasswordLabel->TabIndex = 12;
    mPasswordLabel->Text = LocalizedText::Instance->PasswordLabel;

    mTitleLabel = gcnew Label();
    mTitleLabel->AutoSize = true;
    mTitleLabel->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 12,
        System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mTitleLabel->Location = System::Drawing::Point(107, 16);
    mTitleLabel->Size = System::Drawing::Size(225, 20);
    mTitleLabel->TabIndex = 1;
    mTitleLabel->Text = LocalizedText::Instance->LoginTitle;

    mUsernameLabel = gcnew Label();
    mUsernameLabel->AutoSize = true;
    mUsernameLabel->Location = System::Drawing::Point(93, 70);
    mUsernameLabel->Size = System::Drawing::Size(69, 15);
    mUsernameLabel->TabIndex = 7;
    mUsernameLabel->Text = LocalizedText::Instance->UsernameLabel;

    // link labels

    mForgetPassLinkLabel = gcnew LinkLabel();
    mForgetPassLinkLabel->AutoSize = true;
    mForgetPassLinkLabel->Location = System::Drawing::Point(265, 124);
    mForgetPassLinkLabel->Size = System::Drawing::Size(79, 15);
    mForgetPassLinkLabel->TabIndex = 20;
    mForgetPassLinkLabel->TabStop = true;
    mForgetPassLinkLabel->Text = LocalizedText::Instance->ForgotPasswordLabel;
    mForgetPassLinkLabel->Click += gcnew EventHandler(this, &LoginPanel::OnClickForgetPass);

    // text boxes

    mDeviceNameTextBox = gcnew TextBox();
    mDeviceNameTextBox->Location = System::Drawing::Point(168, 157);
    mDeviceNameTextBox->Size = System::Drawing::Size(196, 24);
    mDeviceNameTextBox->TabIndex = 21;
    mDeviceNameTextBox->Text = Environment::MachineName->ToLower();
    mDeviceNameTextBox->MaxLength = 64;
    mDeviceNameTextBox->TextChanged += gcnew EventHandler(this, &LoginPanel::OnChangeDeviceName);

    mPasswordTextBox = gcnew TextBox();
    mPasswordTextBox->Location = System::Drawing::Point(168, 97);
    mPasswordTextBox->Size = System::Drawing::Size(196, 24);
    mPasswordTextBox->TabIndex = 9;
    mPasswordTextBox->PasswordChar = L'*';
    mPasswordTextBox->MaxLength = 32;
    mPasswordTextBox->TextChanged += gcnew EventHandler(this, &LoginPanel::OnChangePassword);

    mUsernameTextBox = gcnew TextBox();
    mUsernameTextBox->Location = System::Drawing::Point(168, 67);
    mUsernameTextBox->Size = System::Drawing::Size(196, 24);
    mUsernameTextBox->TabIndex = 5;
    mUsernameTextBox->MaxLength = 32;
    mUsernameTextBox->TextChanged += gcnew EventHandler(this, &LoginPanel::OnChangeUsername);

    // async operations

    AsyncOperations::Instance->LoginComplete += gcnew LoginHandler(this, &LoginPanel::LoginComplete);

    // panel

    this->Controls->Add(mCheckBox);
    this->Controls->Add(mForgetPassLinkLabel);
    this->Controls->Add(mDeviceNameLabel);
    this->Controls->Add(mDeviceNameTextBox);
    this->Controls->Add(mPasswordLabel);
    this->Controls->Add(mPasswordTextBox);
    this->Controls->Add(mUsernameLabel);
    this->Controls->Add(mUsernameTextBox);
    this->Controls->Add(mTitleLabel);
    this->Location = System::Drawing::Point(14, 14);
    this->Size = System::Drawing::Size(564, 315);
    this->TabIndex = 0;
}

LoginPanel::~LoginPanel()
{
    // do nothing, memory is managed
}

bool LoginPanel::IsEnableSubmit::get()
{
    bool goodDevice = (mDeviceNameTextBox->Text != nullptr && mDeviceNameTextBox->Text->Length > 0);
    bool goodPass = (mPasswordTextBox->Text != nullptr && mPasswordTextBox->Text->Length >= 6);
    bool goodUser = (mUsernameTextBox->Text != nullptr && mUsernameTextBox->Text->Length > 0);

    return goodDevice && goodPass && goodUser;
}

void LoginPanel::EnableSubmit()
{
    mParent->EnableSubmit(this->IsEnableSubmit);
}

bool LoginPanel::Login()
{
    // check input

    bool badDevice = (mDeviceNameTextBox->Text == nullptr || mDeviceNameTextBox->Text->Length <= 0);
    bool badPass = (mPasswordTextBox->Text == nullptr || mPasswordTextBox->Text->Length < 6);
    bool badUser = (mUsernameTextBox->Text == nullptr || mUsernameTextBox->Text->Length <= 0);

    if (badDevice || badPass || badUser) {
        MessageBox::Show(L"All fields need to be filled in.",
            L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return false;
    }

    return AsyncOperations::Instance->Login(
        mUsernameTextBox->Text,
        mPasswordTextBox->Text,
        mDeviceNameTextBox->Text,
        mCheckBox->Checked);
}

void LoginPanel::LoginComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (result != L"") {
        MessageBox::Show(result, L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return;
    }

    mDeviceNameTextBox->Text = Environment::MachineName->ToLower();
    mPasswordTextBox->Text = L"";
    mUsernameTextBox->Text = L"";
    mCheckBox->Checked = false;
    mParent->Advance(nodes, filters);
}

void LoginPanel::OnChangeDeviceName(Object^ sender, EventArgs^ e)
{
    String^ newName = UtilFuncs::StripChars(mDeviceNameTextBox->Text, L"<>:\"/\\|?*+[]\t\n\r");
    if (mDeviceNameTextBox->Text != newName) {
        mDeviceNameTextBox->Text = newName;
    }

    this->EnableSubmit();
}

void LoginPanel::OnChangePassword(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void LoginPanel::OnChangeUsername(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void LoginPanel::OnClickForgetPass(Object^ sender, EventArgs^ e)
{
    UtilFuncs::ShowUrl(CcdManager::Instance->UrlForgetPassword);
}
 