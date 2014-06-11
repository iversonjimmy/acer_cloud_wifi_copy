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
#include "Signup_panel.h"
#include "util_funcs.h"
#include "setup_form.h"
#include "ccd_manager.h"
#include "async_operations.h"
#include "localized_text.h"
#include "Layout.h"

#include "WrapLabel.h"

#using <System.dll>

using namespace System::Diagnostics;
using namespace System::Collections;
using namespace System::IO;

SignupPanel::SignupPanel(SetupForm^ parent) : Panel()
{
    mParent = parent;
	

	//Create
	this->pictureBox_Cloud = (gcnew System::Windows::Forms::PictureBox());
	
	this->mTitleLabel = (gcnew System::Windows::Forms::Label());
	this->mDescription = (gcnew System::Windows::Forms::Label());
	this->pictureBox_Line = (gcnew System::Windows::Forms::PictureBox());
	this->mSignInLabel = (gcnew System::Windows::Forms::Label());
	this->mAccountName = (gcnew System::Windows::Forms::Label());
	this->mUsernameTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mPasswordLabel = (gcnew System::Windows::Forms::Label());
	this->mPasswordTextBox = (gcnew System::Windows::Forms::TextBox());
	this->mForgetPassLinkLabel = (gcnew System::Windows::Forms::LinkLabel());
	this->mSignUpButton = (gcnew CMyButton());

	//Add
	this->BackColor = System::Drawing::Color::FromArgb(255, 245,245,245);
	this->Controls->Add(this->mSignUpButton);
	this->Controls->Add(this->mForgetPassLinkLabel);
	this->Controls->Add(this->mPasswordTextBox);
	this->Controls->Add(this->mUsernameTextBox);
	this->Controls->Add(this->mPasswordLabel);
	this->Controls->Add(this->pictureBox_Line);
	this->Controls->Add(this->mAccountName);
	this->Controls->Add(this->mDescription);
	this->Controls->Add(this->mSignInLabel);
	this->Controls->Add(this->mTitleLabel);
	this->Controls->Add(this->pictureBox_Cloud);
	this->Name = L"panel1";
	this->Size = System::Drawing::Size(728, 424);			
	// 
	// mSignUpButton
	// 
	this->mSignUpButton->Font = (gcnew System::Drawing::Font(L"Arial", 11, System::Drawing::FontStyle::Regular));
	this->mSignUpButton->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mSignUpButton->Location = System::Drawing::Point(335, 124);
	this->mSignUpButton->Name = L"mSignUpButton";
	this->mSignUpButton->Size = System::Drawing::Size(102, 32);
	this->mSignUpButton->TabIndex = 1;
	this->mSignUpButton->Text = L"Sign up";

	// 
		// mForgetPassLinkLabel
		// 
	this->mForgetPassLinkLabel->AutoSize = true;
	this->mForgetPassLinkLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
	this->mForgetPassLinkLabel->LinkColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(52)), 
		static_cast<System::Int32>(static_cast<System::Byte>(145)), static_cast<System::Int32>(static_cast<System::Byte>(218)));
	this->mForgetPassLinkLabel->Location = System::Drawing::Point(481, 396); 
	this->mForgetPassLinkLabel->Name = L"mForgetPassLinkLabel";
	this->mForgetPassLinkLabel->Size = System::Drawing::Size(126, 17);
	this->mForgetPassLinkLabel->TabIndex = 5;
	this->mForgetPassLinkLabel->TabStop = true;
	this->mForgetPassLinkLabel->Text = L"Forgot password\?";
	// 
	// mPasswordTextBox
	// 
	this->mPasswordTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
		static_cast<System::Byte>(0)));
	this->mPasswordTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mPasswordTextBox->Location = System::Drawing::Point(336, 339);
	this->mPasswordTextBox->Name = L"mPasswordTextBox";
	this->mPasswordTextBox->Size = System::Drawing::Size(364, 27);
	this->mPasswordTextBox->TabIndex = 3;
			 mPasswordTextBox->PasswordChar = L'*';
	// 
	// mUsernameTextBox
	// 
	this->mUsernameTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
		static_cast<System::Byte>(0)));
	this->mUsernameTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mUsernameTextBox->Location = System::Drawing::Point(336, 276);
	this->mUsernameTextBox->Name = L"mUsernameTextBox";
	this->mUsernameTextBox->Size = System::Drawing::Size(364, 27);
	this->mUsernameTextBox->TabIndex = 2;
	// 
	// mPasswordLabel
	// 
	this->mPasswordLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
	this->mPasswordLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mPasswordLabel->Location = System::Drawing::Point(337, 317);
	this->mPasswordLabel->Name = L"mPasswordLabel";
	this->mPasswordLabel->Size = System::Drawing::Size(142, 20);
	this->mPasswordLabel->TabIndex = 11;
	this->mPasswordLabel->Text = L"Password";
	// 
	// pictureBox_Line
	// 
	this->pictureBox_Line->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(178)), static_cast<System::Int32>(static_cast<System::Byte>(178)), 
		static_cast<System::Int32>(static_cast<System::Byte>(178)));
	this->pictureBox_Line->Location = System::Drawing::Point(335, 190);
	this->pictureBox_Line->Name = L"pictureBox_Line";
	this->pictureBox_Line->Size = System::Drawing::Size(365, 1);
	this->pictureBox_Line->TabIndex = 12;
	this->pictureBox_Line->TabStop = false;
	// 
	// mAccountName
	// 
	this->mAccountName->Font = (gcnew System::Drawing::Font(L"Arial", 11));
	this->mAccountName->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mAccountName->Location = System::Drawing::Point(337, 253);
	this->mAccountName->Name = L"mAccountName";
	this->mAccountName->Size = System::Drawing::Size(142, 20);
	this->mAccountName->TabIndex = 13;
	this->mAccountName->Text = L"Email (acer ID)";
	// 
	// mDescription
	// 
	this->mDescription->Font = (gcnew System::Drawing::Font(L"Arial", 11));
	this->mDescription->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mDescription->Location = System::Drawing::Point(335, 74);
	this->mDescription->Name = L"mDescription";
	this->mDescription->Size = System::Drawing::Size(393, 46);
	this->mDescription->TabIndex = 14;
	this->mDescription->Text = L"Sign up an acer ID to enjoy acerCloud service. It is simple and only takes a minu" 
		L"te to complete!";
	// 
	// mSignInLabel
	// 
	this->mSignInLabel->Font = (gcnew System::Drawing::Font(L"Arial", 18));
	this->mSignInLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mSignInLabel->Location = System::Drawing::Point(334, 217);
	this->mSignInLabel->Name = L"mSignInLabel";
	this->mSignInLabel->Size = System::Drawing::Size(366, 33);
	this->mSignInLabel->TabIndex = 15;
	this->mSignInLabel->Text = L"Sign in to acerCloud!";
	// 
	// mTitleLabel
	// 
	this->mTitleLabel->Font = (gcnew System::Drawing::Font(L"Arial", 18));
	this->mTitleLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mTitleLabel->Location = System::Drawing::Point(335, 33);
	this->mTitleLabel->Name = L"mTitleLabel";
	this->mTitleLabel->Size = System::Drawing::Size(282, 33);
	this->mTitleLabel->TabIndex = 16;
	this->mTitleLabel->Text = L"New to acerCloud\?";
	// 
	// pictureBox_Cloud
	// 
	this->pictureBox_Cloud->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\logo.png");
	this->pictureBox_Cloud->Location = System::Drawing::Point(30, 144);
	this->pictureBox_Cloud->Name = L"pictureBox_Cloud";
	this->pictureBox_Cloud->Size = System::Drawing::Size(260, 145);
	this->pictureBox_Cloud->TabIndex = 17;
	this->pictureBox_Cloud->TabStop = false;			

    // check boxes (HACK)
    mForgetPassLinkLabel->Click += gcnew EventHandler(this, &SignupPanel::OnClickForgetPass);

	mSignUpButton->OnClickedEvent += gcnew CMyButton::OnClickedHandler(this,  &SignupPanel::OnClickCreateAccount);
    // text boxes

    mPasswordTextBox->TextChanged += gcnew EventHandler(this, &SignupPanel::OnChangePassword);

    mUsernameTextBox->TextChanged += gcnew EventHandler(this, &SignupPanel::OnChangeUsername);


    // async operations

    AsyncOperations::Instance->LoginComplete += gcnew LoginHandler(this, &SignupPanel::LoginComplete);

    // panel
    this->Location = System::Drawing::Point(14, 48);

	mDeviceName = gcnew String(Environment::MachineName->ToLower());

}

SignupPanel::~SignupPanel()
{
    // do nothing, memory is managed
}

bool SignupPanel::IsEnableSubmit::get()
{

    bool goodPass = (mPasswordTextBox->Text != nullptr && mPasswordTextBox->Text->Length >= 6);
    bool goodUser = (mUsernameTextBox->Text != nullptr && mUsernameTextBox->Text->Length > 0);

    return goodPass && goodUser;
}

void SignupPanel::EnableSubmit()
{
    mParent->EnableSubmit(this->IsEnableSubmit);
}

bool SignupPanel::Login()
{
    // check input
    bool badDevice = (mDeviceName == nullptr || mDeviceName->Length <= 0);
	bool badPass = (mPasswordTextBox->Text == nullptr || mPasswordTextBox->Text->Length < 6);
    bool badUser = (mUsernameTextBox->Text == nullptr || mUsernameTextBox->Text->Length <= 0);

    if (badPass || badUser || badDevice) {
        MessageBox::Show(L"All fields need to be filled in.",
            L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
        return false;
    }

    return AsyncOperations::Instance->Login(
        mUsernameTextBox->Text,
        mPasswordTextBox->Text,
        mDeviceName,
        false);
}

void SignupPanel::LoginComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (result != L"") {
		if(!result->Contains(L"32232"))
		{
			MessageBox::Show(L"The user name or password you entered is incorrect", L"Warning",MessageBoxButtons::OK, MessageBoxIcon::Exclamation);
			//MessageBox::Show(result, L"", MessageBoxButtons::OK, MessageBoxIcon::Stop);
			return;
		}
    }

    mPasswordTextBox->Text = L"";
    mUsernameTextBox->Text = L"";    
    mParent->Advance(nodes, filters);
}

void SignupPanel::OnChangePassword(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void SignupPanel::OnChangeUsername(Object^ sender, EventArgs^ e)
{
    this->EnableSubmit();
}

void SignupPanel::OnClickForgetPass(Object^ sender, EventArgs^ e)
{
    //UtilFuncs::ShowUrl(CcdManager::Instance->UrlForgetPassword);
	mParent->ChangeState(StateEvent::ForgetPassword);
}

 void SignupPanel::OnClickCreateAccount()//Object^ sender, EventArgs^ e)
{
	mParent->ChangeState(StateEvent::CreateAccount);
 }
