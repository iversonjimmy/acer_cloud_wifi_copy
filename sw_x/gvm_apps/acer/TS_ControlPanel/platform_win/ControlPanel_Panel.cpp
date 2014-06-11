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
#include "ControlPanel_Panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"
#include "Layout.h"
#include "async_operations.h"
#include "logger.h"


using namespace System::Collections;
using namespace System::IO;

CControlPanel::CControlPanel(Form^ parent) : Panel()
{
    mParent = parent;

    // labels

	//int nFontSize = 9;
	mTreeView = gcnew FilterTreeView();

    mTreeView->CheckBoxes = true;    
    mTreeView->TabIndex = 1;
    mTreeView->ImageList = gcnew ImageList();
	mTreeView->ImageList->Images->Add(System::Drawing::Image::FromFile(L"images\\folder.png"));
    mTreeView->ImageList->Images->Add(System::Drawing::Image::FromFile(L"images\\generic_file.png"));

	this->mCaptionLabel = (gcnew System::Windows::Forms::Label());
	this->mCaptionLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
	this->mCaptionLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mCaptionLabel->Location = System::Drawing::Point(25, 15);
	this->mCaptionLabel->Name = L"mCaptionLabel";
	this->mCaptionLabel->Size = System::Drawing::Size(453, 20);
	this->mCaptionLabel->TabIndex = 2;
	this->mCaptionLabel->Text = L"Select items to sync or share in the cloud.";
	this->mCaptionLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

	//Set size and location
	int xPosition = mCaptionLabel->Location.X;
	int yPosition = mCaptionLabel->Location.Y + mCaptionLabel->Size.Height;

	mCameraRollItem = gcnew CSyncItem();
	mCameraRollItem->LabelText = L"Camera roll stream";
	mCameraRollItem->Location = System::Drawing::Point(xPosition, yPosition);	
	//mCameraRollItem->Location = System::Drawing::Point(xPosition, yPosition+=(item3->Size.Height));
	mCameraRollItem->ChangeIconType = IconType::Camera;
	mCameraRollItem->GetButton->Hide();

	mCloudPCItem = gcnew CSyncItem();
	mCloudPCItem->LabelText = L"Make this Device as the Cloud PC";
	mCloudPCItem->Location = System::Drawing::Point(xPosition, yPosition+=(mCameraRollItem->Size.Height));
	mCloudPCItem->ChangeIconType = IconType::CloudPC;
	mCloudPCItem->GetButton->Hide();

	this->Controls->Add(mCaptionLabel);
	//this->Controls->Add(item);
	//this->Controls->Add(item1);
	//this->Controls->Add(item2);
	//this->Controls->Add(item3);
	this->Controls->Add(mCameraRollItem);
	this->Controls->Add(mCloudPCItem);
    
	Size = System::Drawing::Size(730, 414);
	Location = System::Drawing::Point(0, 34);		
	BackColor = System::Drawing::Color::White;

	//this->Size = System::Drawing::Size(722, 362);
	//this->Location = System::Drawing::Point(3, 37);

    this->TabIndex = 1;
}


void CControlPanel::Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
	//System::Windows::Forms::TreeNode^ master = gcnew System::Windows::Forms::TreeNode();	
	//master->Text = L"Make this PC as master Device";
	//master->StateImageIndex = 0;
	//nodes->Add(master);

	if(mNodesChecked == nullptr)
		mNodesChecked= gcnew List<bool>();
	else
		mNodesChecked->Clear();

	mCloudPCItem->IsChecked = CcdManager::Instance->IsLogInPSN;

	for (int i = 0; i < nodes->Count; i++) 
	{
		TreeNode^ node = nodes[i];
		if(node->Text->Contains(L"CameraRoll"))
		{
			mCameraRollItem->mnIndexInList = i;
			mCameraRollItem->IsChecked = node->Checked;

		} else if( node->Text->Contains(L"clear.fi"))
		{
			node->Checked = true;
		}
		else
		{
			node->Checked = false;
		}

		mNodesChecked->Add(node->Checked);
    }
    mTreeView->Reset(nodes, filters);

    //mTreeViewLabel->Text = String::Format(
    //    LocalizedText::Instance->FilterLabel,
    //    CcdManager::Instance->Location);
}

void CControlPanel::ApplySetting(String^ accountName, String^ password)
{
	if (CcdManager::Instance->IsLoggedIn == false)
	{
        return;
    }
		
	try
	{
		//if(mTreeView->NodeList[mTreeView->NodeList->Count-1]->Checked)		
				
		if(mCloudPCItem->IsChecked)
		{
			//Logger::Instance->WriteLine(L"Start PSN node");
			//CcdManager::Instance->RegisterStorageNode();
			UtilFuncs::EnablePSN();
	        //CcdManager::Instance->StartPSN = true;
		}
		else
		{
			UtilFuncs::DisablePSN();
			//CcdManager::Instance->UnregisterStorageNode();	
			//CcdManager::Instance->StartPSN = false;
		}

		//mCameraRollItem->mNode->Checked = mCameraRollItem->IsChecked;
		if(mNodesChecked != nullptr)
			mNodesChecked[mCameraRollItem->mnIndexInList] = mCameraRollItem->IsChecked;
		
	}
	catch(Exception^ ex) 
	{
		Logger::Instance->WriteLine(ex->Message);
		
	}
	AsyncOperations::Instance->SetNodeCheckedStatus(mNodesChecked);
    AsyncOperations::Instance->ApplySettings(mTreeView->NodeList, mTreeView->Filters);
}

CControlPanel::~CControlPanel()
{
    // do nothing, memory is managed
}
