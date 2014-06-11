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
#include "SyncItems_Panel.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "localized_text.h"
#include "Layout.h"
#include "async_operations.h"
#include "logger.h"
#include "SyncItem.h"


using namespace System::Collections;
using namespace System::IO;

CSyncItemsPanel::CSyncItemsPanel(Form^ parent) : Panel()
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
	this->mCaptionLabel->Name = L"mCaptionLabel";
	this->mCaptionLabel->Size = System::Drawing::Size(453, 20);
	this->mCaptionLabel->TabIndex = 2;
	this->mCaptionLabel->Text = L"Select items to sync or share in the cloud.";
	this->mCaptionLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

	//Set size and location

	int xPosition = 10;
	int yPosition = 0;

    mCaptionLabel->Location = System::Drawing::Point(xPosition, yPosition);
	mCaptionLabel->Size = System::Drawing::Size(Panel_Width-25, 20);

	mTreeView->Location = System::Drawing::Point(xPosition, yPosition+=(mCaptionLabel->Size.Height - 0.5));
	mTreeView->Size = System::Drawing::Size(mCaptionLabel->Size.Width, Panel_Height - 180);


	//int xPosition = 25;
	//int yPosition = 15;
	//mCaptionLabel->Location = System::Drawing::Point(xPosition, yPosition);
	//mTreeView->Location = System::Drawing::Point(xPosition, yPosition+=(mCaptionLabel->Size.Height));
	//mTreeView->AutoSize = false;
	//mTreeView->Size = System::Drawing::Size(453, 300);

	//mListBox->Location = System::Drawing::Point(xPosition, yPosition+=(mCaptionLabel->Size.Height - 0.5));
	//mListBox->Size = System::Drawing::Size(mCaptionLabel->Size.Width, Panel_Height - 145);

    // panel
    //this->controls->add(mlistbox);
	this->Controls->Add(mTreeView);
	this->Controls->Add(mCaptionLabel);
	
    this->Location = System::Drawing::Point(3, 37);
	this->Size = System::Drawing::Size(Panel_Width, Panel_Height);	
    this->TabIndex = 1;
	
}


void CSyncItemsPanel::Reset(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
	System::Windows::Forms::TreeNode^ master = gcnew System::Windows::Forms::TreeNode();	
	master->Text = L"Make this PC as master Device";
	//master->Checked = true;
	master->StateImageIndex = 0;

	nodes->Add(master);
    mTreeView->Reset(nodes, filters);

    //mTreeViewLabel->Text = String::Format(
    //    LocalizedText::Instance->FilterLabel,
    //    CcdManager::Instance->Location);
}

void CSyncItemsPanel::ApplySetting(String^ accountName, String^ password)
{
	if (CcdManager::Instance->IsLoggedIn == false)
	{
        return;
    }

	try
	{
		if(mTreeView->NodeList[mTreeView->NodeList->Count-1]->Checked)
		{
			Logger::Instance->WriteLine(L"Start PSN node");
			
			UtilFuncs::EnablePSN();
	//        CcdManager::Instance->StartPSN = true;
		}
		else
		{
			UtilFuncs::DisablePSN();
			//CcdManager::Instance->UnregisterStorageNode();
	//	    CcdManager::Instance->StartPSN = false;
		}
	}
	catch(Exception^ ex) 
	{
		Logger::Instance->WriteLine(ex->Message);
	}
    AsyncOperations::Instance->ApplySettings(mTreeView->NodeList, mTreeView->Filters);
}

CSyncItemsPanel::~CSyncItemsPanel()
{
    // do nothing, memory is managed
}
