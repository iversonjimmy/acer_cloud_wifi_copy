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

#include "progress_dialog.h"
#include "json_filter.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

public delegate void AsyncCompleteHandler(String^ result);
public delegate void LoginHandler(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
public delegate void RegisterHandler(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
public delegate void ResetSettingsHandler(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);

public ref class AsyncOperations 
{
public:
    static property AsyncOperations^ Instance
    {
        AsyncOperations^ get()
        {
            return mInstance;
        }
    }

    event AsyncCompleteHandler^ ApplySettingsComplete;
    event LoginHandler^         LoginComplete;
    event RegisterHandler^      RegisterComplete;
    event ResetSettingsHandler^ ResetSettingsComplete;
    event AsyncCompleteHandler^ LogoutComplete;

	void SetNodeCheckedStatus(List<bool>^ nodesChecked);
    bool ApplySettings(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
    bool Login(String^ user, String^ pass, String^ device, bool startPSN);
    bool Register(String^ user, String^ pass, String^ device, String^ email, bool startPSN);
    bool ResetSettings();
    bool Logout();

private:
    static AsyncOperations^ mInstance = gcnew AsyncOperations();

    bool              mIsRunning; // only one call at a time
    BackgroundWorker^ mWorker;
    ProgressDialog^   mProgressDialog;
	List<bool>^ mNodesChecked;

    AsyncOperations();
    ~AsyncOperations();

    void ApplySettingsAsync(Object^ sender, DoWorkEventArgs^ e);
    void ApplySettingsAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e);
    void ApplySettingsProgress(Object^ sender, ProgressChangedEventArgs^ e);
    void CreateSyncFilter(List<TreeNode^>^ nodes, List<JsonFilter^>^ filters, bool useExisting);
    void LoginAsync(Object^ sender, DoWorkEventArgs^ e);
    void LoginAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e);
    void LoginProgress(Object^ sender, ProgressChangedEventArgs^ e);
    void RegisterAsync(Object^ sender, DoWorkEventArgs^ e);
    void RegisterAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e);
    void RegisterProgress(Object^ sender, ProgressChangedEventArgs^ e);
    void ResetSettingsAsync(Object^ sender, DoWorkEventArgs^ e);
    void ResetSettingsAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e);
    void ResetSettingsProgress(Object^ sender, ProgressChangedEventArgs^ e);
    void LogoutAsync(Object^ sender, DoWorkEventArgs^ e);
    void LogoutAsyncComplete(Object^ sender, RunWorkerCompletedEventArgs^ e);
    void LogoutProgress(Object^ sender, ProgressChangedEventArgs^ e);
};
