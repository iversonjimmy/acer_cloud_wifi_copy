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
#include "start_panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"

using namespace System::Collections;
using namespace System::IO;

StartPanel::StartPanel(Form^ parent) : Panel()
{
    mParent = parent;

    // labels

    mLogoLabel = gcnew Label();
    mLogoLabel->AutoSize = true;
    mLogoLabel->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 24,
        System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mLogoLabel->Location = System::Drawing::Point(193, 48);
    mLogoLabel->Size = System::Drawing::Size(263, 39);
    mLogoLabel->TabIndex = 0;
    mLogoLabel->Text = LocalizedText::Instance->LogoLabel;

    // picture box

    mPictureBox = gcnew PictureBox();
    mPictureBox->Location = System::Drawing::Point(57, 18);
    mPictureBox->Size = System::Drawing::Size(128, 103);
    mPictureBox->TabIndex = 4;
    mPictureBox->TabStop = false;
    mPictureBox->BackgroundImage = System::Drawing::Image::FromFile(L"images\\start_logo.png");
    mPictureBox->BackgroundImageLayout = ImageLayout::Stretch;

    // radio buttons

    mNewUserRadioButton = gcnew RadioButton();
    mNewUserRadioButton->AutoSize = true;
    mNewUserRadioButton->Location = System::Drawing::Point(140, 157);
    mNewUserRadioButton->Size = System::Drawing::Size(159, 19);
    mNewUserRadioButton->TabIndex = 1;
    mNewUserRadioButton->TabStop = true;
    mNewUserRadioButton->Text = LocalizedText::Instance->NewUserRadioButton;
    mNewUserRadioButton->UseVisualStyleBackColor = true;
    mNewUserRadioButton->Checked = true;

    mOldUserRadioButton = gcnew RadioButton();
    mOldUserRadioButton->AutoSize = true;
    mOldUserRadioButton->Location = System::Drawing::Point(140, 198);
    mOldUserRadioButton->Size = System::Drawing::Size(163, 19);
    mOldUserRadioButton->TabIndex = 2;
    mOldUserRadioButton->TabStop = true;
    mOldUserRadioButton->Text = LocalizedText::Instance->OldUserRadioButton;
    mOldUserRadioButton->UseVisualStyleBackColor = true;

    // panel

    this->Controls->Add(mOldUserRadioButton);
    this->Controls->Add(mNewUserRadioButton);
    this->Controls->Add(mLogoLabel);
    this->Controls->Add(mPictureBox);
    this->Location = System::Drawing::Point(14, 14);
    this->Size = System::Drawing::Size(436, 315);
    this->TabIndex = 0;
}

StartPanel::~StartPanel()
{
    // do nothing, memory is managed
}

bool StartPanel::HasAccount::get()
{
    return mOldUserRadioButton->Checked;
}
