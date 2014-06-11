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

public ref class ProxyTab : public Panel
{
public:
    ProxyTab(SettingsForm^ parent);
    ~ProxyTab();

    void Apply();

private:
    SettingsForm^ mParent;
    CheckBox^     mRequiredCheckBox;
    ComboBox^     mTypeComboBox;
    GroupBox^     mSettingsGroupBox;
    Label^        mPasswordLabel;
    Label^        mPortLabel;
    Label^        mServerLabel;
    Label^        mTypeLabel;
    Label^        mUsernameLabel;
    RadioButton^  mAutoRadioButton;
    RadioButton^  mManualRadioButton;
    RadioButton^  mNoneRadioButton;
    TextBox^      mPasswordTextBox;
    TextBox^      mPortTextBox;
    TextBox^      mServerTextBox;
    TextBox^      mUsernameTextBox;

    void OnChangePassword(Object^ sender, EventArgs^ e);
    void OnChangePort(Object^ sender, EventArgs^ e);
    void OnChangeServer(Object^ sender, EventArgs^ e);
    void OnChangeType(Object^ sender, EventArgs^ e);
    void OnChangeUsername(Object^ sender, EventArgs^ e);
    void OnClickAuto(Object^ sender, EventArgs^ e);
    void OnClickManual(Object^ sender, EventArgs^ e);
    void OnClickNone(Object^ sender, EventArgs^ e);
    void OnClickRequirePassword(Object^ sender, EventArgs^ e);
};

