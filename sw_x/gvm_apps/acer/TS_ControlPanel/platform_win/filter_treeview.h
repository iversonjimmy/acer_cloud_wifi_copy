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
#include "tristate_treeview.h"

using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

public ref class FilterTreeView : public TriStateTreeView
{
public:
    FilterTreeView();

    property List<JsonFilter^>^ Filters { List<JsonFilter^>^ get(); }
    property List<TreeNode^>^   NodeList { List<TreeNode^>^ get(); }

    /// This function fills the node and filter lists, but
    /// does not call CcdManager::UpdateDatasets().
    static void  Create(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters, bool useExisting);

	virtual void OnAfterCheck(TreeViewEventArgs^ e) override;
    virtual void OnBeforeExpand(TreeViewCancelEventArgs^ e) override;
    void         Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);

private:
    List<JsonFilter^>^ mJsonFilters;

    void UpdateChildren(TreeNode^ node);
    void UpdateParent(TreeNode^ node);
};