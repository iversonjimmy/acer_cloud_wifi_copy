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
#include "TabController.h"
#include "settings_form.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "logger.h"
#include "localized_text.h"
#include "Layout.h"
#include "async_operations.h"


using namespace System::Collections;
using namespace System::IO;

CTabController::CTabController(SetupForm^ parent) : Panel()
{
    mParent = parent;

	this->mDeviceMgrButton = (gcnew CTabButton());
	this->mControlPanelButton = (gcnew CTabButton());
	this->pictureBox_Line = (gcnew System::Windows::Forms::PictureBox());
	this->mAccountLabel = (gcnew System::Windows::Forms::Label());
	
	//mDeviceMgrPanel = gcnew CDeviceManagePanel(parent);
	//mDeviceMgrPanel = gcnew CSyncItemsPanel(parent);
	//mControlPanel = gcnew CControlPanel(parent);
	mDeviceMgrPanel = gcnew CDeviceManagePanel(parent);
	mControlPanel = gcnew CControlPanel(parent);

	this->BackColor = System::Drawing::Color::FromArgb(255, 245, 245, 245);
	this->Controls->Add(mAccountLabel);
	this->Controls->Add(mDeviceMgrButton);
	this->Controls->Add(mControlPanelButton);
	this->Controls->Add(pictureBox_Line);			
	this->Controls->Add(mControlPanel);
	this->Controls->Add(mDeviceMgrPanel);

	this->Margin = System::Windows::Forms::Padding(0);
	this->Name = L"panel1";
	this->Size = System::Drawing::Size(728, 424);
	this->Location = System::Drawing::Point(14, 67);
	this->TabIndex = 3;

	// 
	// mControlPanelButton
	// 
	this->mControlPanelButton->Location = System::Drawing::Point(31, 1);	
	this->mControlPanelButton->Name = L"mControlPanelButton";
	this->mControlPanelButton->Size = System::Drawing::Size(180, 33);
	//this->mControlPanelButton->GetBtn->Text = L"Control Panel";
	this->mControlPanelButton->Text = L"Control Panel";
	mControlPanelButton->Unselected();
	// 
	// mDeviceMgrButton
	// 
	this->mDeviceMgrButton->Location = System::Drawing::Point(mControlPanelButton->Location.X + mControlPanelButton->Size.Width -2, 1);
	this->mDeviceMgrButton->Name = L"mDeviceMgrButton";
	this->mDeviceMgrButton->Size = System::Drawing::Size(160, 33);
	this->mDeviceMgrButton->TabIndex = 5;
	//this->mDeviceMgrButton->GetBtn->Text = L"Device Management";	
	this->mDeviceMgrButton->Text = L"Device Management";	
	
	mDeviceMgrButton->Selected();

	// 
	// pictureBox_Line
	// 
	this->pictureBox_Line->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(178)), static_cast<System::Int32>(static_cast<System::Byte>(178)), 
		static_cast<System::Int32>(static_cast<System::Byte>(178)));
	this->pictureBox_Line->Location = System::Drawing::Point(3, 33);
	this->pictureBox_Line->Name = L"pictureBox_Line";
	this->pictureBox_Line->Size = System::Drawing::Size(725, 1);
	this->pictureBox_Line->TabIndex = 2;
	this->pictureBox_Line->TabStop = false;
	// 
	// mAccountLabel
	// 
	this->mAccountLabel->Font = (gcnew System::Drawing::Font(L"Arial", 10));
	this->mAccountLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(120)), static_cast<System::Int32>(static_cast<System::Byte>(120)), 
		static_cast<System::Int32>(static_cast<System::Byte>(120)));
	this->mAccountLabel->Location = System::Drawing::Point(480, 8);
	this->mAccountLabel->Name = L"mAccountLabel";
	this->mAccountLabel->Size = System::Drawing::Size(233, 20);
	this->mAccountLabel->TabIndex = 6;
	this->mAccountLabel->Text = L"Account";
	this->mAccountLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;


	mControlPanelButton->OnClickedEvent += gcnew CTabButton::OnClickedHandler(this, &CTabController::OnClickControlPanel);
	mDeviceMgrButton->OnClickedEvent += gcnew CTabButton::OnClickedHandler(this, &CTabController::OnClickDeviceMgr);


	//Set size and location

    // panel

	ControlPanelState();

	AsyncOperations::Instance->LogoutComplete += gcnew AsyncCompleteHandler(this, &CTabController::LogoutComplete);	
}

//void CTabController::roundButton_Paint( Object^ sender,
//      System::Windows::Forms::PaintEventArgs^ e )
//   {
//      System::Drawing::Drawing2D::GraphicsPath^ buttonPath = gcnew System::Drawing::Drawing2D::GraphicsPath;
//      
//      // Set a new rectangle to the same size as the button's 
//      // ClientRectangle property.
//      System::Drawing::Rectangle newRectangle = mDeviceMgrButton->ClientRectangle;
//      
//      // Decrease the size of the rectangle.
//      //newRectangle.Inflate(  -10, -10);
//      
//      // Draw the button's border.
//      //e->Graphics->DrawEllipse( System::Drawing::Pens::Black, newRectangle );
//      
//      // Increase the size of the rectangle to include the border.
//      //newRectangle.Inflate( 5, 5 );
//      
//      // Create a circle within the new rectangle.
//      //buttonPath->AddEllipse( newRectangle );
//	  buttonPath->AddRectangle(System::Drawing::RectangleF(newRectangle.X, newRectangle.Y, newRectangle.Width, newRectangle.Height));
//      
//      // Set the button's Region property to the newly created 
//      // circle region.
//      mDeviceMgrButton->Region = gcnew System::Drawing::Region( buttonPath );
//	  
//   }



void CTabController::OnClickControlPanel()//Object^ sender, EventArgs^ e)
{	
	ControlPanelState();
}

void CTabController::OnClickDeviceMgr()//Object^ sender, EventArgs^ e)
{
	DeviceMgrState();
}

//void CTabController::OnClickUnlink(Object^ sender, EventArgs^ e)
//{	
//	AsyncOperations::Instance->Logout();
//}


void CTabController::ControlPanelState()
{
	mControlPanelButton->Selected();
	mDeviceMgrButton->Unselected();

	mControlPanel->Show();
	mDeviceMgrPanel->Hide();

	mIsControlPanelState = true;
	mParent->ChangeState(StateEvent::ControlPanel);

	mAccountLabel->Text = CcdManager::Instance->UserName;
}

void CTabController::DeviceMgrState()
{
	mControlPanelButton->Unselected();
	mDeviceMgrButton->Selected();

	mDeviceMgrPanel->UpdateDevicesStatus();
	

	mControlPanel->Hide();
	mDeviceMgrPanel->Show();
	
	mIsControlPanelState = false;

	mParent->ChangeState(StateEvent::DeviceMgr);

	mAccountLabel->Text = CcdManager::Instance->UserName;



}

void CTabController::ResetSyncItems(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
 	mControlPanel->Reset(nodes, filters);
}


void CTabController::LogoutComplete(String^ result)
{
    if (result != L"") {        
		//if( !result->Contains(L"14118") )
		{
			MessageBox::Show(result);
			return;
		}
    }
	mParent->ChangeState(StateEvent::LogoutCompleted);
}

void CTabController::Apply()
{
	mControlPanel->ApplySetting(mAccountName, mPassword);

}

void CTabController::SetAccountAndPassword(String^ name, String^ password)
{	
	mAccountName = name;
	mPassword = password;

	mAccountLabel->Text = name;
}


bool CTabController::IsControlPanelMode()
{
	return mIsControlPanelState;
}


CTabController::~CTabController()
{
    // do nothing, memory is managed
}






