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
#include "proxy_tab.h"
#include "settings_form.h"
#include "ccd_manager.h"

ProxyTab::ProxyTab(SettingsForm^ parent) : Panel()
{
    mParent = parent;

    // check boxes

    mRequiredCheckBox = gcnew CheckBox();
    mRequiredCheckBox->AutoSize = true;
    mRequiredCheckBox->Location = System::Drawing::Point(89, 171);
    mRequiredCheckBox->Size = System::Drawing::Size(128, 17);
    mRequiredCheckBox->TabIndex = 9;
    mRequiredCheckBox->Text = L"Requires a password.";
    mRequiredCheckBox->UseVisualStyleBackColor = true;
    mRequiredCheckBox->Checked = CcdManager::Instance->Settings->UseProxyPassword;
    mRequiredCheckBox->Click += gcnew EventHandler(this, &ProxyTab::OnClickRequirePassword);

    // combo boxes

    mTypeComboBox = gcnew ComboBox();
    mTypeComboBox->Items->AddRange(gcnew cli::array<Object^>(3) {L"HTTP", L"SOCKS4", L"SOCKS5"});
    mTypeComboBox->FormattingEnabled = true;
    mTypeComboBox->Location = System::Drawing::Point(89, 92);
    mTypeComboBox->Size = System::Drawing::Size(87, 21);
    mTypeComboBox->TabIndex = 4;
    mTypeComboBox->SelectedIndex = 0;
    mTypeComboBox->DropDownStyle = ComboBoxStyle::DropDownList;
    mTypeComboBox->SelectedIndexChanged += gcnew EventHandler(this, &ProxyTab::OnChangeType);

    // labels

    mPasswordLabel = gcnew Label();
    mPasswordLabel->AutoSize = true;
    mPasswordLabel->Location = System::Drawing::Point(27, 223);
    mPasswordLabel->Size = System::Drawing::Size(56, 13);
    mPasswordLabel->TabIndex = 13;
    mPasswordLabel->Text = L"Password:";

    mPortLabel = gcnew Label();
    mPortLabel->AutoSize = true;
    mPortLabel->Location = System::Drawing::Point(54, 148);
    mPortLabel->Size = System::Drawing::Size(29, 13);
    mPortLabel->TabIndex = 8;
    mPortLabel->Text = L"Port:";

    mServerLabel = gcnew Label();
    mServerLabel->AutoSize = true;
    mServerLabel->Location = System::Drawing::Point(42, 122);
    mServerLabel->Size = System::Drawing::Size(41, 13);
    mServerLabel->TabIndex = 5;
    mServerLabel->Text = L"Server:";

    mTypeLabel = gcnew Label();
    mTypeLabel->AutoSize = true;
    mTypeLabel->Location = System::Drawing::Point(24, 95);
    mTypeLabel->Size = System::Drawing::Size(59, 13);
    mTypeLabel->TabIndex = 3;
    mTypeLabel->Text = L"Proxy type:";

    mUsernameLabel = gcnew Label();
    mUsernameLabel->AutoSize = true;
    mUsernameLabel->Location = System::Drawing::Point(25, 197);
    mUsernameLabel->Size = System::Drawing::Size(58, 13);
    mUsernameLabel->TabIndex = 11;
    mUsernameLabel->Text = L"Username:";

    // radio buttons

    mAutoRadioButton = gcnew RadioButton();
    mAutoRadioButton->AutoSize = true;
    mAutoRadioButton->Location = System::Drawing::Point(6, 43);
    mAutoRadioButton->Size = System::Drawing::Size(80, 17);
    mAutoRadioButton->TabIndex = 1;
    mAutoRadioButton->TabStop = true;
    mAutoRadioButton->Text = L"Auto-detect";
    mAutoRadioButton->UseVisualStyleBackColor = true;
    mAutoRadioButton->Click += gcnew EventHandler(this, &ProxyTab::OnClickAuto);

    mManualRadioButton = gcnew RadioButton();
    mManualRadioButton->AutoSize = true;
    mManualRadioButton->Location = System::Drawing::Point(6, 66);
    mManualRadioButton->Size = System::Drawing::Size(60, 17);
    mManualRadioButton->TabIndex = 2;
    mManualRadioButton->TabStop = true;
    mManualRadioButton->Text = L"Manual";
    mManualRadioButton->UseVisualStyleBackColor = true;
    mManualRadioButton->Click += gcnew EventHandler(this, &ProxyTab::OnClickManual);

    mNoneRadioButton = gcnew RadioButton();
    mNoneRadioButton->AutoSize = true;
    mNoneRadioButton->Location = System::Drawing::Point(6, 20);
    mNoneRadioButton->Size = System::Drawing::Size(67, 17);
    mNoneRadioButton->TabIndex = 0;
    mNoneRadioButton->TabStop = true;
    mNoneRadioButton->Text = L"None";
    mNoneRadioButton->UseVisualStyleBackColor = true;
    mNoneRadioButton->Click += gcnew EventHandler(this, &ProxyTab::OnClickNone);

    // text boxes

    mPasswordTextBox = gcnew TextBox();
    mPasswordTextBox->Location = System::Drawing::Point(89, 220);
    mPasswordTextBox->Size = System::Drawing::Size(172, 20);
    mPasswordTextBox->TabIndex = 12;
    mPasswordTextBox->Text = CcdManager::Instance->Settings->ProxyPassword;

    mPortTextBox = gcnew TextBox();
    mPortTextBox->Location = System::Drawing::Point(89, 145);
    mPortTextBox->Size = System::Drawing::Size(53, 20);
    mPortTextBox->TabIndex = 7;
    mPortTextBox->Text = CcdManager::Instance->Settings->ProxyPort.ToString();
    mPortTextBox->TextAlign = HorizontalAlignment::Right;

    mServerTextBox = gcnew TextBox();
    mServerTextBox->Location = System::Drawing::Point(89, 119);
    mServerTextBox->Size = System::Drawing::Size(251, 20);
    mServerTextBox->TabIndex = 6;
    mServerTextBox->Text = CcdManager::Instance->Settings->ProxyServer;

    mUsernameTextBox = gcnew TextBox();
    mUsernameTextBox->Location = System::Drawing::Point(89, 194);
    mUsernameTextBox->Size = System::Drawing::Size(172, 20);
    mUsernameTextBox->TabIndex = 10;
    mUsernameTextBox->Text = CcdManager::Instance->Settings->ProxyUsername;

    // group boxes

    mSettingsGroupBox = gcnew GroupBox();
    mSettingsGroupBox->Controls->Add(mPasswordLabel);
    mSettingsGroupBox->Controls->Add(mPasswordTextBox);
    mSettingsGroupBox->Controls->Add(mUsernameLabel);
    mSettingsGroupBox->Controls->Add(mUsernameTextBox);
    mSettingsGroupBox->Controls->Add(mRequiredCheckBox);
    mSettingsGroupBox->Controls->Add(mPortLabel);
    mSettingsGroupBox->Controls->Add(mPortTextBox);
    mSettingsGroupBox->Controls->Add(mServerTextBox);
    mSettingsGroupBox->Controls->Add(mServerLabel);
    mSettingsGroupBox->Controls->Add(mTypeComboBox);
    mSettingsGroupBox->Controls->Add(mTypeLabel);
    mSettingsGroupBox->Controls->Add(mManualRadioButton);
    mSettingsGroupBox->Controls->Add(mAutoRadioButton);
    mSettingsGroupBox->Controls->Add(mNoneRadioButton);
    mSettingsGroupBox->Location = System::Drawing::Point(3, 3);
    mSettingsGroupBox->Size = System::Drawing::Size(346, 258);
    mSettingsGroupBox->TabIndex = 0;
    mSettingsGroupBox->TabStop = false;
    mSettingsGroupBox->Text = L"Proxy Settings";

    // panel

    this->Controls->Add(mSettingsGroupBox);
    this->Location = System::Drawing::Point(117, 12);
    this->Size = System::Drawing::Size(352, 264);
    this->TabIndex = 10;

    this->Apply();
}

ProxyTab::~ProxyTab()
{
    // do nothing, memory is managed
}

void ProxyTab::Apply()
{
    mRequiredCheckBox->Checked = CcdManager::Instance->Settings->UseProxyPassword;
    mPasswordTextBox->Text = CcdManager::Instance->Settings->ProxyPassword;
    mPortTextBox->Text = CcdManager::Instance->Settings->ProxyPort.ToString();
    mServerTextBox->Text = CcdManager::Instance->Settings->ProxyServer;
    mUsernameTextBox->Text = CcdManager::Instance->Settings->ProxyUsername;

    if (CcdManager::Instance->Settings->UseProxy == true
        && CcdManager::Instance->Settings->UseProxyAuto == true) {
        mAutoRadioButton->Checked = true;
        this->OnClickAuto(nullptr, nullptr);
    } else if (CcdManager::Instance->Settings->UseProxy == true) {
        mManualRadioButton->Checked = true;
        this->OnClickManual(nullptr, nullptr);
    } else {
        mNoneRadioButton->Checked = true;
        this->OnClickNone(nullptr, nullptr);
    }
}

void ProxyTab::OnChangePassword(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->ProxyPassword = mPasswordTextBox->Text;
    mParent->SetApplyButton(true);
}

void ProxyTab::OnChangePort(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->ProxyPort = Convert::ToInt32(mPortTextBox->Text);
    mParent->SetApplyButton(true);
}

void ProxyTab::OnChangeServer(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->ProxyServer = mServerTextBox->Text;
    mParent->SetApplyButton(true);
}

void ProxyTab::OnChangeType(Object^ sender, EventArgs^ e)
{
    if (mTypeComboBox->SelectedIndex == 0) {
        CcdManager::Instance->Settings->ProxyType = L"HTTP";
    } else if (mTypeComboBox->SelectedIndex == 1) {
        CcdManager::Instance->Settings->ProxyType = L"SOCKS4";
    } else if (mTypeComboBox->SelectedIndex == 2) {
        CcdManager::Instance->Settings->ProxyType = L"SOCKS5";
    }
    mParent->SetApplyButton(true);
}

void ProxyTab::OnChangeUsername(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->ProxyUsername = mUsernameTextBox->Text;
    mParent->SetApplyButton(true);
}

void ProxyTab::OnClickAuto(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->UseProxy = true;
    CcdManager::Instance->Settings->UseProxyAuto = true;

    mRequiredCheckBox->Enabled = false;
    mTypeComboBox->Enabled = false;
    mPasswordLabel->ForeColor = System::Drawing::Color::Gray;
    mPortLabel->ForeColor = System::Drawing::Color::Gray;
    mServerLabel->ForeColor = System::Drawing::Color::Gray;
    mTypeLabel->ForeColor = System::Drawing::Color::Gray;
    mUsernameLabel->ForeColor = System::Drawing::Color::Gray;
    mPasswordTextBox->Enabled = false;
    mPortTextBox->Enabled = false;
    mServerTextBox->Enabled = false;
    mUsernameTextBox->Enabled = false;

    mParent->SetApplyButton(true);
}

void ProxyTab::OnClickManual(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->UseProxy = true;
    CcdManager::Instance->Settings->UseProxyAuto = false;

    mRequiredCheckBox->Enabled = true;
    mTypeComboBox->Enabled = true;
    mPortLabel->ForeColor = System::Drawing::Color::Black;
    mServerLabel->ForeColor = System::Drawing::Color::Black;
    mTypeLabel->ForeColor = System::Drawing::Color::Black;
    mPortTextBox->Enabled = true;
    mServerTextBox->Enabled = true;
    this->OnClickRequirePassword(nullptr, nullptr);

    mParent->SetApplyButton(true);
}

void ProxyTab::OnClickNone(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->UseProxy = false;
    CcdManager::Instance->Settings->UseProxyAuto = false;

    mRequiredCheckBox->Enabled = false;
    mTypeComboBox->Enabled = false;
    mPasswordLabel->ForeColor = System::Drawing::Color::Gray;
    mPortLabel->ForeColor = System::Drawing::Color::Gray;
    mServerLabel->ForeColor = System::Drawing::Color::Gray;
    mTypeLabel->ForeColor = System::Drawing::Color::Gray;
    mUsernameLabel->ForeColor = System::Drawing::Color::Gray;
    mPasswordTextBox->Enabled = false;
    mPortTextBox->Enabled = false;
    mServerTextBox->Enabled = false;
    mUsernameTextBox->Enabled = false;

    mParent->SetApplyButton(true);
}

void ProxyTab::OnClickRequirePassword(Object^ sender, EventArgs^ e)
{
    if (mRequiredCheckBox->Checked == true) {
        mPasswordLabel->ForeColor = System::Drawing::Color::Black;
        mUsernameLabel->ForeColor = System::Drawing::Color::Black;
        mPasswordTextBox->Enabled = true;
        mUsernameTextBox->Enabled = true;
    } else {
        mPasswordLabel->ForeColor = System::Drawing::Color::Gray;
        mUsernameLabel->ForeColor = System::Drawing::Color::Gray;
        mPasswordTextBox->Enabled = false;
        mUsernameTextBox->Enabled = false;
    }

    CcdManager::Instance->Settings->UseProxyPassword = mRequiredCheckBox->Checked;
    mParent->SetApplyButton(true);
}
