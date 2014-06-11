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
#include "MyButton.h"
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

ref class SetupForm;

public ref class SignupPanel : public Panel
{
public:
    SignupPanel(SetupForm^ parent);
    ~SignupPanel();

    property bool IsEnableSubmit { bool get(); }

    bool Login();

private:
    SetupForm^			mParent;
	Label^  mDescription;
	Label^  mTitleLabel;
	Label^  mAccountName;
	Label^  mSignInLabel;
	
	PictureBox^  pictureBox_Cloud;
	PictureBox^  pictureBox_Line;
	
	LinkLabel^  mForgetPassLinkLabel;

	TextBox^  mPasswordTextBox;
	TextBox^  mUsernameTextBox;

	Label^  mPasswordLabel;
	CMyButton^  mSignUpButton;

	String^ mDeviceName;

    void EnableSubmit();
    void LoginComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);   
    void OnChangePassword(Object^ sender, EventArgs^ e);
    void OnChangeUsername(Object^ sender, EventArgs^ e);
    void OnClickForgetPass(Object^ sender, EventArgs^ e);
	void OnClickCreateAccount();//Object^ sender, EventArgs^ e);
};
