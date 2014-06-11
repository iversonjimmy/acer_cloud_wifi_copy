//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#pragma once

#include "json_filter.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

ref class SetupForm;

public ref class RegisterPanel : public Panel
{
public:
    RegisterPanel(SetupForm^ parent);
    ~RegisterPanel();

    property bool IsEnableSubmit { bool get(); }
	property System::String^ GetRegisteredEmail { System::String^ get(){if(mEMailCompleted != nullptr) return mEMailCompleted; else return nullptr;}; }

    bool Register();

private:
    SetupForm^        mParent;
	System::Windows::Forms::Label^  mTitleLabel;
	System::Windows::Forms::Label^  mEnterYourName;
	System::Windows::Forms::Label^  mFistNameLabel;
	System::Windows::Forms::Label^  mLastNameLabel;
	System::Windows::Forms::Label^  mEnterDetailLabel;
	System::Windows::Forms::Label^  mValidatCodeLabel;
	System::Windows::Forms::Label^  mEmailLabel;
	System::Windows::Forms::Label^  mReenterEmailLabel;
	System::Windows::Forms::Label^  mPasswordLabel;

	System::Windows::Forms::PictureBox^  pictureBox_Line;

	System::Windows::Forms::TextBox^  mLastNameTextBox;
	System::Windows::Forms::TextBox^  mUsernameTextBox;
	System::Windows::Forms::TextBox^  mPasswordTextBox;
	System::Windows::Forms::TextBox^  mConfirmEmailTextBox;
	System::Windows::Forms::TextBox^  mEmailTextBox;

	String^					mDeviceName;
	String^					mEMailCompleted;
    void EnableSubmit();
    void RegisterComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
    void RegisterComplete();
    void OnChangeConfirm(Object^ sender, EventArgs^ e);
    //void OnChangeDeviceName(Object^ sender, EventArgs^ e);
    void OnChangeEmail(Object^ sender, EventArgs^ e);
    void OnChangeKey(Object^ sender, EventArgs^ e);
    void OnChangePassword(Object^ sender, EventArgs^ e);
    void OnChangeUsername(Object^ sender, EventArgs^ e);
    //void OnClickPrivacy(Object^ sender, EventArgs^ e);
    //void OnClickTermsService(Object^ sender, EventArgs^ e);
};
