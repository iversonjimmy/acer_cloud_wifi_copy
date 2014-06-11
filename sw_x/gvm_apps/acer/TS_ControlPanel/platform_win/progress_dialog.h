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
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

public ref class ProgressDialog : public Form
{
public:
	ProgressDialog();
    ~ProgressDialog();

    property String^ ProgressLabel { void set(String^ value); }
    property int ProgressValue { void set(int value); }

private:
    Label^       mLabel;
    ProgressBar^ mProgressBar;

    void OnClose(Object^ sender, CancelEventArgs^ e);
};
