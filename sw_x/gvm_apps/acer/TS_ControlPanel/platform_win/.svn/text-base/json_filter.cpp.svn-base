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
#include "json_filter.h"
#include "util_funcs.h"
#include "logger.h"

JsonFilter::JsonFilter(String^ filter)
{
    mFilterText = filter;
    if (mFilterText->Length > 0) {
        mFilter = dynamic_cast<JsonArray^>(JsonConvert::Import(mFilterText));
    } else {
        mFilter = gcnew JsonArray();
    }
}

JsonFilter::~JsonFilter()
{
    // do nothing, memory is managed
}

// path does not include dataset name
void JsonFilter::Add(String^ path, TreeView^ tree, String^ dataSetName)
{
    try {
        // do not do anything if already added to filter
        if (JsonFilter::Contains(path) == FilterState::Checked) {
            return;
        }

        this->Update(path, tree, dataSetName);
        this->Minimize(tree, dataSetName);

        Logger::Instance->WriteLine(L"JsonFilter::Add " + path);
        Logger::Instance->WriteLine(this->ToString());
    } catch (Exception^ e) {
        MessageBox::Show(e->Message);
    }
}

FilterState JsonFilter::Contains(String^ path)
{
    if (mFilterText->Length <= 0) {
        return FilterState::Checked;
    }

    String^ newPath = path->Replace(L'\\', L'/');

    JsonObject^ obj = JsonFilter::Find(newPath, mFilter);
    if (obj != nullptr) {
        if (obj->Contains(L"f") == true) {
            return FilterState::Mixed;
        } else {
            return FilterState::Checked;
        }
    }

    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1) { L'/' };
    cli::array<String^>^ tokens = newPath->Split(separator);

    // first token is ""
    // no need to process last token, already tested full path
    for (int i = 1; i < tokens->Length - 1; i++) {
        String^ temp = L"";
        for (int j = 1; j <= i; j++) {
            temp += L"/" + tokens[j];
        }
        obj = JsonFilter::Find(temp, mFilter);
        if (obj == nullptr) {
            return FilterState::Unchecked;
        } else if (obj->Contains(L"f") == false) {
            return FilterState::Checked;
        }
    }

    return FilterState::Unchecked;
}

JsonObject^ JsonFilter::Find(String^ path, JsonArray^ jsonArray)
{
    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1) { L'/' };
    cli::array<String^>^ tokens = path->Split(separator);

    if (tokens->Length < 2) {
        return nullptr;
    }

    for (int i = 0; i < jsonArray->Count; i++) {
        JsonObject^ obj = dynamic_cast<JsonObject^>(jsonArray[i]);
        if (obj != nullptr && obj->Contains(L"n") == true && tokens[1] == obj[L"n"]->ToString()) {
            if (tokens->Length == 2) {
                return obj;
            } else if (obj->Contains(L"f") == true) {
                String^ newPath = L"";
                for (int j = 2; j < tokens->Length; j++) {
                    newPath += L"/" + tokens[j];
                }
                return this->Find(newPath, dynamic_cast<JsonArray^>(obj[L"f"]));
            }
        }
    }

    return nullptr;
}

void JsonFilter::Minimize(TreeView^ tree, String^ dataSetName)
{
    TreeNode^ node = nullptr;

    for (int i = 0; i < tree->Nodes->Count; i++) {
        if (tree->Nodes[i]->Text == dataSetName) {
            node = tree->Nodes[i];
            break;
        }
    }

    if (node == nullptr) {
        return;
    }

    this->Minimize(tree, node, dataSetName, true);
}

void JsonFilter::Minimize(TreeView^ tree, TreeNode^ node, String^ dataSetName, bool isRoot)
{
    if (node->Nodes->Count <= 0) {
        return;
    }

    if (node->Nodes->Count == 1 && node->Nodes[0]->Text == L"Updating...") {
        return;
    }

    bool allSync = true;
    bool noSync = true;

    for (int i = 0; i < node->Nodes->Count; i++) {
        allSync &= node->Nodes[i]->StateImageIndex == 1;
        noSync &= node->Nodes[i]->StateImageIndex == 0;
    }

    if (noSync == true && isRoot == true) {
        mFilter = gcnew JsonArray();
        mFilterText = JsonConvert::ExportToString(mFilter);
        return;
    } else if (noSync == true) {
        String^ path = UtilFuncs::StripDataSet(node->FullPath);
        this->Remove(path, tree, dataSetName, false);
        return;
    }
    
    if (allSync == true && isRoot == true) {
        mFilter = gcnew JsonArray();
        mFilterText = L"";
        return;
    } else if (allSync == true) {
        String^ path = UtilFuncs::StripDataSet(node->FullPath);
        JsonObject^ obj = this->Find(path, mFilter);
        if (obj != nullptr && obj->Contains(L"f") == true) {
            obj->Remove(L"f");
        }
        mFilterText = JsonConvert::ExportToString(mFilter);
        return;
    }

    for (int i = 0; i < node->Nodes->Count; i++) {
        this->Minimize(tree, node->Nodes[i], dataSetName, false);
    }
}

// path does not include dataset name
void JsonFilter::Remove(String^ path, TreeView^ tree, String^ dataSetName)
{
    this->Remove(path, tree, dataSetName, true);
}

// path does not include dataset name
void JsonFilter::Remove(String^ path, TreeView^ tree, String^ dataSetName, bool minimize)
{
    try {
        // do not do anything if already removed from filter
        if (JsonFilter::Contains(path) == FilterState::Unchecked) {
            return;
        }

        this->Update(path, tree, dataSetName);
        if (minimize == true) {
            this->Minimize(tree, dataSetName);
            Logger::Instance->WriteLine(L"JsonFilter::Remove " + path);
            Logger::Instance->WriteLine(this->ToString());
        }
    } catch (Exception^ e) {
        MessageBox::Show(e->Message);
    }
}

String^ JsonFilter::ToString()
{
    return mFilterText;
}

void JsonFilter::Update(TreeNode^ node, JsonArray^ jsonArray)
{
    for (int i = 0; i < node->Nodes->Count; i++) {
        if (node->Nodes[i]->StateImageIndex == 1 || node->Nodes[i]->StateImageIndex == 2) {

            // find existing if checked
            JsonObject^ obj = nullptr;
            for (int j = 0; j < jsonArray->Count; j++) {
                obj = dynamic_cast<JsonObject^>(jsonArray[j]);
                if (obj->Contains(L"n") == true
                    && obj[L"n"]->ToString() == node->Nodes[i]->Text) {
                    break;
                }
                obj = nullptr;
            }
            
            // add if checked and does not exist
            if (obj == nullptr) {
                obj = gcnew JsonObject();
                obj->Add(L"n", node->Nodes[i]->Text);
                jsonArray->Add(obj);
            }
            
            if (node->Nodes[i]->Nodes->Count > 0
                && node->Nodes[i]->Nodes[0]->Text != L"Updating...") {
                JsonArray^ arr = nullptr;
                if (obj->Contains(L"f") == true) {
                    arr = dynamic_cast<JsonArray^>(obj[L"f"]);
                } else {
                    arr = gcnew JsonArray();
                    obj->Add(L"f", arr);
                }

                this->Update(node->Nodes[i], arr);
            }
        } else {

            // remove existing if unchecked
            for (int j = 0; j < jsonArray->Count; j++) {
                JsonObject^ obj = dynamic_cast<JsonObject^>(jsonArray[j]);
                if (obj->Contains(L"n") == true
                    && obj[L"n"]->ToString() == node->Nodes[i]->Text) {
                    jsonArray->RemoveAt(j);
                    break;
                }
            }
        }
    }
}

void JsonFilter::Update(String^ path, TreeView^ tree, String^ dataSetName)
{
    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1) { L'/' };
    cli::array<String^>^ tokens = path->Split(separator);

    for (int i = 0; i < tree->Nodes->Count; i++) {
        if (tree->Nodes[i]->Text == dataSetName) {
            if (mFilterText->Length <= 0) {
                mFilter = gcnew JsonArray();
            }
            this->Update(tree->Nodes[i], mFilter);
            break;
        }
    }

    mFilterText = JsonConvert::ExportToString(mFilter);
}
