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
#include "bandwidth_tab.h"
#include "settings_form.h"
#include "ccd_manager.h"

BandwidthTab::BandwidthTab(SettingsForm^ parent) : Panel()
{
    mParent = parent;

    // labels

    mDownLabel = gcnew Label();
    mDownLabel->AutoSize = true;
    mDownLabel->Location = System::Drawing::Point(146, 57);
    mDownLabel->Size = System::Drawing::Size(30, 13);
    mDownLabel->TabIndex = 3;
    mDownLabel->Text = L"kB/s";
    mDownLabel->ForeColor = System::Drawing::Color::Gray;

    mUpLabel = gcnew Label();
    mUpLabel->AutoSize = true;
    mUpLabel->Location = System::Drawing::Point(146, 79);
    mUpLabel->Size = System::Drawing::Size(30, 13);
    mUpLabel->TabIndex = 3;
    mUpLabel->Text = L"kB/s";
    mUpLabel->ForeColor = System::Drawing::Color::Gray;

    // radio buttons

    mDownLimitRadioButton = gcnew RadioButton();
    mDownLimitRadioButton->AutoSize = true;
    mDownLimitRadioButton->Location = System::Drawing::Point(7, 53);
    mDownLimitRadioButton->Size = System::Drawing::Size(61, 17);
    mDownLimitRadioButton->TabIndex = 1;
    mDownLimitRadioButton->TabStop = true;
    mDownLimitRadioButton->Text = L"Limit to:";
    mDownLimitRadioButton->UseVisualStyleBackColor = true;
    mDownLimitRadioButton->Click += gcnew EventHandler(this, &BandwidthTab::OnClickDownLimit);

    mDownUnlimitedRadioButton = gcnew RadioButton();
    mDownUnlimitedRadioButton->AutoSize = true;
    mDownUnlimitedRadioButton->Location = System::Drawing::Point(7, 29);
    mDownUnlimitedRadioButton->Size = System::Drawing::Size(68, 17);
    mDownUnlimitedRadioButton->TabIndex = 0;
    mDownUnlimitedRadioButton->TabStop = true;
    mDownUnlimitedRadioButton->Text = L"Unlimited";
    mDownUnlimitedRadioButton->UseVisualStyleBackColor = true;
    mDownUnlimitedRadioButton->Click += gcnew EventHandler(this, &BandwidthTab::OnClickDownUnlimit);

    mUpLimitRadioButton = gcnew RadioButton();
    mUpLimitRadioButton->AutoSize = true;
    mUpLimitRadioButton->Location = System::Drawing::Point(6, 75);
    mUpLimitRadioButton->Size = System::Drawing::Size(61, 17);
    mUpLimitRadioButton->TabIndex = 1;
    mUpLimitRadioButton->TabStop = true;
    mUpLimitRadioButton->Text = L"Limit to:";
    mUpLimitRadioButton->UseVisualStyleBackColor = true;
    mUpLimitRadioButton->Click += gcnew EventHandler(this, &BandwidthTab::OnClickUpLimit);

    mUpUnlimitedRadioButton = gcnew RadioButton();
    mUpUnlimitedRadioButton->AutoSize = true;
    mUpUnlimitedRadioButton->Location = System::Drawing::Point(7, 29);
    mUpUnlimitedRadioButton->Size = System::Drawing::Size(68, 17);
    mUpUnlimitedRadioButton->TabIndex = 0;
    mUpUnlimitedRadioButton->TabStop = true;
    mUpUnlimitedRadioButton->Text = L"Unlimited";
    mUpUnlimitedRadioButton->UseVisualStyleBackColor = true;
    mUpUnlimitedRadioButton->Click += gcnew EventHandler(this, &BandwidthTab::OnClickUpUnlimit);

    mUpLimitAutoRadioButton = gcnew RadioButton();
    mUpLimitAutoRadioButton->AutoSize = true;
    mUpLimitAutoRadioButton->Location = System::Drawing::Point(6, 52);
    mUpLimitAutoRadioButton->Size = System::Drawing::Size(110, 17);
    mUpLimitAutoRadioButton->TabIndex = 4;
    mUpLimitAutoRadioButton->TabStop = true;
    mUpLimitAutoRadioButton->Text = L"Limit automatically";
    mUpLimitAutoRadioButton->UseVisualStyleBackColor = true;
    mUpLimitAutoRadioButton->Click += gcnew EventHandler(this, &BandwidthTab::OnClickUpLimitAuto);

    // text boxes

    mDownTextBox = gcnew TextBox();
    mDownTextBox->Location = System::Drawing::Point(74, 53);
    mDownTextBox->Size = System::Drawing::Size(66, 20);
    mDownTextBox->TabIndex = 2;
    mDownTextBox->Enabled = false;
    mDownTextBox->Text = CcdManager::Instance->Settings->DownloadRate.ToString();
    mDownTextBox->TextAlign = HorizontalAlignment::Right;
    mDownTextBox->ForeColor = System::Drawing::Color::Gray;
    mDownTextBox->TextChanged += gcnew EventHandler(this, &BandwidthTab::OnChangeDownRate);

    mUpTextBox = gcnew TextBox();
    mUpTextBox->Location = System::Drawing::Point(74, 75);
    mUpTextBox->Size = System::Drawing::Size(65, 20);
    mUpTextBox->TabIndex = 2;
    mUpTextBox->Enabled = false;
    mUpTextBox->Text = CcdManager::Instance->Settings->UploadRate.ToString();
    mUpTextBox->TextAlign = HorizontalAlignment::Right;
    mUpTextBox->ForeColor = System::Drawing::Color::Gray;
    mUpTextBox->TextChanged += gcnew EventHandler(this, &BandwidthTab::OnChangeUpRate);

    // group boxes

    mDownGroupBox = gcnew GroupBox();
    mDownGroupBox->Controls->Add(mDownLabel);
    mDownGroupBox->Controls->Add(mDownTextBox);
    mDownGroupBox->Controls->Add(mDownLimitRadioButton);
    mDownGroupBox->Controls->Add(mDownUnlimitedRadioButton);
    mDownGroupBox->Location = System::Drawing::Point(3, 3);
    mDownGroupBox->Size = System::Drawing::Size(346, 83);
    mDownGroupBox->TabIndex = 9;
    mDownGroupBox->TabStop = false;
    mDownGroupBox->Text = L"Download Rate";

    mUpGroupBox = gcnew GroupBox();
    mUpGroupBox->Controls->Add(mUpLimitAutoRadioButton);
    mUpGroupBox->Controls->Add(mUpLabel);
    mUpGroupBox->Controls->Add(mUpTextBox);
    mUpGroupBox->Controls->Add(mUpLimitRadioButton);
    mUpGroupBox->Controls->Add(mUpUnlimitedRadioButton);
    mUpGroupBox->Location = System::Drawing::Point(3, 92);
    mUpGroupBox->Size = System::Drawing::Size(346, 105);
    mUpGroupBox->TabIndex = 10;
    mUpGroupBox->TabStop = false;
    mUpGroupBox->Text = L"Upload Rate";

    // panel

    this->Controls->Add(mUpGroupBox);
    this->Controls->Add(mDownGroupBox);
    this->Location = System::Drawing::Point(117, 12);
    this->Size = System::Drawing::Size(352, 258);
    this->TabIndex = 9;

    this->Apply();
}

BandwidthTab::~BandwidthTab()
{
    // do nothing, memory is managed
}

void BandwidthTab::Apply()
{
    mDownTextBox->Text = CcdManager::Instance->Settings->DownloadRate.ToString();
    mUpTextBox->Text = CcdManager::Instance->Settings->UploadRate.ToString();

    // download init

    if (CcdManager::Instance->Settings->LimitDownload == true) {
        mDownLimitRadioButton->Checked = true;
        mDownLabel->ForeColor = System::Drawing::Color::Black;
        mDownTextBox->Enabled = true;
        mDownTextBox->ForeColor = System::Drawing::Color::Black;
    } else {
        mDownUnlimitedRadioButton->Checked = true;
        mDownLabel->ForeColor = System::Drawing::Color::Gray;
        mDownTextBox->Enabled = false;
        mDownTextBox->ForeColor = System::Drawing::Color::Gray;
    }

    // upload init

    if (CcdManager::Instance->Settings->LimitUpload == true) {
        if (CcdManager::Instance->Settings->LimitUploadAuto == true) {
            mUpLimitAutoRadioButton->Checked = true;
            mUpLabel->ForeColor = System::Drawing::Color::Gray;
            mUpTextBox->Enabled = false;
            mUpTextBox->ForeColor = System::Drawing::Color::Gray;
        } else {
            mUpLimitRadioButton->Checked = true;
            mUpLabel->ForeColor = System::Drawing::Color::Black;
            mUpTextBox->Enabled = true;
            mUpTextBox->ForeColor = System::Drawing::Color::Black;
        }
    } else {
        mUpUnlimitedRadioButton->Checked = true;
        mUpLabel->ForeColor = System::Drawing::Color::Gray;
        mUpTextBox->Enabled = false;
        mUpTextBox->ForeColor = System::Drawing::Color::Gray;
    }
}

void BandwidthTab::OnChangeDownRate(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->DownloadRate = Convert::ToInt32(mDownTextBox->Text);
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnChangeUpRate(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->UploadRate = Convert::ToInt32(mUpTextBox->Text);
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnClickDownLimit(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->LimitDownload = true;
    mDownLabel->ForeColor = System::Drawing::Color::Black;
    mDownTextBox->Enabled = true;
    mDownTextBox->ForeColor = System::Drawing::Color::Black;
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnClickDownUnlimit(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->LimitDownload = false;
    mDownLabel->ForeColor = System::Drawing::Color::Gray;
    mDownTextBox->Enabled = false;
    mDownTextBox->ForeColor = System::Drawing::Color::Gray;
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnClickUpLimit(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->LimitUpload = true;
    CcdManager::Instance->Settings->LimitUploadAuto = false;
    mUpLabel->ForeColor = System::Drawing::Color::Black;
    mUpTextBox->Enabled = true;
    mUpTextBox->ForeColor = System::Drawing::Color::Black;
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnClickUpLimitAuto(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->LimitUpload = true;
    CcdManager::Instance->Settings->LimitUploadAuto = true;
    mUpLabel->ForeColor = System::Drawing::Color::Gray;
    mUpTextBox->Enabled = false;
    mUpTextBox->ForeColor = System::Drawing::Color::Gray;
    mParent->SetApplyButton(true);
}

void BandwidthTab::OnClickUpUnlimit(Object^ sender, EventArgs^ e)
{
    CcdManager::Instance->Settings->LimitUpload = false;
    CcdManager::Instance->Settings->LimitUploadAuto = false;
    mUpUnlimitedRadioButton->Checked = true;
    mUpLabel->ForeColor = System::Drawing::Color::Gray;
    mUpTextBox->Enabled = false;
    mUpTextBox->ForeColor = System::Drawing::Color::Gray;
    mParent->SetApplyButton(true);
}