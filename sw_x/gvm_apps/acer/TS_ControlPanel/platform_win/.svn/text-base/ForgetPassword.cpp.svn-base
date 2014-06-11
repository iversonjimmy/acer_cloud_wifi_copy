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
#include "ForgetPassword.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"
#include "Layout.h"

using namespace System::Collections;
using namespace System::IO;

CForgetPassword::CForgetPassword(Form^ parent) : Panel()
{
    mParent = parent;

    // labels
    mTitleLabel = gcnew Label();
    mTitleLabel->AutoSize = true;
    mTitleLabel->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 10,
        System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mTitleLabel->TabIndex = 0;
	mTitleLabel->Text = LocalizedText::Instance->ForgetPasswordLabel;

	mDescription = gcnew TextBox();
    mDescription->AutoSize = true;
    mDescription->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 9,
		System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mDescription->TabIndex = 1;
	mDescription->BorderStyle = System::Windows::Forms::BorderStyle::None;
	mDescription->Text = LocalizedText::Instance->ForgetPasswordDescription;
	mDescription->Multiline = true;
    mDescription->Location = System::Drawing::Point(10, mTitleLabel->Size.Height + 20);
    mDescription->Size = System::Drawing::Size(500, 60);
	
    mEmailLabel = gcnew Label();
    mEmailLabel->AutoSize = true;
    mEmailLabel->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 9,
        System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mEmailLabel->TabIndex = 0;
    mEmailLabel->Text = LocalizedText::Instance->EmailLabel;

	mEmailTextBox = gcnew TextBox();  
    mEmailTextBox->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 9,
		System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    mEmailTextBox->TabIndex = 1;
	//mEmailTextBox->BorderStyle = System::Windows::Forms::BorderStyle::None;
    //mEmailTextBox->Text = LocalizedText::Instance->OtherDescription;
	//mEmailTextBox->Multiline = true;
	
	mBGPictureBox = gcnew PictureBox();
    mBGPictureBox->TabIndex = 4;
    mBGPictureBox->TabStop = false;
    mBGPictureBox->BackgroundImage = System::Drawing::Image::FromFile(L"images\\start_logo.png");
    mBGPictureBox->BackgroundImageLayout = ImageLayout::Stretch;

	//Set size and location
	int xPosition = 0;
	int yPosition = 0;

    mTitleLabel->Location = System::Drawing::Point(xPosition+=10, 0);
    mTitleLabel->Size = System::Drawing::Size(225, 20);

	mDescription->Location = System::Drawing::Point(xPosition, yPosition+=(mTitleLabel->Size.Height+ 5) );
    mDescription->Size = System::Drawing::Size(Panel_Width - 50, 100);

	mEmailLabel->Location = System::Drawing::Point(xPosition+=5, yPosition+= (mDescription->Size.Height + 5) );
    mEmailLabel->Size = System::Drawing::Size(Panel_Width - 100 , 20);

	mEmailTextBox->Location = System::Drawing::Point(xPosition, yPosition+= ( mEmailLabel->Size.Height + 2) );
    mEmailTextBox->Size = System::Drawing::Size(Panel_Width -100, 18);


	mBGPictureBox->Size = System::Drawing::Size(128, 103);
	mBGPictureBox->Location = System::Drawing::Point(Panel_Width -  mBGPictureBox->Size.Width - 50, Panel_Height - mBGPictureBox->Size.Height - 100);
    

    // panel
    this->Controls->Add(mTitleLabel);
    this->Controls->Add(mDescription);

	this->Controls->Add(mEmailLabel);
	this->Controls->Add(mEmailTextBox);
	this->Controls->Add(mBGPictureBox);

    this->Location = System::Drawing::Point(14, 14);
	this->Size = System::Drawing::Size(Panel_Width, Panel_Height);
    this->TabIndex = 1;
}

CForgetPassword::~CForgetPassword()
{
    // do nothing, memory is managed
}
