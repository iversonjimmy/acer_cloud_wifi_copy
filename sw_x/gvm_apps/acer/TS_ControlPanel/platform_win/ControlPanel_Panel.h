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


#include "filter_treeview.h"
#include "SyncItem.h"
using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

public ref class CControlPanel : public Panel
{
public:
    CControlPanel(Form^ parent);
	~CControlPanel();

	void Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
	void ApplySetting(String^ accountName, String^ password);

private:
    Form^					mParent;
    //CheckedListBox^	mListBox;
	Label^					mCaptionLabel;    
	FilterTreeView^	mTreeView;

	CSyncItem^			mCameraRollItem;
	CSyncItem^			mCloudPCItem;
	List<bool>^			mNodesChecked;
};