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

public ref class LoginPanel : public Panel
{
public:
    LoginPanel(SetupForm^ parent);
    ~LoginPanel();

    property bool IsEnableSubmit { bool get(); }

    bool Login();

private:
    SetupForm^        mParent;
    CheckBox^         mCheckBox; // HACK
    Label^            mDeviceNameLabel;
    Label^            mPasswordLabel;
    Label^            mTitleLabel;
    Label^            mUsernameLabel;
    LinkLabel^        mForgetPassLinkLabel;
    TextBox^          mPasswordTextBox;
    TextBox^          mUsernameTextBox;
    TextBox^          mDeviceNameTextBox;

    void EnableSubmit();
    void LoginComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
    void OnChangeDeviceName(Object^ sender, EventArgs^ e);
    void OnChangePassword(Object^ sender, EventArgs^ e);
    void OnChangeUsername(Object^ sender, EventArgs^ e);
    void OnClickForgetPass(Object^ sender, EventArgs^ e);
};
