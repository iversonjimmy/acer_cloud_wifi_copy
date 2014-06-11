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

using namespace System;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

ref class SettingsForm;

public ref class SyncTab : public Panel
{
public:
    SyncTab(SettingsForm^ parent);
    ~SyncTab();

    property List<JsonFilter^>^ Filters { List<JsonFilter^>^ get(); }
    property bool               HasChanged { bool get(); void set(bool value); }
    property List<TreeNode^>^   Nodes { List<TreeNode^>^ get(); }

    void Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);

private:
    bool            mHasChanged;
    SettingsForm^   mParent;
    Label^          mLabel;
    FilterTreeView^ mTreeView;

    void OnCheck(Object^ sender, TreeViewEventArgs^ e);
};
