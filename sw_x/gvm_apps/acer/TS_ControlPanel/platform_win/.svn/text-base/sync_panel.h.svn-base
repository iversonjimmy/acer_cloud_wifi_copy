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

using namespace System;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

ref class SetupForm;

public ref class SyncPanel : public Panel
{
public:
    SyncPanel(SetupForm^ parent);
    ~SyncPanel();

    void Apply();
    void Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);

private:
    SetupForm^      mParent;
    Label^          mTitleLabel;
    Label^          mTreeViewLabel;
    FilterTreeView^ mTreeView;

    void ApplyComplete(String^ result);
};
