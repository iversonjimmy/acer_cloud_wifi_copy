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

public ref class StartPanel : public Panel
{
public:
    StartPanel(Form^ parent);
    ~StartPanel();

    property bool HasAccount { bool get(); }

private:
    Form^        mParent;
    Label^       mLogoLabel;
    PictureBox^  mPictureBox;
    RadioButton^ mNewUserRadioButton;
    RadioButton^ mOldUserRadioButton;
};