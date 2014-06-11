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
#include "register_panel.h"
#include "util_funcs.h"
#include "setup_form.h"
#include "ccd_manager.h"
#include "async_operations.h"
#include "localized_text.h"

#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Collections;
using namespace System::IO;

RegisterPanel::RegisterPanel(SetupForm^ parent) : Panel()
{
    mParent = parent;

    //Create 
	this->mLastNameTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mUsernameTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mLastNameLabel = (gcnew System::Windows::Forms::Label());
	this->pictureBox_Line = (gcnew System::Windows::Forms::PictureBox());
	this->mFistNameLabel = (gcnew System::Windows::Forms::Label());
	this->mEnterYourName = (gcnew System::Windows::Forms::Label());
	this->mTitleLabel = (gcnew System::Windows::Forms::Label());
	this->mEnterDetailLabel = (gcnew System::Windows::Forms::Label());
	this->mEmailLabel = (gcnew System::Windows::Forms::Label());
	this->mReenterEmailLabel = (gcnew System::Windows::Forms::Label());
	this->mEmailTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mConfirmEmailTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mPasswordLabel = (gcnew System::Windows::Forms::Label());
	this->mPasswordTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mValidatCodeLabel = (gcnew System::Windows::Forms::Label());

	//Add 
	this->BackColor = System::Drawing::Color::FromArgb(255, 245,245,245);
	this->Controls->Add(this->mPasswordTextBox);
	this->Controls->Add(this->mConfirmEmailTextBox);
	this->Controls->Add(this->mEmailTextBox);
	this->Controls->Add(this->mPasswordLabel);
	this->Controls->Add(this->mLastNameTextBox);
	this->Controls->Add(this->mReenterEmailLabel);
	this->Controls->Add(this->mUsernameTextBox);
	this->Controls->Add(this->mLastNameLabel);
	this->Controls->Add(this->mValidatCodeLabel);
	this->Controls->Add(this->mEmailLabel);
	this->Controls->Add(this->pictureBox_Line);
	this->Controls->Add(this->mFistNameLabel);
	this->Controls->Add(this->mEnterDetailLabel);
	this->Controls->Add(this->mEnterYourName);
	this->Controls->Add(this->mTitleLabel);
	this->Size = System::Drawing::Size(728, 424);

// 
			// mLastNameTextBox
			// 
			this->mLastNameTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mLastNameTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mLastNameTextBox->Location = System::Drawing::Point(211, 117);
			this->mLastNameTextBox->Name = L"mLastNameTextBox";
			this->mLastNameTextBox->Size = System::Drawing::Size(355, 27);
			this->mLastNameTextBox->TabIndex = 2;
			// 
			// mUsernameTextBox
			// 
			this->mUsernameTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mUsernameTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mUsernameTextBox->Location = System::Drawing::Point(211, 70);
			this->mUsernameTextBox->Name = L"mUsernameTextBox";
			this->mUsernameTextBox->Size = System::Drawing::Size(355, 27);
			this->mUsernameTextBox->TabIndex = 1;
			// 
			// mLastNameLabel
			// 
			this->mLastNameLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mLastNameLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mLastNameLabel->Location = System::Drawing::Point(25, 119);
			this->mLastNameLabel->Name = L"mLastNameLabel";
			this->mLastNameLabel->Size = System::Drawing::Size(168, 20);
			this->mLastNameLabel->TabIndex = 1;
			this->mLastNameLabel->Text = L"Last Name";
			this->mLastNameLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// pictureBox_Line
			// 
			this->pictureBox_Line->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(178)), static_cast<System::Int32>(static_cast<System::Byte>(178)), 
				static_cast<System::Int32>(static_cast<System::Byte>(178)));
			this->pictureBox_Line->Location = System::Drawing::Point(25, 168);
			this->pictureBox_Line->Name = L"pictureBox_Line";
			this->pictureBox_Line->Size = System::Drawing::Size(670, 1);
			this->pictureBox_Line->TabIndex = 2;
			this->pictureBox_Line->TabStop = false;
			// 
			// mFistNameLabel
			// 
			this->mFistNameLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mFistNameLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mFistNameLabel->Location = System::Drawing::Point(25, 70);
			this->mFistNameLabel->Name = L"mFistNameLabel";
			this->mFistNameLabel->Size = System::Drawing::Size(168, 20);
			this->mFistNameLabel->TabIndex = 1;
			this->mFistNameLabel->Text = L"First Name";
			this->mFistNameLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// mEnterYourName
			// 
			this->mEnterYourName->Font = (gcnew System::Drawing::Font(L"Arial", 12));
			this->mEnterYourName->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(145)), static_cast<System::Int32>(static_cast<System::Byte>(175)), 
				static_cast<System::Int32>(static_cast<System::Byte>(80)));
			this->mEnterYourName->Location = System::Drawing::Point(22, 42);
			this->mEnterYourName->Name = L"mEnterYourName";
			this->mEnterYourName->Size = System::Drawing::Size(385, 25);
			this->mEnterYourName->TabIndex = 1;
			this->mEnterYourName->Text = L"Enter your name:";
			// 
			// mTitleLabel
			// 
			this->mTitleLabel->Font = (gcnew System::Drawing::Font(L"Arial", 18));
			this->mTitleLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mTitleLabel->Location = System::Drawing::Point(21, 3);
			this->mTitleLabel->Name = L"mTitleLabel";
			this->mTitleLabel->Size = System::Drawing::Size(282, 30);
			this->mTitleLabel->TabIndex = 1;
			this->mTitleLabel->Text = L"Create an acer ID";
			// 
			// mEnterDetailLabel
			// 
			this->mEnterDetailLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12));
			this->mEnterDetailLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(145)), 
				static_cast<System::Int32>(static_cast<System::Byte>(175)), static_cast<System::Int32>(static_cast<System::Byte>(80)));
			this->mEnterDetailLabel->Location = System::Drawing::Point(23, 187);
			this->mEnterDetailLabel->Name = L"mEnterDetailLabel";
			this->mEnterDetailLabel->Size = System::Drawing::Size(599, 28);
			this->mEnterDetailLabel->TabIndex = 1;
			this->mEnterDetailLabel->Text = L"Enter your primary email address as your acer ID:";
			// 
			// mEmailLabel
			// 
			this->mEmailLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mEmailLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mEmailLabel->Location = System::Drawing::Point(25, 238);
			this->mEmailLabel->Name = L"mEmailLabel";
			this->mEmailLabel->Size = System::Drawing::Size(168, 20);
			this->mEmailLabel->TabIndex = 1;
			this->mEmailLabel->Text = L"Email Address";
			this->mEmailLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// mReenterEmailLabel
			// 
			this->mReenterEmailLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mReenterEmailLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mReenterEmailLabel->Location = System::Drawing::Point(25, 276);
			this->mReenterEmailLabel->Name = L"mReenterEmailLabel";
			this->mReenterEmailLabel->Size = System::Drawing::Size(168, 46);
			this->mReenterEmailLabel->TabIndex = 1;
			this->mReenterEmailLabel->Text = L"Re-enter Email Address";
			this->mReenterEmailLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// mEmailTextBox
			// 
			this->mEmailTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mEmailTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mEmailTextBox->Location = System::Drawing::Point(211, 238);
			this->mEmailTextBox->Name = L"mEmailTextBox";
			this->mEmailTextBox->Size = System::Drawing::Size(355, 27);
			this->mEmailTextBox->TabIndex = 3;
			// 
			// mConfirmEmailTextBox
			// 
			this->mConfirmEmailTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mConfirmEmailTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mConfirmEmailTextBox->Location = System::Drawing::Point(211, 285);
			this->mConfirmEmailTextBox->Name = L"mConfirmEmailTextBox";
			this->mConfirmEmailTextBox->Size = System::Drawing::Size(355, 27);
			this->mConfirmEmailTextBox->TabIndex = 4;
			// 
			// mPasswordLabel
			// 
			this->mPasswordLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mPasswordLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mPasswordLabel->Location = System::Drawing::Point(28, 335);
			this->mPasswordLabel->Name = L"mPasswordLabel";
			this->mPasswordLabel->Size = System::Drawing::Size(165, 20);
			this->mPasswordLabel->TabIndex = 1;
			this->mPasswordLabel->Text = L"Password";
			this->mPasswordLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			mPasswordTextBox->PasswordChar = L'*';
			// 
			// mPasswordTextBox
			// 
			this->mPasswordTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mPasswordTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mPasswordTextBox->Location = System::Drawing::Point(211, 333);
			this->mPasswordTextBox->Name = L"mPasswordTextBox";
			this->mPasswordTextBox->Size = System::Drawing::Size(355, 27);
			this->mPasswordTextBox->TabIndex = 5;
			// 
			// mValidatCodeLabel
			// 
			this->mValidatCodeLabel->Font = (gcnew System::Drawing::Font(L"Arial", 10));
			this->mValidatCodeLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mValidatCodeLabel->Location = System::Drawing::Point(572, 213);
			this->mValidatCodeLabel->Name = L"mValidatCodeLabel";
			this->mValidatCodeLabel->Size = System::Drawing::Size(153, 77);
			this->mValidatCodeLabel->TabIndex = 1;
			this->mValidatCodeLabel->Text = L"A validation code will be sent to this address";
			this->mValidatCodeLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;


	
    mConfirmEmailTextBox->TextChanged += gcnew EventHandler(this, &RegisterPanel::OnChangeConfirm);

    mEmailTextBox->TextChanged += gcnew EventHandler(this, &RegisterPanel::OnChangeEmail);

    mPasswordTextBox->TextChanged += gcnew EventHandler(this, &RegisterPanel::OnChangePassword);

    mUsernameTextBox->TextChanged += gcnew EventHandler(this, &RegisterPanel::OnChangeUsername);


	mDeviceName = gcnew String(Environment::MachineName->ToLower());

    // async operations

    AsyncOperations::Instance->RegisterComplete += gcnew RegisterHandler(this, &RegisterPanel::RegisterComplete);
    // panel

    this->Location = System::Drawing::Point(14, 67);
    this->TabIndex = 0;
}

RegisterPanel::~RegisterPanel()
{
    // do nothing, memory is managed
}

bool RegisterPanel::IsEnableSubmit::get()
{
	bool goodDevice = (mDeviceName != nullptr && mDeviceName->Length > 0);
    bool goodEmail = (mEmailTextBox->Text != nullptr && mEmailTextBox->Text->Length > 0);
    bool goodPass = (mPasswordTextBox->Text != nullptr && mPasswordTextBox->Text->Length > 0);
    bool goodConfirm = (mConfirmEmailTextBox->Text != nullptr && mConfirmEmailTextBox->Text->Length > 0);
    bool goodUser = (mUsernameTextBox->Text != nullptr && mUsernameTextBox->Text->Length > 0);

    return goodDevice && goodEmail && goodPass && goodConfirm && goodUser;
}

void RegisterPanel::EnableSubmit()
{
    mParent->EnableSubmit(this->IsEnableSubmit);
}

bool RegisterPanel::Register()
{
    // check input

    bool badConfirm = (mConfirmEmailTextBox->Text == nullptr || mConfirmEmailTextBox->Text->Length <= 0);
    bool badDevice = (mDeviceName== nullptr || mDeviceName->Length <= 0);
    bool badEmail = (mEmailTextBox->Text == nullptr && mEmailTextBox->Text->Length <= 0);
    bool badPass = (mPasswordTextBox->Text == nullptr || mPasswordTextBox->Text->Length <= 0);
    bool badUser = (mUsernameTextBox->Text == nullptr || mUsernameTextBox->Text->Length <= 0);

    if (badConfirm || badDevice || badEmail || badPass || badUser) {
        MessageBox::Show(L"All fields need to be filled in.",
            L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return false;
    }

    if (mEmailTextBox->Text != mConfirmEmailTextBox->Text) {
        MessageBox::Show(L"Email do not match.",
            L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return false;
    }

    if (mPasswordTextBox->Text->Length < 6) {
        MessageBox::Show(L"Passwords must be at least 6 characters long.",
            L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return false;
    }

    return AsyncOperations::Instance->Register(
        mUsernameTextBox->Text,
        mPasswordTextBox->Text,
        mDeviceName,
        mEmailTextBox->Text,
        false);
}

void RegisterPanel::RegisterComplete()
{
   RegisterComplete(gcnew String(""), nullptr,nullptr);
}



void RegisterPanel::RegisterComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (result != L"") {
        MessageBox::Show(result, L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return;
    }

	mEMailCompleted = mEmailTextBox->Text ;
	///clean textbox ====
    mConfirmEmailTextBox->Text = L"";
    mDeviceName = Environment::MachineName->ToLower();
    mEmailTextBox->Text = L"";
    mPasswordTextBox->Text = L"";
    mUsernameTextBox->Text = L"";
	mLastNameTextBox->Text = L"";
	//======

    mParent->Advance(nodes, filters);
}

void RegisterPanel::OnChangeConfirm(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

//void RegisterPanel::OnChangeDeviceName(Object^ sender, EventArgs^ e)
//{
//    String^ newName = UtilFuncs::StripChars(mLastNameTextBox->Text, L"<>:\"/\\|?*+[]\t\n\r");
//    if (mLastNameTextBox->Text != newName) {
//        mLastNameTextBox->Text = newName;
//    }
//
//    this->EnableSubmit();
//}

void RegisterPanel::OnChangeEmail(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void RegisterPanel::OnChangeKey(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void RegisterPanel::OnChangePassword(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void RegisterPanel::OnChangeUsername(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

//void RegisterPanel::OnClickPrivacy(Object^ sender, EventArgs^ e)
//{
//    UtilFuncs::ShowUrl(CcdManager::Instance->UrlPrivacy);
//}
//
//void RegisterPanel::OnClickTermsService(Object^ sender, EventArgs^ e)
//{
//    UtilFuncs::ShowUrl(CcdManager::Instance->UrlTermsService);
//}
