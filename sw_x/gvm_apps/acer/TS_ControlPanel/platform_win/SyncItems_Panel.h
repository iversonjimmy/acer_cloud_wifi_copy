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

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

public ref class CSyncItemsPanel : public Panel
{
public:
    CSyncItemsPanel(Form^ parent);
	~CSyncItemsPanel();

	void Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
	void ApplySetting(String^ accountName, String^ password);

private:
    Form^					mParent;    
	Label^					mCaptionLabel;    
	FilterTreeView^	mTreeView;

	
};