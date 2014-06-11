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

using namespace System;
using namespace System::Windows::Forms;
using namespace Jayrock::Json;
using namespace Jayrock::Json::Conversion;

enum class FilterState
{
    Checked,
    Unchecked,
    Mixed,
};

public ref class JsonFilter
{
public:
    JsonFilter(String^ filter);
    ~JsonFilter();

    void            Add(String^ path, TreeView^ tree, String^ dataSetName);
    FilterState     Contains(String^ path);
    void            Remove(String^ path, TreeView^ tree, String^ dataSetName);
    virtual String^ ToString() override;
    
private:
    String^    mFilterText;
    JsonArray^ mFilter;

    JsonObject^ Find(String^ path, JsonArray^ jsonArray);
    void        Minimize(TreeView^ tree, String^ dataSetName);
    void        Minimize(TreeView^ tree, TreeNode^ node, String^ dataSetName, bool isRoot);
    void        Remove(String^ path, TreeView^ tree, String^ dataSetName, bool minimize);
    void        Update(TreeNode^ node, JsonArray^ jsonArray);
    void        Update(String^ path, TreeView^ tree, String^ dataSetName);
};
