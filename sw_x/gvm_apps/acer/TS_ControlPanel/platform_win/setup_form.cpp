//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "util_funcs.h"
#include "setup_form.h"
#include "localized_text.h"
#include "Layout.h"
#include "async_operations.h"
#include "WM_USER.h"


#include <windows.h>
#pragma comment (lib, "user32.lib")

SetupForm::SetupForm()
{
    
	mClosePictureBox = gcnew System::Windows::Forms::PictureBox();

	// buttons
	mNextButton = gcnew CMyButton();
	mCancelButton = gcnew CMyButton();

	mLoginPanel = gcnew SignupPanel(this);
    mRegisterPanel = gcnew RegisterPanel(this);
    mStartPanel = gcnew StartPanel(this);
	mFinishPanel = gcnew FinishPanel(this);
	mVerifyIDPanel = gcnew CVerifyID();
	mTabController = gcnew CTabController(this);
	mForgetPassword = gcnew CForgetPassword(this);

	SuspendLayout();

	//
	//mClosePictureBox
	//
	mClosePictureBox->BackColor = System::Drawing::Color::WhiteSmoke;			
	mClosePictureBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_close_n.png");
	mClosePictureBox->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Center;
	mClosePictureBox->Location = System::Drawing::Point(702, 21);
	mClosePictureBox->Name = L"mClosePictureBox";
	mClosePictureBox->Size = System::Drawing::Size(30, 30);
	mClosePictureBox->TabIndex = 6;
	mClosePictureBox->TabStop = false;	
	mClosePictureBox->Click += gcnew System::EventHandler(this, &SetupForm::mClosePictureBox_Click);
	//
	//mNextButton
	//
    mNextButton->Location = System::Drawing::Point(Panel_Width - mNextButton->Size.Width - 150, Panel_Height - mNextButton->Size.Height - 15);
    mNextButton->Text = LocalizedText::Instance->SignInButton;
	mNextButton->OnClickedEvent += gcnew CMyButton::OnClickedHandler(this, &SetupForm::OnClickNext);
	//
    //mCancelButton
	//
    mCancelButton->Location = System::Drawing::Point(mNextButton->Location.X + mNextButton->Size.Width + 10 , mNextButton->Location.Y);    
    mCancelButton->Text = LocalizedText::Instance->CancelButton;
    mCancelButton->Enabled = false;
	mCancelButton->OnClickedEvent += gcnew CMyButton::OnClickedHandler(this, &SetupForm::OnClickCancel);
    mCancelButton->Hide();

#pragma push_macro("ExtractAssociatedIcon")
#undef ExtractAssociatedIcon
	//System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(DeviceMgrForm::typeid));
    this->Icon = System::Drawing::Icon::ExtractAssociatedIcon(Application::ExecutablePath);	
	//this->Icon = System::Drawing::Icon( (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.BackgroundImage")));

#pragma pop_macro("ExtractAssociatedIcon")

    AutoScaleDimensions = System::Drawing::SizeF(7, 15);
    AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
	BackColor = System::Drawing::Color::Lime;
	TransparencyKey = System::Drawing::Color::Lime;
    ClientSize = System::Drawing::Size(Panel_Width, Panel_Height);
    Font = (gcnew System::Drawing::Font(L"Arial", 11,
        System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
	FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
    Name = L"Form1";
    StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
    Text = LocalizedText::Instance->FormTitle;	
    Closing += gcnew System::ComponentModel::CancelEventHandler(this, &SetupForm::OnClose);
    //FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
    //MaximizeBox = false;
    //MinimizeBox = false;
    
	//ShowInTaskbar = false;
	//這個被mark掉是有故事的: 因為半透明背景, 所以實作了AlphaBGForm, 再將SetupForm與其連動, 所以其中一個必須將ShowInTaskbar設成False, 
	//然而這裡這個False卻造成SetupForm在某些情況下會顯示不出來, 所以將這個拿掉, 把AlphaBGForm的設成false.

	Controls->Add(mClosePictureBox);
	Controls->Add(mCancelButton);
    Controls->Add(mNextButton);
    Controls->Add(mLoginPanel);
    Controls->Add(mRegisterPanel);
    Controls->Add(mStartPanel);
	Controls->Add(mFinishPanel);
	Controls->Add(mTabController);
	Controls->Add(mForgetPassword);
	Controls->Add(mVerifyIDPanel);
	
    mRegisterPanel->Hide();
	mFinishPanel->Hide();
	mStartPanel->Hide();
	mLoginPanel->Hide();
	mTabController->Hide();
	mForgetPassword->Hide();
	mVerifyIDPanel->Hide();

	mPanel = mTabController;
	mPanel->Hide();
	
	//TabControlState();
	LoginState(); 
	//FinishState(true);
	
	AsyncOperations::Instance->ResetSettingsComplete +=
        gcnew ResetSettingsHandler(this, &SetupForm::ResetSettingsComplete);
    AsyncOperations::Instance->ApplySettingsComplete +=
        gcnew AsyncCompleteHandler(this, &SetupForm::ApplySettingsComplete);

	this->Load += gcnew EventHandler(this, &SetupForm::OnLoad);
	this->Activated+= gcnew EventHandler(this, &SetupForm::OnActived);
	this->ResumeLayout(false);
}

SetupForm::~SetupForm()
{
}

void SetupForm::OnActived(Object^ sender, EventArgs^ e)
{
	this->BringToFront();
	//System::Diagnostics::Debug::WriteLine("Actived");
}

void SetupForm::OnLoad(Object^ sender, EventArgs^ e)
{
	//this->SetStyle(System::Windows::Forms::ControlStyles::SupportsTransparentBackColor, true);
	//this->BackColor = System::Drawing::Color::Transparent;
	

}

void SetupForm::EnableSubmit(bool value)
{
    mNextButton->Enabled = value;
}

void SetupForm::Advance(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (mPanel == mLoginPanel || mPanel == mRegisterPanel) {		
		//mPanel->Hide();

		//marked by Cigar, for new Acer TS 1.0 flow
		if(nodes != nullptr && filters != nullptr)
		{
			mTabController->ResetSyncItems(nodes, filters);
		}
		//else 
		ChangeState(StateEvent::RegisterCompeted);
		//mPanel = mFinishPanel;

        //mPanel->Show();
        //mNextButton->Enabled = true;
        //mCancelButton->Enabled = true;//false;
        //mNextButton->Text = LocalizedText::Instance->FinishButton;
        //mCancelButton->Hide();
    }
	//else if (mPanel == mSyncPanel) {
    //    mPanel->Hide();
    //    mPanel = mStartPanel;
    //    mPanel->Show();
    //    mCancelButton->Enabled = false;
    //    mNextButton->Text = LocalizedText::Instance->NextButton;
    //    this->Hide();
    //}
}

void SetupForm::OnClickNext()//Object^ sender, EventArgs^ e)
{

	ChangeState(StateEvent::OnNextButton);
 //   if (mPanel == mStartPanel) 
	//{
 //       mPanel->Hide();
 //       if (mStartPanel->HasAccount == true) {
 //           mPanel = mLoginPanel;
 //           mNextButton->Enabled = mLoginPanel->IsEnableSubmit;
 //       } else {
 //           mPanel = mRegisterPanel;
 //           mNextButton->Enabled = mRegisterPanel->IsEnableSubmit;
 //       }
 //       mPanel->Show();
 //       mCancelButton->Enabled = true;
 //       mCancelButton->Show();
 //   }
	//else if (mPanel == mLoginPanel)
	//{
	//	mPanel->Hide();
	//	mRegisterPanel->Show();
	//}
	//else if (mPanel == mRegisterPanel) {
 //       mRegisterPanel->Register();
 //   } else if (mPanel == mLoginPanel) {
 //       mLoginPanel->Login();
 //   } else if (mPanel == mSyncPanel) {
 //       mSyncPanel->Apply();	
 //   }
}



//Get the process of main process by name and kill it.
void SetupForm::mClosePictureBox_Click(System::Object^  sender, System::EventArgs^  e) 
{
	this->Hide();
	return;

	Application::Exit();

	return;

	array<System::Diagnostics::Process^>^ localByName  = System::Diagnostics::Process::GetProcessesByName( "acpanel_win" );
	Collections::IEnumerator^ myEnum = localByName->GetEnumerator();
	while(myEnum->MoveNext())
	{
		System::Diagnostics::Process^ proc = ((System::Diagnostics::Process^)(myEnum->Current));
		proc->Kill();
	}

	//HWND hwnd = FindWindow(nullptr, L"AlphaBGForm ");	
	//if(hwnd != 0)
	//{
	//	PostMessage(hwnd, WM_CLOSE, 0 , 0);
	//}
}

void SetupForm::OnClickCancel()//Object^ sender, EventArgs^ e)
{	
	
	if(CcdManager::Instance->IsLoggedIn == false) 
	{
		LoginState();
	}
	else
	{
		this->Hide();
	}
	//if (mPanel == mRegisterPanel || mPanel == mLoginPanel) {
    //    mPanel->Hide();
    //    mPanel = mStartPanel;
    //    mPanel->Show();
    //    mCancelButton->Enabled = false;
    //    mNextButton->Enabled = true;
    //    mCancelButton->Hide();
    //}
}

void SetupForm::OnClose(Object^ sender, System::ComponentModel::CancelEventArgs^ e)
{
    e->Cancel = true;

	
	this->Hide();
	return ;



    //if (mPanel == mSyncPanel) {
    //    mPanel->Hide();
    //    mPanel = mStartPanel;
    //    mPanel->Show();
    //    mCancelButton->Enabled = false;
    //    mNextButton->Text = LocalizedText::Instance->NextButton;
    //    this->Hide();
    //    mSyncPanel->Apply();
    //} else {
    //    ::DialogResult result = MessageBox::Show(
    //        LocalizedText::Instance->ExitQuestion,
    //        L"",
    //        MessageBoxButtons::YesNo);
    //    if (result == ::DialogResult::Yes) {
    //        Application::Exit();
    //    }
    //}
}

void SetupForm::ChangeState(StateEvent stateEvent)
{
	if(stateEvent == StateEvent::LogoutCompleted)
	{
		LoginState();
	}
	else if (stateEvent == StateEvent::ApplyCompleted)
	{
		LoginState();		
		this->Hide();
	}
	 else if (mPanel == mStartPanel) 
	{
        mPanel->Hide();
        if (mStartPanel->HasAccount == true) {
            mPanel = mLoginPanel;
            mNextButton->Enabled = mLoginPanel->IsEnableSubmit;
        } else {
            mPanel = mRegisterPanel;
            mNextButton->Enabled = mRegisterPanel->IsEnableSubmit;
        }
        mPanel->Show();
        mCancelButton->Enabled = true;
        mCancelButton->Show();
    }
	else if (mPanel == mRegisterPanel) 
	{
		switch(stateEvent)
		{
			case StateEvent::RegisterCompeted:
				FinishState(true);
			break;
			case StateEvent::OnNextButton:
				mRegisterPanel->Register();
			break;
		}
    } 
	else if (mPanel == mLoginPanel) 
	{
		//mPanel->Hide();
		switch(stateEvent)
		{ 
		case StateEvent::CreateAccount:
			RegistrationState();
			break;
		case StateEvent::ForgetPassword:
			ForgetPasswordState();
			break;
		case StateEvent::OnNextButton:	//Sign in.
			mLoginPanel->Login();
			break;
		case StateEvent::RegisterCompeted:
			TabControlState();						
			break;
		}
		//mPanel->Show();        
    } 
	//else if (mPanel == mSyncPanel) 
	//{
 //       mSyncPanel->Apply();
 //   }
	else if (mPanel == mFinishPanel)
	{
		TabControlState();						
		//LoginState();
	}
	else if (mPanel == mVerifyIDPanel)
	{
		FinishState(false);
	}
	else if(mPanel == mTabController)
	{
		//mPanel = mTabController;
		switch(stateEvent)
		{
		case StateEvent::DeviceMgr:
			mNextButton->Hide();
			mCancelButton->Hide();
			break;
		case StateEvent::ControlPanel:
			mNextButton->Show();
			mCancelButton->Show();
			break;
		case StateEvent::OnNextButton:	//Apply sync settings.
			mTabController->Apply();
			break;
		}
		if(mPanel != nullptr)
			mPanel->Show();
	}
	
}


bool SetupForm::IsSyncPanelState()
{
    if(mPanel == mTabController)
	{
		return mTabController->IsControlPanelMode();
	}
	return false;
}


void SetupForm::ApplySettingsComplete(String^ result)
{
    if (result != L"") {
        MessageBox::Show(result);
    }
	else
	{
		ChangeState(StateEvent::ApplyCompleted);
	}
}


void SetupForm::ResetSettingsComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (result != L"") {
        MessageBox::Show(result);
    }

    //mSyncTab->Reset(nodes, filters);
    //this->SetApplyButton(false);
    //mTabListBox->SelectedIndex = 0;
	mTabController->ResetSyncItems(nodes, filters);
	//this->ShowSetup();	
	TabControlState();

    // show form
    this->Opacity = 1;	
    this->ShowInTaskbar = true;
    this->Focus();
}

void SetupForm::LoginState()
{
	mPanel->Hide();
	mPanel = mLoginPanel;
	mPanel->Show();
	mCancelButton->Hide();

	mNextButton->Text = LocalizedText::Instance->SignInButton;
	
    mNextButton->Location = System::Drawing::Point(354, 430);
	mNextButton->Show();
	mNextButton->Enabled = false;

    mCancelButton->Location = System::Drawing::Point(mNextButton->Location.X + mNextButton->Size.Width + 10 , mNextButton->Location.Y);    
}

void SetupForm::RegistrationState()
{  

	if(!UtilFuncs::CheckAcerWMI())
	{
		MessageBox::Show(LocalizedText::Instance->NonAcerDescription, LocalizedText::Instance->NonAcerCaption, MessageBoxButtons::OK, MessageBoxIcon::Exclamation);
#ifndef _Test
		return;
#endif
	}

	mPanel->Hide();
	mPanel	 =  mRegisterPanel;
	mPanel->Show();
	//mNextButton->Size = System::Drawing::Size(Button_Width_L, Button_Height);
    mNextButton->Location = System::Drawing::Point(255, 467);
    mNextButton->Text = LocalizedText::Instance->IAcceptButton;
	mNextButton->Enabled = false;

    mCancelButton->Location = System::Drawing::Point(391, 467);
	mCancelButton->Show();
	mCancelButton->Enabled = true;
}


void SetupForm::FinishState(bool bVerifyIDStyle)
{
	mPanel->Hide();	
	if(bVerifyIDStyle)
	{
		mPanel = mVerifyIDPanel;
		mVerifyIDPanel->SetEmail(mRegisterPanel->GetRegisteredEmail);
		
		mNextButton->Text = LocalizedText::Instance->NextButton;

		mNextButton->Location = System::Drawing::Point(266, 485);//Panel_Width - mNextButton->Size.Width - 150, Panel_Height - mNextButton->Size.Height - 15);
		mCancelButton->Location = System::Drawing::Point(410, 485);//mNextButton->Location.X + mNextButton->Size.Width + 10 , mNextButton->Location.Y);    
		mCancelButton->Show();
	}
	else 
	{
		mPanel = mFinishPanel;		
		mNextButton->Text = LocalizedText::Instance->FinishButton;

		mNextButton->Location = System::Drawing::Point(330, 485);//Panel_Width - mNextButton->Size.Width - 150, Panel_Height - mNextButton->Size.Height - 15);
		mCancelButton->Hide();
		//mCancelButton->Location = System::Drawing::Point(410, 485);//mNextButton->Location.X + mNextButton->Size.Width + 10 , mNextButton->Location.Y);    
	}

	mPanel->Show();

	//mNextButton->Size = System::Drawing::Size(Button_Width, Button_Height);
    

	mNextButton->Enabled = true;
	
}

void SetupForm::ForgetPasswordState()
{

	UtilFuncs::ShowUrl(CcdManager::Instance->UrlForgetPassword);

	return ;

	//mPanel->Hide();
	//mPanel	 = mForgetPassword;
	//	
	//mNextButton->Text = LocalizedText::Instance->SubmitButton;
	//mPanel->Show();

	//mNextButton->Size = System::Drawing::Size(Button_Width, Button_Height);
 //   mNextButton->Location = System::Drawing::Point(Panel_Width - mNextButton->Size.Width - 150, Panel_Height - mNextButton->Size.Height - 15);
	//mNextButton->Enabled = true;

 //   mCancelButton->Location = System::Drawing::Point(mNextButton->Location.X + mNextButton->Size.Width + 10 , mNextButton->Location.Y);    
	//mCancelButton->Show();
}

void SetupForm::TabControlState()
{
	mPanel->Hide();
	mPanel	 = mTabController;

	//mTabController->SetAccountAndPassword(CcdManager::Instance->UserName, CcdManager::Instance->Password);
	mTabController->ControlPanelState(); 

	mNextButton->Text = LocalizedText::Instance->ApplyButton;
	mPanel->Show();
	
    mNextButton->Location = System::Drawing::Point(234, 494);
	mNextButton->Enabled = true;

    mCancelButton->Location = System::Drawing::Point(370, 494);
	mCancelButton->Show();
	mCancelButton->Enabled = true;
}



//
//void SetupForm::WndProc( Message% m )
//{
//	MessageProc(m);
//}

//
//void  SetupForm::MessageProc(Message %m)
//{
//	 // Listen for operating system messages.
//	switch ( m.Msg )
//	{
//		case WM_LOGIN:
//		case WM_PSN:
//			System::Diagnostics::Debug::WriteLine(L"SetupForm, handle: "+m.WParam+", msg: " + m.Msg);
//
//			int nResult = 0;
//			if( (m.Msg == WM_LOGIN && CcdManager::Instance->IsLoggedIn) ||
//				(m.Msg == WM_PSN && CcdManager::Instance->IsLogInPSN) )
//			{	
//					nResult = 1;
//			}
//			PostMessage((HWND)(m.WParam.ToInt32()), m.Msg, nResult, 0);
//			break;
//	}
//	Form::WndProc( m );
//}