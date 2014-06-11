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
#include "DeviceManagement_Panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"
#include "Layout.h"
#include "SyncItem.h"
#include "Logger.h"
#include "DeviceLabel.h"
#include "async_operations.h"

using namespace System::Collections;
using namespace System::IO;

CDeviceManagePanel::CDeviceManagePanel(Form^ parent) : Panel()
{
    mParent = parent;
			//System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(CDeviceManagePanel::typeid));

			this->mUnlinkBtn = (gcnew CMyButton(true));
			this->mCloudPC = (gcnew System::Windows::Forms::Label());
			this->mPlatform = (gcnew System::Windows::Forms::Label());
			this->mDeviceName = (gcnew System::Windows::Forms::Label());
			this->mPlatformBarBottom = (gcnew System::Windows::Forms::PictureBox());
			this->mInfoBarPanel = (gcnew System::Windows::Forms::Panel());
			this->mActionLabel = (gcnew System::Windows::Forms::Label());
			this->mPlatformLabel = (gcnew System::Windows::Forms::Label());
			this->mDeviceNameLabel = (gcnew System::Windows::Forms::Label());
			this->mDescription = (gcnew System::Windows::Forms::Label());
			this->mLeftLine = (gcnew System::Windows::Forms::PictureBox());
			this->mRightLine = (gcnew System::Windows::Forms::PictureBox());
			mDeviceDescLabel = gcnew System::Windows::Forms::Label();
			mDevicesListview = gcnew System::Windows::Forms::ListView();
			mRightArrow = gcnew System::Windows::Forms::PictureBox();
			mLeftArrow =  gcnew System::Windows::Forms::PictureBox();
			mDevicesPanel =  gcnew System::Windows::Forms::Panel();

			this->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->mPlatformBarBottom))->BeginInit();
			this->mInfoBarPanel->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->mLeftLine))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->mRightLine))->BeginInit();
			this->SuspendLayout();
			// 
			// MainPanel
			// 
			BackColor = System::Drawing::Color::White;
			BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			Controls->Add(mUnlinkBtn);
			Controls->Add(mCloudPC);
			Controls->Add(mDescription);
			Controls->Add(mPlatform);
			Controls->Add(mInfoBarPanel);
			Controls->Add(mDeviceName);
			Controls->Add(mPlatformBarBottom);
			Controls->Add(mRightLine);
			Controls->Add(mLeftLine);			

			Controls->Add(mDeviceDescLabel);
			Controls->Add(mDevicesListview);
			Controls->Add(mRightArrow);
			Controls->Add(mLeftArrow);
			mDevicesListview->Controls->Add(mDevicesPanel);

			Name = L"MainPanel";
			Size = System::Drawing::Size(730, 414);
			Location = System::Drawing::Point(0, 34);			
			this->TabIndex = 7;
			// 
			// mUnlinkBtn
			// 
			this->mUnlinkBtn->Font = (gcnew System::Drawing::Font(L"Arial", 11, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mUnlinkBtn->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mUnlinkBtn->Location = System::Drawing::Point(489, 278);
			this->mUnlinkBtn->Name = L"mUnlinkBtn";
			this->mUnlinkBtn->Size = System::Drawing::Size(130, 29);
			this->mUnlinkBtn->TabIndex = 11;
			this->mUnlinkBtn->Text = L"Unlink";
			this->mUnlinkBtn->Enabled = false;
			this->mUnlinkBtn->OnClickedEvent +=	gcnew CMyButton::OnClickedHandler(this, &CDeviceManagePanel::OnClickUnlink);
			// 
			// mCloudPC
			// 
			this->mCloudPC->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mCloudPC->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(255)), static_cast<System::Int32>(static_cast<System::Byte>(187)), 
				static_cast<System::Int32>(static_cast<System::Byte>(0)));
			this->mCloudPC->Image =  System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_cloud_pc.png");
			this->mCloudPC->ImageAlign = System::Drawing::ContentAlignment::MiddleLeft;
			this->mCloudPC->Location = System::Drawing::Point(63, 304);
			this->mCloudPC->Name = L"mCloudPC";
			this->mCloudPC->Size = System::Drawing::Size(155, 28);
			this->mCloudPC->TabIndex = 10;
			this->mCloudPC->Text = L"   Cloud PC";
			this->mCloudPC->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			this->mCloudPC->Hide();
			// 
			// mPlatform
			// 
			this->mPlatform->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mPlatform->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mPlatform->Location = System::Drawing::Point(298, 278);
			this->mPlatform->Name = L"mPlatform";
			this->mPlatform->Size = System::Drawing::Size(166, 26);
			this->mPlatform->TabIndex = 9;
			//this->mPlatform->Text = L"Windows 7";
			this->mPlatform->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mDeviceName
			// 
			this->mDeviceName->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mDeviceName->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mDeviceName->Location = System::Drawing::Point(62, 278);
			this->mDeviceName->Name = L"mDeviceName";
			this->mDeviceName->Size = System::Drawing::Size(212, 26);
			this->mDeviceName->TabIndex = 9;
			//this->mDeviceName->Text = L"Acer Aspire M3910";
			this->mDeviceName->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mPlatformBarBottom
			// 
			this->mPlatformBarBottom->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\info_bar_bottom.png");
			this->mPlatformBarBottom->BackgroundImageLayout = System::Windows::Forms::ImageLayout::None;
			this->mPlatformBarBottom->Location = System::Drawing::Point(46, 345);
			this->mPlatformBarBottom->Name = L"mPlatformBarBottom";
			this->mPlatformBarBottom->Size = System::Drawing::Size(634, 7);
			this->mPlatformBarBottom->TabIndex = 6;
			this->mPlatformBarBottom->TabStop = false;
			// 
			// mInfoBarPanel
			// 
			this->mInfoBarPanel->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\info_bar.png");
			this->mInfoBarPanel->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			this->mInfoBarPanel->Controls->Add(this->mActionLabel);
			this->mInfoBarPanel->Controls->Add(this->mPlatformLabel);
			this->mInfoBarPanel->Controls->Add(this->mDeviceNameLabel);
			this->mInfoBarPanel->Location = System::Drawing::Point(45, 236);
			this->mInfoBarPanel->Name = L"mInfoBarPanel";
			this->mInfoBarPanel->Size = System::Drawing::Size(636, 24);
			this->mInfoBarPanel->TabIndex = 5;
			// 
			// mActionLabel
			// 
			this->mActionLabel->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(111)), static_cast<System::Int32>(static_cast<System::Byte>(111)), 
				static_cast<System::Int32>(static_cast<System::Byte>(111)));
			this->mActionLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mActionLabel->ForeColor = System::Drawing::Color::White;
			this->mActionLabel->Location = System::Drawing::Point(439, 1);
			this->mActionLabel->Name = L"mActionLabel";
			this->mActionLabel->Size = System::Drawing::Size(177, 22);
			this->mActionLabel->TabIndex = 4;
			this->mActionLabel->Text = L"Action";
			this->mActionLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mPlatformLabel
			// 
			this->mPlatformLabel->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(111)), static_cast<System::Int32>(static_cast<System::Byte>(111)), 
				static_cast<System::Int32>(static_cast<System::Byte>(111)));
			this->mPlatformLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mPlatformLabel->ForeColor = System::Drawing::Color::White;
			this->mPlatformLabel->Location = System::Drawing::Point(252, 1);
			this->mPlatformLabel->Name = L"mPlatformLabel";
			this->mPlatformLabel->Size = System::Drawing::Size(190, 22);
			this->mPlatformLabel->TabIndex = 4;
			this->mPlatformLabel->Text = L"Platform";
			this->mPlatformLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mDeviceNameLabel
			// 
			this->mDeviceNameLabel->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(111)), 
				static_cast<System::Int32>(static_cast<System::Byte>(111)), static_cast<System::Int32>(static_cast<System::Byte>(111)));
			this->mDeviceNameLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mDeviceNameLabel->ForeColor = System::Drawing::Color::White;
			this->mDeviceNameLabel->Location = System::Drawing::Point(13, 1);
			this->mDeviceNameLabel->Name = L"mDeviceNameLabel";
			this->mDeviceNameLabel->Size = System::Drawing::Size(239, 22);
			this->mDeviceNameLabel->TabIndex = 4;
			this->mDeviceNameLabel->Text = L"Device Name";
			this->mDeviceNameLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mDescription
			// 
			this->mDescription->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mDescription->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mDescription->Location = System::Drawing::Point(31, 20);
			this->mDescription->Name = L"mDescription";
			this->mDescription->Size = System::Drawing::Size(685, 46);
			this->mDescription->TabIndex = 2;
			this->mDescription->Text = L"You can add acer devices and two non-acer devices to acerCloud, you could automat" 
				L"ically sync and share your data.";
			this->mDescription->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// mLeftLine
			// 
			this->mLeftLine->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(226)), static_cast<System::Int32>(static_cast<System::Byte>(226)), 
				static_cast<System::Int32>(static_cast<System::Byte>(226)));
			this->mLeftLine->Location = System::Drawing::Point(46, 260);
			this->mLeftLine->Name = L"mLeftLine";
			this->mLeftLine->Size = System::Drawing::Size(1, 85);
			this->mLeftLine->TabIndex = 7;
			this->mLeftLine->TabStop = false;
			// 
			// mRightLine
			// 
			this->mRightLine->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(226)), static_cast<System::Int32>(static_cast<System::Byte>(226)), 
				static_cast<System::Int32>(static_cast<System::Byte>(226)));
			this->mRightLine->Location = System::Drawing::Point(679, 260);
			this->mRightLine->Name = L"mRightLine";
			this->mRightLine->Size = System::Drawing::Size(1, 85);
			this->mRightLine->TabIndex = 8;
			this->mRightLine->TabStop = false;


			// 
			// mDeviceDescLabel
			// 
			this->mDeviceDescLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mDeviceDescLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(120)), 
				static_cast<System::Int32>(static_cast<System::Byte>(120)), static_cast<System::Int32>(static_cast<System::Byte>(120)));
			this->mDeviceDescLabel->Location = System::Drawing::Point(225, 91);
			this->mDeviceDescLabel->Name = L"mDeviceDescLabel";
			this->mDeviceDescLabel->Size = System::Drawing::Size(454, 31);
			this->mDeviceDescLabel->TabIndex = 16;
			this->mDeviceDescLabel->Text = DeviceDesc;
			this->mDeviceDescLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// mDevicesPanel
			// 
			this->mDevicesPanel->AutoSize = true;
			
			this->mDevicesPanel->Location = System::Drawing::Point(0, 0);
			this->mDevicesPanel->Name = L"mDevicesPanel";
			this->mDevicesPanel->Size = System::Drawing::Size(260, 110);
			this->mDevicesPanel->TabIndex = 13;

			// 
			// mDevicesListview
			// 
			this->mDevicesListview->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->mDevicesListview->Location = System::Drawing::Point(49, 124);
			this->mDevicesListview->Name = L"mDevicesListview";
			this->mDevicesListview->Size = System::Drawing::Size(630, 110);
			this->mDevicesListview->TabIndex = 12;
			this->mDevicesListview->UseCompatibleStateImageBehavior = false;
			// 
			// mRightArrow
			// 
			this->mRightArrow->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\arrow_n_r.png");// (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"arrow_n_r.png")));
			this->mRightArrow->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Center;
			this->mRightArrow->Location = System::Drawing::Point(678, 161);
			this->mRightArrow->Name = L"mRightArrow";
			this->mRightArrow->Size = System::Drawing::Size(27, 30);
			this->mRightArrow->TabIndex = 15;
			this->mRightArrow->TabStop = false;
			this->mRightArrow->Click += gcnew System::EventHandler(this, &CDeviceManagePanel::mRightArrow_Click);
			// 
			// mLeftArrow
			// 
			this->mLeftArrow->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\arrow_n_l.png");//(cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"arrow_n_l.png")));
			this->mLeftArrow->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Center;
			this->mLeftArrow->Location = System::Drawing::Point(25, 161);
			this->mLeftArrow->Name = L"mLeftArrow";
			this->mLeftArrow->Size = System::Drawing::Size(27, 30);
			this->mLeftArrow->TabIndex = 14;
			this->mLeftArrow->TabStop = false;
			this->mLeftArrow->Click += gcnew System::EventHandler(this, &CDeviceManagePanel::mLeftArrow_Click);

}

CDeviceManagePanel::~CDeviceManagePanel()
{
    // do nothing, memory is managed
}

void CDeviceManagePanel::OnClickUnlink()//Object^ sender, EventArgs^ e)
{	
	if(mSelectedDevice != nullptr)
	{
		try
		{
			
			System::String^ CurrentDeviceName = gcnew String(Environment::MachineName->ToUpper());
			if(CurrentDeviceName->Equals( mSelectedDevice->DeviceName->ToUpper()))
			{
				
				try
				{					
					AsyncOperations::Instance->Logout();
				}
				catch(Exception^ e)
				{

				}
			}
			else
			{
				CcdManager::Instance->Unlink(mSelectedDevice->mDeviceID);
				UpdateDevicesStatus();
				mSelectedDevice = nullptr;
				UpdateSelectedDeviceInfo();
			}
		}
		catch(Exception^ e)
		{
			Logger::Instance->WriteLine("OnClickUnlink: " + e->Message);
		}
	}
	
	//Following is for testing to get all link devices. It is not related with Unlink button .
	//CcdManager::Instance->UpdateLinkedDevices();
	//int DeviceNum = CcdManager::Instance->NumDevices;
	//Logger::Instance->WriteLine("OnClickUnlink: Total device: " + DeviceNum.ToString());
	//
	////MessageBox::Show(DeviceNum);
	//for(int i = 0; i < DeviceNum; i++)
	//{
	//	Logger::Instance->WriteLine(	"Device ID: " +CcdManager::Instance->GetDeviceID(i).ToString() +
	//													"\nDevice Name: " + CcdManager::Instance->GetDeviceName(i) +
	//													"\nDevice Type: " +  CcdManager::Instance->GetDeviceClass(i));

	//	Logger::Instance->WriteLine(	"Acer Machine: " + CcdManager::Instance->IsAcerMachine(i).ToString());
	//	Logger::Instance->WriteLine(	"Android Device: " + CcdManager::Instance->IsAndroidDevice(i).ToString());
	//}
}


void CDeviceManagePanel::mRightArrow_Click(System::Object^  sender, System::EventArgs^  e) 
 {
	 int xPosition= mDevicesPanel->Location.X -DEVICE_LABEL_WIDTH;
	 int nDeviceCount = mDevicesPanel->Controls->Count;

	 if(nDeviceCount >5)
	 {
		 if(xPosition <= (nDeviceCount - 5) * (-DEVICE_LABEL_WIDTH)) 
		 {
			 xPosition = (nDeviceCount - 5) *-DEVICE_LABEL_WIDTH;
		 }

		 UpdateArrowStatus();

		 mDevicesPanel->Location = System::Drawing::Point(xPosition, 0);
		 UpdateArrowStatus();
	 }
 }
void CDeviceManagePanel::mLeftArrow_Click(System::Object^  sender, System::EventArgs^  e) 
 {
	 int xPosition= mDevicesPanel->Location.X +DEVICE_LABEL_WIDTH;

	 int nDeviceCount = mDevicesPanel->Controls->Count;

	 if(nDeviceCount >5)
	 {
		if(xPosition >=0)
		{
			xPosition = 0;
		}
		

		mDevicesPanel->Location = System::Drawing::Point(xPosition, 0);
		UpdateArrowStatus();
	}
 }

void CDeviceManagePanel::UpdateArrowStatus()
 {
	 int xPosition= mDevicesPanel->Location.X;
	 int nDeviceCount = mDevicesPanel->Controls->Count;
	 if(nDeviceCount >5)
	 {
		 if(xPosition > (nDeviceCount - 5) * (-DEVICE_LABEL_WIDTH))
			 mRightArrow->Show();
		 else
			mRightArrow->Hide();


		 if(xPosition <0)
			 mLeftArrow->Show();
		 else
			 mLeftArrow->Hide();
	 }
	 else
	 {
		mRightArrow->Hide();
		mLeftArrow->Hide();
	 }
 }

/**
Update the devices on DevicePanel.
1. check the old device lable on panel (unhook event handler and clear 
2. get device count from CcdManager, if it is zero, update it and re-get
3. Add device information from CcdManager on DevicePanel. By the way, calcuate Acer and Non-Acer machines.
4. Update description information on DeviceManagement.
**/
void CDeviceManagePanel::UpdateDevicesStatus()
{
	//	CcdManager::Instance->UpdateLinkedDevices()  is run in "ResetSettingsAsync" 

	//clear device panel
	int DeviceNum = mDevicesPanel->Controls->Count;
	if(DeviceNum >0)
	{
		for(int j= 0; j<DeviceNum ; j++)
		{
			CDeviceLabel^ OldDevice = (CDeviceLabel^) mDevicesPanel->Controls[j];
			OldDevice->Click -= gcnew System::EventHandler(this, &CDeviceManagePanel::DeviceLabelClicked);
		}
		mDevicesPanel->Controls->Clear();
	}

	//Get new device information. 
	//DeviceNum = CcdManager::Instance->NumDevices;
	//if(DeviceNum == 0)
	//{
	CcdManager::Instance->UpdateLinkedDevices();
	DeviceNum = CcdManager::Instance->NumDevices;
	//}

	Logger::Instance->WriteLine("OnClickUnlink: Total device: " + DeviceNum.ToString());
	
	int nAcer = 0;
	int nNonAcer = 0;

	//Add new device info on device label and put it on device panel.
	for(int i = 0; i < DeviceNum; i++)
	{
		CDeviceLabel^ device = gcnew CDeviceLabel();		
		device->mbAcer = CcdManager::Instance->IsAcerMachine(i);
		if(device->mbAcer)
			nAcer++;
		else
			nNonAcer++;

		device->mDeviceID = CcdManager::Instance->GetDeviceID(i);
		device->mbOnline = true;

		device->mbCloudPC = false;
		device->mbSelected = false;

		device->mDeviceType = CcdManager::Instance->GetDeviceClass(i);
		device->DeviceName = CcdManager::Instance->GetDeviceName(i);

		Logger::Instance->WriteLine(	"Android Device: " + CcdManager::Instance->IsAndroidDevice(i).ToString());
		device->UpdateImg();

		device->Location = System::Drawing::Point(i * DEVICE_LABEL_WIDTH, 0);
		device->Click += gcnew System::EventHandler(this, &CDeviceManagePanel::DeviceLabelClicked);

		
		mDevicesPanel->Controls->Add(device);
	}

	//Display Acer and Non-acer machines count.
	System::String^ desc = DeviceDesc;					
	desc = desc->Replace(L"%Acer%", nAcer.ToString());
	desc = desc->Replace(L"%NonAcer%", nNonAcer.ToString());

	mDeviceDescLabel->Text =  desc;

	//Update Arrow button status (If the device is less than 5, the arrow buttons are hidden)
	UpdateArrowStatus();
}

/**
Device label is clicked. 
1. Update all device label in device panel. (selected or unselected)
2. Update the selected device infomation panel 
3. Set the variable 'mSelectedDevice" as the clicked label.
**/
void CDeviceManagePanel::DeviceLabelClicked(System::Object^  sender, System::EventArgs^  e) 
{
	CDeviceLabel^ obj = (CDeviceLabel^) sender;

	int nCount  = mDevicesPanel->Controls->Count;
	for(int i=0 ; i< nCount ; i++)
	{
		CDeviceLabel^ device = (CDeviceLabel^) mDevicesPanel->Controls[i];
		device->mbSelected = false;
		if(device == obj)
			device->mbSelected = true;

		device->UpdateImg();
	}

	//Logger::Instance->WriteLine("Device label clicked: " + obj->mDeviceID);
	mSelectedDevice = obj;
	UpdateSelectedDeviceInfo();
}

void CDeviceManagePanel::UpdateSelectedDeviceInfo()
{
	if(mSelectedDevice!= nullptr)
	{
		mDeviceName->Text = mSelectedDevice->DeviceName;

		if(mSelectedDevice->mDeviceType == CDeviceLabel::WINDOWS_PC)
			mPlatform->Text = L"Windows 7";
		else 
			mPlatform->Text = L"Android";
		
		mUnlinkBtn->Enabled = true;
	}
	else
	{
		mDeviceName->Text = String::Empty;
		mPlatform->Text = String::Empty;
		mUnlinkBtn->Enabled = false;
	}
}