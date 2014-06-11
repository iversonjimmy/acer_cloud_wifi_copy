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

public ref class BandwidthTab : public Panel
{
public:
    BandwidthTab(SettingsForm^ parent);
    ~BandwidthTab();

    void Apply();

private:
    SettingsForm^ mParent;
    GroupBox^     mDownGroupBox;
    GroupBox^     mUpGroupBox;
    Label^        mDownLabel;
    Label^        mUpLabel;
    RadioButton^  mDownLimitRadioButton;
    RadioButton^  mDownUnlimitedRadioButton;
    RadioButton^  mUpLimitRadioButton;
    RadioButton^  mUpLimitAutoRadioButton;
    RadioButton^  mUpUnlimitedRadioButton;
    TextBox^      mDownTextBox;
    TextBox^      mUpTextBox;

    void OnChangeDownRate(Object^ sender, EventArgs^ e);
    void OnChangeUpRate(Object^ sender, EventArgs^ e);
    void OnClickDownLimit(Object^ sender, EventArgs^ e);
    void OnClickDownUnlimit(Object^ sender, EventArgs^ e);
    void OnClickUpLimit(Object^ sender, EventArgs^ e);
    void OnClickUpLimitAuto(Object^ sender, EventArgs^ e);
    void OnClickUpUnlimit(Object^ sender, EventArgs^ e);
};
