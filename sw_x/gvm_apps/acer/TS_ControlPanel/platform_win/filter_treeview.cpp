//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "filter_treeview.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"

FilterTreeView::FilterTreeView() : TriStateTreeView()
{
    mJsonFilters = gcnew List<JsonFilter^>(); 
}

List<JsonFilter^>^ FilterTreeView::Filters::get()
{
    return mJsonFilters;
}

List<TreeNode^>^ FilterTreeView::NodeList::get()
{
    List<TreeNode^>^ nodes = gcnew List<TreeNode^>();
    for (int i = 0; i < this->Nodes->Count; i++) {
        TreeNode^ node = this->Nodes[i];
        nodes->Add(node);
    }
    return nodes;
}

// This function fills the node and filter lists, but
// does not call CcdManager::UpdateDatasets().
void FilterTreeView::Create(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters, bool useExisting)
{
    nodes->Clear();
    filters->Clear();
    for (int i = 0; i < CcdManager::Instance->NumDataSets; i++) {

        bool isAvailable = true;
        try {
            CcdManager::Instance->SetCurrentDir(i, L"/");
        } catch (Exception^) {
            isAvailable = false;
        }

        TreeNode^ node = gcnew TreeNode(CcdManager::Instance->GetDataSetName(i));
        nodes->Add(node);

        if (useExisting == true) {
            filters->Add(gcnew JsonFilter(CcdManager::Instance->GetFilter(i)));
        } else {
            filters->Add(gcnew JsonFilter(L""));
        }

        if (isAvailable == false) {
            if (useExisting == true) {
                node->Checked = CcdManager::Instance->IsSubscribed(i);
            } else {
                node->Checked = true;
            }
            if (node->Checked == true) {
                node->StateImageIndex = 1;
            } else {
                node->StateImageIndex = 0;
            }
            node->Text += LocalizedText::Instance->UnavailableDataset;
        } else {
            int count = 0;
            int tcount = 0;
            int fcount = 0;

            for (int j = 0; j < CcdManager::Instance->NumCurrentDirEntries(i); j++) {
                TreeNode^ sub = gcnew TreeNode(CcdManager::Instance->GetCurrentDirEntryName(i, j));
                if (CcdManager::Instance->IsCurrentDirEntryDir(i, j) == true) {
                    sub->Nodes->Add(LocalizedText::Instance->TreeNodePlaceholder);
                } else {
                    sub->ImageIndex = 1;
                    sub->SelectedImageIndex = 1;
                }
                node->Nodes->Add(sub);

                FilterState state = FilterState::Checked;
                if (useExisting == true) {
                    if (CcdManager::Instance->IsSubscribed(i) == false) {
                        state = FilterState::Unchecked;
                    } else {
                        state = filters[i]->Contains(L"/" + sub->Text);
                    }
                }

                // check,StateImageIndex call order matters
                // the reverse will set all mixed states to unchecked
                if (state == FilterState::Unchecked) {
                    sub->Checked = false;
                    sub->StateImageIndex = 0;
                    fcount++;
                } else if (state == FilterState::Checked) {
                    sub->Checked = true;
                    sub->StateImageIndex = 1;
                    tcount++;
                } else {
                    sub->Checked = false;
                    sub->StateImageIndex = 2;
                }

                count++;
            }

            if (useExisting == true) {
                node->Checked = CcdManager::Instance->IsSubscribed(i);
            } else {
                node->Checked = true;
            }

            if (count > 0) {
                if (count == fcount) {
                    node->StateImageIndex = 0;
                } else if (count == tcount) {
                    node->StateImageIndex = 1;
                } else {
                    node->StateImageIndex = 2;
                }
            } else {
                if (node->Checked == true) {
                    node->StateImageIndex = 1;
                } else {
                    node->StateImageIndex = 0;
                }
            }
        }
    }
}

void FilterTreeView::OnAfterCheck(TreeViewEventArgs^ e)
{
    TriStateTreeView::OnAfterCheck(e);

    if (e->Action != TreeViewAction::ByMouse && e->Action != TreeViewAction::ByKeyboard) {
        return;
    }

    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1);
    separator[0] = L'\\';
    cli::array<String^>^ tokens = e->Node->FullPath->Split(separator);

    // return if top-level
    if (tokens->Length < 2) {
        return;
    }

    // find data set index
    int index = 0;
    for (int i = 0; i < CcdManager::Instance->NumDataSets; i++) {
        if (CcdManager::Instance->GetDataSetName(i) == tokens[0]) {
            index = i;
            break;
        }
    }

    // JsonFilter relies on StateImageIndex
    // Make sure entire tree is updated before add/remove
    if (e->Node->Checked == true) {
        e->Node->StateImageIndex = 1;
        this->UpdateParent(e->Node);
        this->UpdateChildren(e->Node);
        mJsonFilters[index]->Add(
            UtilFuncs::StripDataSet(e->Node->FullPath),
            this,
            CcdManager::Instance->GetDataSetName(index));
    } else {
        e->Node->StateImageIndex = 0;
        this->UpdateParent(e->Node);
        this->UpdateChildren(e->Node);
        mJsonFilters[index]->Remove(
            UtilFuncs::StripDataSet(e->Node->FullPath),
            this,
            CcdManager::Instance->GetDataSetName(index));
    }
}

void FilterTreeView::OnBeforeExpand(TreeViewCancelEventArgs^ e)
{
    TriStateTreeView::OnBeforeExpand(e);

    try {
        cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1);
        separator[0] = L'\\';
        cli::array<String^>^ tokens = e->Node->FullPath->Split(separator);

        // return if top-level
        if (tokens->Length < 2) {
            return;
        }

        // find data set index
        int index = 0;
        for (int i = 0; i < CcdManager::Instance->NumDataSets; i++) {
            if (CcdManager::Instance->GetDataSetName(i) == tokens[0]) {
                index = i;
                break;
            }
        }

        // strip top-level directory from path
        String^ path = UtilFuncs::StripDataSet(e->Node->FullPath);

        e->Node->Nodes->Clear();
        CcdManager::Instance->SetCurrentDir(index, path);

        for (int i = 0; i < CcdManager::Instance->NumCurrentDirEntries(index); i++) {
            TreeNode^ sub = gcnew TreeNode(CcdManager::Instance->GetCurrentDirEntryName(index, i));
            if (CcdManager::Instance->IsCurrentDirEntryDir(index, i) == true) {
                sub->Nodes->Add(LocalizedText::Instance->TreeNodePlaceholder);
            } else {
                sub->ImageIndex = 1;
                sub->SelectedImageIndex = 1;
            }
            e->Node->Nodes->Add(sub);
            
            // check,StateImageIndex call order matters
            // the reverse will set all mixed states to unchecked
            FilterState state = mJsonFilters[index]->Contains(UtilFuncs::StripDataSet(sub->FullPath));
            if (state == FilterState::Unchecked) {
                sub->Checked = false;
                sub->StateImageIndex = 0;
            } else if (state == FilterState::Checked) {
                sub->Checked = true;
                sub->StateImageIndex = 1;
            } else {
                sub->Checked = false;
                sub->StateImageIndex = 2;
            }

            this->UpdateParent(sub);
        }
    } catch (Exception^ e) {
        MessageBox::Show(e->Message);
    }
}

void FilterTreeView::Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    mJsonFilters = filters;
    this->Nodes->Clear();
    for (int i = 0; i < nodes->Count; i++) {
        this->Nodes->Add(nodes[i]);
    }
}

void FilterTreeView::UpdateChildren(TreeNode^ node)
{
    if (node == nullptr) {
        return;
    }

    for (int i = 0; i < node->Nodes->Count; i++) {
        node->Nodes[i]->StateImageIndex = node->StateImageIndex;
        this->UpdateChildren(node->Nodes[i]);
    }
}

void FilterTreeView::UpdateParent(TreeNode^ node)
{
    if (node == nullptr || node->Parent == nullptr) {
        return;
    }

    int tcount = 0;
    int fcount = 0;
    for (int i = 0; i < node->Parent->Nodes->Count; i++) {
        if (node->Parent->Nodes[i]->StateImageIndex == 0) {
            fcount++;
        } else if (node->Parent->Nodes[i]->StateImageIndex == 1) {
            tcount++;
        }
    }

    if (node->Parent->Nodes->Count == fcount) {
        node->Parent->StateImageIndex = 0;
    } else if (node->Parent->Nodes->Count == tcount) {
        node->Parent->StateImageIndex = 1;
    } else {
        node->Parent->StateImageIndex = 2;
    }

    this->UpdateParent(node->Parent);
}
