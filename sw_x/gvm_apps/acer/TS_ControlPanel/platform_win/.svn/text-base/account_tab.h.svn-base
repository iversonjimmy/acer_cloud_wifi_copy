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

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

ref class SettingsForm;

public ref class AccountTab : public Panel
{
public:
    AccountTab(::SettingsForm^ parent);
    ~AccountTab();

    void Apply();
    void Reset();

private:
    SettingsForm^ mParent;
    Button^       mLogoutButton;
    GroupBox^     mDeviceNameGroupBox;
    GroupBox^     mLogoutGroupBox;
    Label^        mLogoutLabel;
    TextBox^      mDeviceNameTextBox;

    void OnChangeDeviceName(Object^ sender, EventArgs^ e);
    void OnClickLogout(Object^ sender, EventArgs^ e);
    void LogoutComplete(String^ result);
};