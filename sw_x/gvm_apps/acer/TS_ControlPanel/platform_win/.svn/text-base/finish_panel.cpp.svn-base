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
#include "finish_panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"
#include "Layout.h"

using namespace System::Collections;
using namespace System::IO;

FinishPanel::FinishPanel(Form^ parent) : Panel()
{
    mParent = parent;


		this->mPhoneLabel = (gcnew System::Windows::Forms::Label());
		this->mDescription = (gcnew System::Windows::Forms::Label());
		this->mTitle = (gcnew System::Windows::Forms::Label());
		this->pictureBox_AndroidPhone = (gcnew System::Windows::Forms::PictureBox());
		this->mQBCodeDesc = (gcnew System::Windows::Forms::Label());
		this->mLinkDescLabel = (gcnew System::Windows::Forms::Label());
		this->mOtherLabel = (gcnew System::Windows::Forms::Label());
		this->pictureBox_AndroidPTablet = (gcnew System::Windows::Forms::PictureBox());
		this->mTabletLabel = (gcnew System::Windows::Forms::Label());

		this->BackColor = System::Drawing::Color::FromArgb(255, 245, 245, 245); 
		this->Controls->Add(this->mOtherLabel);
		this->Controls->Add(this->mQBCodeDesc);
		this->Controls->Add(this->mLinkDescLabel);
		this->Controls->Add(this->mTabletLabel);
		this->Controls->Add(this->mPhoneLabel);
		this->Controls->Add(this->mDescription);
		this->Controls->Add(this->mTitle);
		this->Controls->Add(this->pictureBox_AndroidPTablet);
		this->Controls->Add(this->pictureBox_AndroidPhone);		
		this->Name = L"panel1";
		this->Size = System::Drawing::Size(728, 424);
		this->TabIndex = 3;
		this->Location = System::Drawing::Point(14, 67);
	// 
		// mOtherLabel
		// 
		this->mOtherLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12));
		this->mOtherLabel->ForeColor = System::Drawing::Color::FromArgb(255, 145, 175, 80);
		this->mOtherLabel->Location = System::Drawing::Point(27, 319);
		this->mOtherLabel->Name = L"mOtherLabel";
		this->mOtherLabel->Size = System::Drawing::Size(599, 28);
		this->mOtherLabel->TabIndex = 6;
		this->mOtherLabel->Text = L"Others devices:";
		// 
		// mQBCodeDesc
		// 
		this->mQBCodeDesc->Font = (gcnew System::Drawing::Font(L"Arial", 12));
		this->mQBCodeDesc->ForeColor = System::Drawing::Color::FromArgb(255, 145, 175, 80);
		this->mQBCodeDesc->Location = System::Drawing::Point(27, 97);
		this->mQBCodeDesc->Name = L"mQBCodeDesc";
		this->mQBCodeDesc->Size = System::Drawing::Size(599, 28);
		this->mQBCodeDesc->TabIndex = 6;
		this->mQBCodeDesc->Text = L"acerCloud QR code for Android platform:";
		// 
		// mLinkDescLabel
		// 
		this->mLinkDescLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
		this->mLinkDescLabel->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
		this->mLinkDescLabel->Location = System::Drawing::Point(27, 347);
		this->mLinkDescLabel->Name = L"mLinkDescLabel";
		this->mLinkDescLabel->Size = System::Drawing::Size(599, 61);
		this->mLinkDescLabel->TabIndex = 1;
		this->mLinkDescLabel->Text = L"For other devices, check them out here: http://www.acer.com/download/......(TBD)";
		// 
		// mTabletLabel
		// 
		this->mTabletLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
		this->mTabletLabel->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
		this->mTabletLabel->Location = System::Drawing::Point(432, 279);
		this->mTabletLabel->Name = L"mTabletLabel";
		this->mTabletLabel->Size = System::Drawing::Size(142, 20);
		this->mTabletLabel->TabIndex = 1;
		this->mTabletLabel->Text = L"Android Tablet";
		this->mTabletLabel->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
		// 
		// mPhoneLabel
		// 
		this->mPhoneLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
		this->mPhoneLabel->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
		this->mPhoneLabel->Location = System::Drawing::Point(118, 279);
		this->mPhoneLabel->Name = L"mPhoneLabel";
		this->mPhoneLabel->Size = System::Drawing::Size(142, 20);
		this->mPhoneLabel->TabIndex = 1;
		this->mPhoneLabel->Text = L"Android Phone";
		this->mPhoneLabel->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
		// 
		// mDescription
		// 
		this->mDescription->Font = (gcnew System::Drawing::Font(L"Arial", 11));
		this->mDescription->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
		this->mDescription->Location = System::Drawing::Point(27, 51);
		this->mDescription->Name = L"mDescription";
		this->mDescription->Size = System::Drawing::Size(634, 46);
		this->mDescription->TabIndex = 1;
		this->mDescription->Text = L"Congratulations! Your acer ID is successfully created!\r\nYou can install acerCloud" 
			L" clients on your devices and sync them with acerCloud.";
		// 
		// mTitle
		// 
		this->mTitle->Font = (gcnew System::Drawing::Font(L"Arial", 18));
		this->mTitle->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
		this->mTitle->Location = System::Drawing::Point(25, 18);
		this->mTitle->Name = L"mTitle";
		this->mTitle->Size = System::Drawing::Size(282, 33);
		this->mTitle->TabIndex = 1;
		this->mTitle->Text = L"acer ID successfully Created";
		// 
		// pictureBox_AndroidPTablet
		// 
		this->pictureBox_AndroidPTablet->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\QR_sample_138x138.png");
		this->pictureBox_AndroidPTablet->Location = System::Drawing::Point(436, 138);
		this->pictureBox_AndroidPTablet->Name = L"pictureBox_AndroidPTablet";
		this->pictureBox_AndroidPTablet->Size = System::Drawing::Size(138, 138);
		this->pictureBox_AndroidPTablet->TabIndex = 0;
		this->pictureBox_AndroidPTablet->TabStop = false;
		// 
		// pictureBox_AndroidPhone
		// 
		this->pictureBox_AndroidPhone->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\QR_sample_138x138.png");
		this->pictureBox_AndroidPhone->Location = System::Drawing::Point(121, 138);
		this->pictureBox_AndroidPhone->Name = L"pictureBox_AndroidPhone";
		this->pictureBox_AndroidPhone->Size = System::Drawing::Size(138, 138);
		this->pictureBox_AndroidPhone->TabIndex = 0;
		this->pictureBox_AndroidPhone->TabStop = false;
}



FinishPanel::~FinishPanel()
{
    // do nothing, memory is managed
}
