//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
//  This derives from a C# code project under the CPOL license.
//  http://www.codeproject.com/KB/tree/SimpleTriStateTreeView.aspx
//  Copyright © 2010 ViCon GmbH / Sebastian Grote. All Rights Reserved.
//
#include "stdafx.h"
#include "tristate_treeview.h"
#include "logger.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Text;
using namespace System::Windows::Forms;
using namespace System::Windows::Forms::VisualStyles;

TriStateTreeView::TriStateTreeView() : TreeView()
{
    mCheckBoxesVisible = false;
    mPreventCheckEvent = false;
    mStateImages = gcnew System::Windows::Forms::ImageList();

    for (int i = 0; i <= 2; i++) {
        Bitmap^ bitmap = gcnew Bitmap(16, 16);
        System::Drawing::Graphics^ gfx = Graphics::FromImage(bitmap);
        CheckBoxState state = CheckBoxState::UncheckedNormal;

        switch (i) {
        case 0: state = CheckBoxState::UncheckedNormal; break;
        case 1: state = CheckBoxState::CheckedNormal; break;
        case 2: state = CheckBoxState::MixedNormal; break;
        }

        System::Windows::Forms::CheckBoxRenderer::DrawCheckBox(gfx, System::Drawing::Point(2, 2), state);
        gfx->Save();
        mStateImages->Images->Add(bitmap);
    }
}

bool TriStateTreeView::CheckBoxes::get()
{
    return mCheckBoxesVisible;
}

void TriStateTreeView::CheckBoxes::set(bool value)
{
	mCheckBoxesVisible = value;
	TreeView::CheckBoxes = mCheckBoxesVisible;
	this->StateImageList = mCheckBoxesVisible ? mStateImages : nullptr;
}

System::Windows::Forms::ImageList^ TriStateTreeView::StateImageList::get()
{
    return TreeView::StateImageList;
}

void TriStateTreeView::StateImageList::set(System::Windows::Forms::ImageList^ value)
{
    TreeView::StateImageList = value;
}

void TriStateTreeView::Refresh()
{
    Stack<TreeNode^>^ stNodes;
    TreeNode^ tnStacked;

    TreeView::Refresh();

    if (this->CheckBoxes == false) {
        return;
    }

    TreeView::CheckBoxes = false;

    stNodes = gcnew Stack<TreeNode^>(this->Nodes->Count);
    for (int i = 0; i < this->Nodes->Count; i++) {
        stNodes->Push(this->Nodes[i]);
    }

    while (stNodes->Count > 0) {
        tnStacked = stNodes->Pop();
        if (tnStacked->StateImageIndex == -1) {
            tnStacked->StateImageIndex = tnStacked->Checked ? 1 : 0;
        }
        for (int i = 0; i < tnStacked->Nodes->Count; i++) {
            stNodes->Push(tnStacked->Nodes[i]);
        }
    }
}

void TriStateTreeView::OnLayout(LayoutEventArgs^ levent)
{
    TreeView::OnLayout(levent);
    this->Refresh();
}

void TriStateTreeView::OnAfterExpand(TreeViewEventArgs^ e)
{
    TreeView::OnAfterExpand(e);

    for (int i = 0; i < e->Node->Nodes->Count; i++) {
        if (e->Node->Nodes[i]->StateImageIndex == -1) {
            e->Node->Nodes[i]->StateImageIndex = e->Node->Nodes[i]->Checked ? 1 : 0;
        }
    }
}

void TriStateTreeView::OnAfterCheck(TreeViewEventArgs^ e)
{
    TreeView::OnAfterCheck(e);

    if (mPreventCheckEvent == true) {
        return;
    }

    this->OnNodeMouseClick(gcnew TreeNodeMouseClickEventArgs(
        e->Node, System::Windows::Forms::MouseButtons::None, 0, 0, 0));
}

void TriStateTreeView::OnNodeMouseClick(TreeNodeMouseClickEventArgs^ e)
{
    Stack<TreeNode^>^ stNodes;
    TreeNode^ tnBuffer;
    bool bMixedState;
    int iSpacing;
    int iIndex;

    TreeView::OnNodeMouseClick(e);

    mPreventCheckEvent = true;

    iSpacing = this->ImageList == nullptr ? 0 : 18;
    if ((e->X > e->Node->Bounds.Left - iSpacing
        || e->X < e->Node->Bounds.Left - (iSpacing + 16))
        && e->Button != System::Windows::Forms::MouseButtons::None) {
        return;
    }

    tnBuffer = e->Node;
    if (e->Button == System::Windows::Forms::MouseButtons::Left) {
        tnBuffer->Checked = !tnBuffer->Checked;
    }

    tnBuffer->StateImageIndex = tnBuffer->Checked == true ? 1 : tnBuffer->StateImageIndex;

    this->OnAfterCheck(gcnew TreeViewEventArgs(tnBuffer, TreeViewAction::ByMouse));

    stNodes = gcnew Stack<TreeNode^>(tnBuffer->Nodes->Count);
    stNodes->Push(tnBuffer);
    do {
        tnBuffer = stNodes->Pop();
        tnBuffer->Checked = e->Node->Checked;
        for (int i = 0; i < tnBuffer->Nodes->Count; i++) {
            stNodes->Push(tnBuffer->Nodes[i]);
        }
    } while (stNodes->Count > 0);

    bMixedState = false;
    tnBuffer = e->Node;
    while (tnBuffer->Parent != nullptr) {
        for (int i = 0; i < tnBuffer->Parent->Nodes->Count; i++) {
            TreeNode^ tnChild = tnBuffer->Parent->Nodes[i];
            bMixedState |= (tnChild->Checked != tnBuffer->Checked || tnChild->StateImageIndex == 2);
        }
        iIndex = Convert::ToInt32(tnBuffer->Checked);
        tnBuffer->Parent->Checked = bMixedState == true || (iIndex > 0);
        if (bMixedState) {
            tnBuffer->Parent->StateImageIndex = 2;
        } else {
            tnBuffer->Parent->StateImageIndex = iIndex;
        }
        tnBuffer = tnBuffer->Parent;
    }

    mPreventCheckEvent = false;
}
