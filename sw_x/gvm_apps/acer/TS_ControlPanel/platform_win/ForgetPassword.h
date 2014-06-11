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

public ref class CForgetPassword : public Panel
{
public:
    CForgetPassword(Form^ parent);	
    ~CForgetPassword();
	

private:
    Form^				mParent;
    Label^				mTitleLabel;
	Label^				mEmailLabel;
 	TextBox^			mDescription;	
	TextBox^			mEmailTextBox;
	
	PictureBox^		mBGPictureBox;

	
};