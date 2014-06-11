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

public ref class GeneralTab : public Panel
{
public:
    GeneralTab(SettingsForm^ parent);
    ~GeneralTab();

    void Apply();
    void Reset();

private:
    SettingsForm^ mParent;
    CheckBox^     mNotifyCheckBox;
    CheckBox^     mStartupCheckBox;
    ComboBox^     mLanguageComboBox;
    GroupBox^     mLanguageGroupBox;
    GroupBox^     mMiscGroupBox;

    void OnChangeLanguage(Object^ sender, EventArgs^ e);
    void OnClickNotify(Object^ sender, EventArgs^ e);
    void OnClickStartup(Object^ sender, EventArgs^ e);
};
