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

#include "json_filter.h"
#include "filter_treeview.h"
#include "ControlPanel_Panel.h"
#include "SyncItems_Panel.h"
#include "DeviceManagement_Panel.h"
#include "TabButton.h"


using namespace System;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

ref class SetupForm;

public ref class CTabController : public Panel
{
public:
    CTabController(SetupForm^ parent);
	//void CTabController::roundButton_Paint( Object^ sender,  System::Windows::Forms::PaintEventArgs^ e );
	void ResetSyncItems(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
	void ControlPanelState();
	//void SyncPanelState();
	void DeviceMgrState();
	void LogoutComplete(String^ result);
	void Apply();
	void SetAccountAndPassword(String^ name, String^ password);
	bool IsControlPanelMode();
    ~CTabController();

private:
	bool						mIsControlPanelState;
    SetupForm^			mParent;
	CTabButton^		mDeviceMgrButton;
	CTabButton^		mControlPanelButton;
	PictureBox^			pictureBox_Line;
	Label^					mAccountLabel;
	
	CControlPanel^	mControlPanel;	
	CDeviceManagePanel^ mDeviceMgrPanel;
	
	String^					mAccountName;
	String^					mPassword;

private:
	void OnClickControlPanel();//Object^ sender, EventArgs^ e);
	void OnClickDeviceMgr();//Object^ sender, EventArgs^ e);

};
