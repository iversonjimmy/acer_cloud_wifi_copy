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




#include "ccd_manager.h"


#include "Signup_panel.h"

#include "register_panel.h"
#include "start_panel.h"
//#include "sync_panel.h"
#include "finish_panel.h"
#include "StateFlow.h"
#include "TabController.h"
#include "ForgetPassword.h"
#include "FileSystemMonitor.h"
#include "MyButton.h"
#include "VerifyID.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;




public ref class SetupForm : public Form
{
public:
	SetupForm();
    ~SetupForm();

    void Advance(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
    void EnableSubmit(bool value);
	void ChangeState(StateEvent stateEvent );
	void ResetSettingsComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
	void ApplySettingsComplete(String^ result);
	bool IsSyncPanelState();
	//void MessageProc(Message %m);


protected:
	void LoginState();
	void RegistrationState();
	void FinishState(bool bFinishStyle);
	void ForgetPasswordState();	
	void TabControlState();
//	virtual void WndProc( Message% m ) override;

private:
    Panel^ mPanel;
    CMyButton^ mNextButton;
    CMyButton^ mCancelButton;	
	PictureBox^  mClosePictureBox;
//	CMediaController * mMediaController;

	SignupPanel^ mLoginPanel;

    RegisterPanel^	mRegisterPanel;
    StartPanel^			mStartPanel;
    CVerifyID^			mVerifyIDPanel;
	FinishPanel^		mFinishPanel;

	CTabController^	mTabController;
	CForgetPassword^ mForgetPassword;

    void OnClickCancel();//Object^ sender, EventArgs^ e);
	void mClosePictureBox_Click(System::Object^  sender, System::EventArgs^  e);
    void OnClickNext();//Object^ sender, EventArgs^ e);
    void OnClose(Object^ sender, System::ComponentModel::CancelEventArgs^ e);
	void OnLoad(Object^ sender, EventArgs^ e);
	void OnActived(Object^ sender, EventArgs^ e);


};
