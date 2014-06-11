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

public ref class TriStateTreeView : public TreeView
{
public:
	TriStateTreeView();

    property bool CheckBoxes { bool get(); void set(bool value); }
    property System::Windows::Forms::ImageList^ StateImageList
    {
        System::Windows::Forms::ImageList^ get();
        void set(System::Windows::Forms::ImageList^ value);
    }

	virtual void Refresh() override;

protected:
	virtual void OnLayout(LayoutEventArgs^ levent) override;
	virtual void OnAfterExpand(TreeViewEventArgs^ e) override;
	virtual void OnAfterCheck(TreeViewEventArgs^ e) override;
	virtual void OnNodeMouseClick(TreeNodeMouseClickEventArgs^ e) override;

private:
    System::Windows::Forms::ImageList^ mStateImages;
	bool mCheckBoxesVisible;
	bool mPreventCheckEvent;
};