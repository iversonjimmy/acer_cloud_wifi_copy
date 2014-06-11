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

#include "MyButton.h"
#include "DeviceLabel.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

public ref class CDeviceManagePanel : public Panel
{
public:
    CDeviceManagePanel(Form^ parent);	
    ~CDeviceManagePanel();

	void UpdateDevicesStatus();

private:
	//static int mUnitWidth = 126;
	static		String^ DeviceDesc = L"Acer Devices (%Acer%) ; Non-Acer Devices (%NonAcer%)"; 

	Form^			mParent;
	Label^			mDescription;
	Label^			mDeviceNameLabel;
	Panel^			mInfoBarPanel;
	Label^			mActionLabel;
	Label^			mPlatformLabel;
	PictureBox^  mPlatformBarBottom;
	Label^			mCloudPC;
	Label^			mDeviceName;
	Label^			mPlatform;
	CMyButton^ mUnlinkBtn;
	PictureBox^  mRightLine;
	PictureBox^ mLeftLine;
	Panel^			mDevicesPanel;
	ListView^		mDevicesListview;
	PictureBox^		mRightArrow;
	PictureBox^		mLeftArrow;
	Label^				mDeviceDescLabel;
	CDeviceLabel^ mSelectedDevice;
    
	void OnClickUnlink();	
	System::Void mRightArrow_Click(System::Object^  sender, System::EventArgs^  e) ;			
	System::Void mLeftArrow_Click(System::Object^  sender, System::EventArgs^  e) ;
	void UpdateArrowStatus();
	void DeviceLabelClicked(System::Object^  sender, System::EventArgs^  e);
	void UpdateSelectedDeviceInfo();


};