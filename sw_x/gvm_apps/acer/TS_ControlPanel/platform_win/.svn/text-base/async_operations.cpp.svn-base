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
#include "async_operations.h"
#include "ccd_manager.h"
#include "util_funcs.h"
#include "logger.h"
#include "filter_treeview.h"
#include "async_operations.h"
#include "localized_text.h"

using namespace System;
using namespace System::Collections;
using namespace System::ComponentModel;

AsyncOperations::AsyncOperations()
{
    mWorker = nullptr;
    mProgressDialog = nullptr;
    mIsRunning = false;
}

AsyncOperations::~AsyncOperations()
{
    // do nothing, memory is managed
}

void AsyncOperations::SetNodeCheckedStatus(List<bool>^ nodesChecked)
{
	mNodesChecked = nodesChecked;
}

bool AsyncOperations::ApplySettings(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters)
{
    if (mIsRunning == true) {
        return false;
    }

    ArrayList^ list = gcnew ArrayList();
    list->Add(nodes);
    list->Add(filters);
	list->Add(mNodesChecked);

    mWorker = gcnew BackgroundWorker();
    mWorker->WorkerReportsProgress = true;
    mWorker->WorkerSupportsCancellation = true;
    mWorker->DoWork += gcnew DoWorkEventHandler(this, &AsyncOperations::ApplySettingsAsync);
    mWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &AsyncOperations::ApplySettingsAsyncComplete);
    mWorker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &AsyncOperations::ApplySettingsProgress);
    mWorker->RunWorkerAsync(list);

    if (mProgressDialog == nullptr) {
        mProgressDialog = gcnew ProgressDialog();
    }
    mProgressDialog->ProgressLabel = LocalizedText::Instance->ApplySettingsProgressList[0];
    mProgressDialog->ProgressValue = 0;
    mProgressDialog->Show();

    mIsRunning = true;

    return true;
}

void AsyncOperations::ApplySettingsAsync(Object^ sender, DoWorkEventArgs^ e)
{
    BackgroundWorker^ worker = dynamic_cast<BackgroundWorker^>(sender);
    ArrayList^ input = dynamic_cast<ArrayList^>(e->Argument);
    List<TreeNode^>^ nodes = dynamic_cast<List<TreeNode^>^>(input[0]);
    List<JsonFilter^>^ filters = dynamic_cast<List<JsonFilter^>^>(input[1]);
	List<bool>^ nodesChecked = dynamic_cast<List<bool>^>(input[2]);

    int step = 1;
    int progress = 0;
    if (nodes->Count > 0) {
        step = 100 / nodes->Count;
    }

    try {
        bool isSync = CcdManager::Instance->IsSync;
        CcdManager::Instance->IsSync = false;
	
		//if(nodesChecked!=nullptr)
		//	for(int j =0; j<nodesChecked->Count ;j++)
		//	{
		//		nodes[j]->Checked = nodesChecked[j];
		//	}

  //      for (int i = 0; i < nodes->Count; i++) {
		for(int i=0 ; i<nodes->Count ; i++)
		{
            try {
                Logger::Instance->WriteLine(L"ApplySettingsAsync " + nodes[i]->Text + L" " + nodes[i]->Checked);
                //if (nodes[i]->Checked == true) {
				if(nodesChecked[i] == true)
				{
                    CcdManager::Instance->AddSubscription(i);
                    CcdManager::Instance->SetFilter(i, filters[i]->ToString());
                } else {
                    CcdManager::Instance->DeleteSubscription(i);
                }
            } catch (Exception^ ex) {
                e->Result = ex->Message;
            }

            progress += step;
            worker->ReportProgress(progress);
        }

        CcdManager::Instance->IsSync = isSync;
    } catch (Exception^ ex) {
        e->Result = ex->Message;
    }

    worker->ReportProgress(100);
    e->Result = L"";
}

void AsyncOperations::ApplySettingsAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e)
{
    mIsRunning = false;
    mProgressDialog->Hide();
    this->ApplySettingsComplete(e->Result->ToString());
}

void AsyncOperations::ApplySettingsProgress(Object^ sender, ProgressChangedEventArgs^ e)
{
    int value = e->ProgressPercentage;
    mProgressDialog->ProgressValue = value;
    cli::array<String^>^ list = LocalizedText::Instance->ApplySettingsProgressList;
    switch (value) {
    case 0: mProgressDialog->ProgressLabel = list[0]; break;
    case 100: mProgressDialog->ProgressLabel = list[1]; break;
    }
}

void AsyncOperations::CreateSyncFilter(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters, bool useExisting)
{
    CcdManager::Instance->UpdateDataSets();
    FilterTreeView::Create(nodes, filters, useExisting);
}

bool AsyncOperations::Login(String^ user, String^ pass, String^ device, bool startPSN)
{
    if (mIsRunning == true) {
        return false;
    }

    ArrayList^ list = gcnew ArrayList();
    list->Add(user);
    list->Add(pass);
    list->Add(device);
    list->Add(startPSN);

    mWorker = gcnew BackgroundWorker();
    mWorker->WorkerReportsProgress = true;
    mWorker->WorkerSupportsCancellation = true;
    mWorker->DoWork += gcnew DoWorkEventHandler(this, &AsyncOperations::LoginAsync);
    mWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &AsyncOperations::LoginAsyncComplete);
    mWorker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &AsyncOperations::LoginProgress);
    mWorker->RunWorkerAsync(list);

    if (mProgressDialog == nullptr) {
        mProgressDialog = gcnew ProgressDialog();
    }
    mProgressDialog->ProgressLabel = LocalizedText::Instance->LoginProgressList[0];
    mProgressDialog->ProgressValue = 0;
    mProgressDialog->Show();

    mIsRunning = true;

    return true;
}

void AsyncOperations::LoginAsync(Object^ sender, DoWorkEventArgs^ e)
{
    BackgroundWorker^ worker = dynamic_cast<BackgroundWorker^>(sender);
    ArrayList^ input = dynamic_cast<ArrayList^>(e->Argument);
    bool startPSN = Convert::ToBoolean(input[3]);

    List<TreeNode^>^ nodes = gcnew List<TreeNode^>();
    List<JsonFilter^>^ filters = gcnew List<JsonFilter^>();
    ArrayList^ output = gcnew ArrayList();
    output->Add(L"");
    output->Add(nodes);
    output->Add(filters);

    e->Result = output;

    // login

    try {
        CcdManager::Instance->Login(input[0]->ToString(), input[1]->ToString());
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(16);

    // HACK
    try {
		//if (startPSN)
		//{
		//	CcdManager::Instance->RegisterStorageNode();
		//}
		//else 
		//{			
		//	CcdManager::Instance->UnregisterStorageNode();
		//}
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(32);

    // link device

    try {
        CcdManager::Instance->Link(input[2]->ToString());
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(48);

    try {
        CcdManager::Instance->DeviceName = input[2]->ToString();
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(64);

    try {
        CcdManager::Instance->Location = UtilFuncs::GetUserHomeDir();
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(80);

    try {
        this->CreateSyncFilter(nodes, filters, false);
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(100);
}

void AsyncOperations::LoginAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e)
{
    mIsRunning = false;
    mProgressDialog->Hide();
    ArrayList^ output = dynamic_cast<ArrayList^>(e->Result);
    this->LoginComplete(
        output[0]->ToString(),
        dynamic_cast<List<TreeNode^>^>(output[1]),
        dynamic_cast<List<JsonFilter^>^>(output[2]));

	UtilFuncs::NotifyLoginAndPSNChange();
}

void AsyncOperations::LoginProgress(Object^ sender, ProgressChangedEventArgs^ e)
{
    int value = e->ProgressPercentage;
    mProgressDialog->ProgressValue = value;
    cli::array<String^>^ list = LocalizedText::Instance->LoginProgressList;
    switch (value) {
    case 0: mProgressDialog->ProgressLabel = list[0]; break;
    case 16: mProgressDialog->ProgressLabel = list[1]; break;
    case 32: mProgressDialog->ProgressLabel = list[2]; break;
    case 48: mProgressDialog->ProgressLabel = list[3]; break;
    case 64: mProgressDialog->ProgressLabel = list[4]; break;
    case 80: mProgressDialog->ProgressLabel = list[5]; break;
    case 100: mProgressDialog->ProgressLabel = list[6]; break;
    }
}

bool AsyncOperations::Register(String^ user, String^ pass, String^ device, String^ email, bool startPSN)
{
    if (mIsRunning == true) {
        return false;
    }

    ArrayList^ list = gcnew ArrayList();
    list->Add(user);
    list->Add(pass);
    list->Add(device);
    list->Add(email);
    list->Add(startPSN);

    mWorker = gcnew BackgroundWorker();
    mWorker->WorkerReportsProgress = true;
    mWorker->WorkerSupportsCancellation = true;
    mWorker->DoWork += gcnew DoWorkEventHandler(this, &AsyncOperations::RegisterAsync);
    mWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &AsyncOperations::RegisterAsyncComplete);
    mWorker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &AsyncOperations::RegisterProgress);
    mWorker->RunWorkerAsync(list);

    if (mProgressDialog == nullptr) {
        mProgressDialog = gcnew ProgressDialog();
    }
    mProgressDialog->ProgressLabel = LocalizedText::Instance->RegisterProgressList[0];
    mProgressDialog->ProgressValue = 0;
    mProgressDialog->Show();

    mIsRunning = true;

    return true;
}

void AsyncOperations::RegisterAsync(Object^ sender, DoWorkEventArgs^ e)
{
    BackgroundWorker^ worker = dynamic_cast<BackgroundWorker^>(sender);
    ArrayList^ input = dynamic_cast<ArrayList^>(e->Argument);
    bool startPSN = Convert::ToBoolean(input[4]);

    List<TreeNode^>^ nodes = gcnew List<TreeNode^>();
    List<JsonFilter^>^ filters = gcnew List<JsonFilter^>();
    ArrayList^ output = gcnew ArrayList();
    output->Add(L"");
    output->Add(nodes);
    output->Add(filters);

    e->Result = output;

    // register

    try {
        CcdManager::Instance->Register(input[0]->ToString(), input[1]->ToString(), input[3]->ToString());
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(14);

    // login

    try {
        CcdManager::Instance->Login(input[3]->ToString(), input[1]->ToString());
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(28);

    // HACK
    try 
	{
		//if (startPSN)
		//	CcdManager::Instance->RegisterStorageNode();
		//else 
		//	CcdManager::Instance->UnregisterStorageNode();
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(42);

    // link device

    try {
        CcdManager::Instance->Link(input[2]->ToString());
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(56);

    try {
        CcdManager::Instance->DeviceName = input[2]->ToString();
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(70);

    try {
        CcdManager::Instance->Location = UtilFuncs::GetUserHomeDir();
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(84);

    try {
        this->CreateSyncFilter(nodes, filters, false);
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(100);
}

void AsyncOperations::RegisterAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e)
{
    mIsRunning = false;
    mProgressDialog->Hide();
    ArrayList^ output = dynamic_cast<ArrayList^>(e->Result);
    this->RegisterComplete(
        output[0]->ToString(),
        dynamic_cast<List<TreeNode^>^>(output[1]),
        dynamic_cast<List<JsonFilter^>^>(output[2]));
	UtilFuncs::NotifyLoginAndPSNChange();
}

void AsyncOperations::RegisterProgress(Object^ sender, ProgressChangedEventArgs^ e)
{
    int value = e->ProgressPercentage;
    mProgressDialog->ProgressValue = value;
    cli::array<String^>^ list = LocalizedText::Instance->RegisterProgressList;
    switch (value) {
    case 0: mProgressDialog->ProgressLabel = list[0]; break;
    case 14: mProgressDialog->ProgressLabel = list[1]; break;
    case 28: mProgressDialog->ProgressLabel = list[2]; break;
    case 42: mProgressDialog->ProgressLabel = list[3]; break;
    case 56: mProgressDialog->ProgressLabel = list[4]; break;
    case 70: mProgressDialog->ProgressLabel = list[5]; break;
    case 84: mProgressDialog->ProgressLabel = list[6]; break;
    case 100: mProgressDialog->ProgressLabel = list[7]; break;
    }
}

bool AsyncOperations::ResetSettings()
{
    if (mIsRunning == true) {
        return false;
    }

    mWorker = gcnew BackgroundWorker();
    mWorker->WorkerReportsProgress = true;
    mWorker->WorkerSupportsCancellation = true;
    mWorker->DoWork += gcnew DoWorkEventHandler(this, &AsyncOperations::ResetSettingsAsync);
    mWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &AsyncOperations::ResetSettingsAsyncComplete);
    mWorker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &AsyncOperations::ResetSettingsProgress);
    mWorker->RunWorkerAsync();

    if (mProgressDialog == nullptr) {
        mProgressDialog = gcnew ProgressDialog();
    }
    mProgressDialog->ProgressLabel = LocalizedText::Instance->ResetSettingsProgressList[0];
    mProgressDialog->ProgressValue = 0;
    mProgressDialog->Show();

    mIsRunning = true;

    return true;
}

void AsyncOperations::ResetSettingsAsync(Object^ sender, DoWorkEventArgs^ e)
{
    BackgroundWorker^ worker = dynamic_cast<BackgroundWorker^>(sender);

    List<TreeNode^>^ nodes = gcnew List<TreeNode^>();
    List<JsonFilter^>^ filters = gcnew List<JsonFilter^>();
    ArrayList^ output = gcnew ArrayList();
    output->Add(L"");
    output->Add(nodes);
    output->Add(filters);

    e->Result = output;

	//CcdManager::Instance->UpdateLinkedDevices();
	//int DeviceNum = CcdManager::Instance->NumDevices;
	//Logger::Instance->WriteLine("OnClickUnlink: Total device: " + DeviceNum.ToString());
	//
	////MessageBox::Show(DeviceNum);
	//for(int i = 0; i < DeviceNum; i++)
	//{
	//	Logger::Instance->WriteLine(	"Device ID: " +CcdManager::Instance->GetDeviceID(i).ToString() +
	//													"\nDevice Name: " + CcdManager::Instance->GetDeviceName(i) +
	//													"\nDevice Type: " +  CcdManager::Instance->GetDeviceClass(i));

	//	Logger::Instance->WriteLine(	"Acer Machine: " + CcdManager::Instance->IsAcerMachine(i).ToString());
	//	Logger::Instance->WriteLine(	"Android Device: " + CcdManager::Instance->IsAndroidDevice(i).ToString());
	//}	


    try {
        this->CreateSyncFilter(nodes, filters, true);
    } catch (Exception^ ex) {
        output[0] = ex->Message;
        return;
    }

    worker->ReportProgress(100);
}

void AsyncOperations::ResetSettingsAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e)
{
    mIsRunning = false;
    mProgressDialog->Hide();
    ArrayList^ output = dynamic_cast<ArrayList^>(e->Result);
    this->ResetSettingsComplete(
        output[0]->ToString(),
        dynamic_cast<List<TreeNode^>^>(output[1]),
        dynamic_cast<List<JsonFilter^>^>(output[2]));
}

void AsyncOperations::ResetSettingsProgress(Object^ sender, ProgressChangedEventArgs^ e)
{
    int value = e->ProgressPercentage;
    mProgressDialog->ProgressValue = value;
    cli::array<String^>^ list = LocalizedText::Instance->ResetSettingsProgressList;
    switch (value) {
    case 0: mProgressDialog->ProgressLabel = list[0]; break;
    case 100: mProgressDialog->ProgressLabel = list[1]; break;
    }
}

bool AsyncOperations::Logout()
{
    if (mIsRunning == true) {
        return false;
    }

    mWorker = gcnew BackgroundWorker();
    mWorker->WorkerReportsProgress = true;
    mWorker->WorkerSupportsCancellation = true;
    mWorker->DoWork += gcnew DoWorkEventHandler(this, &AsyncOperations::LogoutAsync);
    mWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &AsyncOperations::LogoutAsyncComplete);
    mWorker->ProgressChanged += gcnew ProgressChangedEventHandler(this, &AsyncOperations::LogoutProgress);
    mWorker->RunWorkerAsync();

    if (mProgressDialog == nullptr) {
        mProgressDialog = gcnew ProgressDialog();
    }
    mProgressDialog->ProgressLabel = LocalizedText::Instance->LogoutProgressList[0];
    mProgressDialog->ProgressValue = 0;
    mProgressDialog->Show();

    mIsRunning = true;

    return true;
}
 
void AsyncOperations::LogoutAsync(Object^ sender, DoWorkEventArgs^ e)
{
    BackgroundWorker^ worker = dynamic_cast<BackgroundWorker^>(sender);

    // unlink device

    try {
        CcdManager::Instance->Unlink();
    } catch (Exception^ ex) {
        e->Result = ex->Message;
        return;
    }

    worker->ReportProgress(50);

    // logout

    try {
        CcdManager::Instance->Logout(true);
    } catch (Exception^ ex) {
        e->Result = ex->Message;
        return;
    }

    worker->ReportProgress(100);
    e->Result = L"";
}

void AsyncOperations::LogoutAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e)
{
    mIsRunning = false;
    mProgressDialog->Hide();
    this->LogoutComplete(e->Result->ToString());
	UtilFuncs::NotifyLoginAndPSNChange();
}

void AsyncOperations::LogoutProgress(Object^ sender, ProgressChangedEventArgs^ e)
{
    int value = e->ProgressPercentage;
    mProgressDialog->ProgressValue = value;
    cli::array<String^>^ list = LocalizedText::Instance->LogoutProgressList;
    switch (value) {
    case 0: mProgressDialog->ProgressLabel = list[0]; break;
    case 50: mProgressDialog->ProgressLabel = list[1]; break;
    case 100: mProgressDialog->ProgressLabel = list[2]; break;
    }
}