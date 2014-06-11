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
#include "account_tab.h"
#include "general_tab.h"
#include "ccd_manager.h"
#include "setup_form.h"
#include "sync_tab.h"
#include "tray_icon.h"
#include "json_filter.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

public ref class SettingsForm : public Form
{
public:
	SettingsForm();
    ~SettingsForm();

    void Reset();
    void SetApplyButton(bool enable);
    void ShowSetup();
	void OpenAcerTS();
	property Form^ GetSetupForm{Form^ get(){return mSetupForm;};}

protected:
	//virtual void WndProc( Message% m ) override;

private:
    bool              mIsReset;
    Panel^            mPanel;
    AccountTab^       mAccountTab;
    GeneralTab^       mGeneralTab;
    SyncTab^          mSyncTab;
    TrayIcon^         mIcon;
    System::ComponentModel::Container^ mComponents;
    ListBox^          mTabListBox;
    //Button^           mApplyButton;
    //Button^           mCancelButton;
    //Button^           mOkayButton;
    SetupForm^        mSetupForm;

    void ApplySettingsComplete(String^ result);
    void HideForm();
    void OnChangeTab(Object^ sender, EventArgs^ e);
    //void OnClickApply(Object^ sender, EventArgs^ e);
    //void OnClickCancel(Object^ sender, EventArgs^ e);
    //void OnClickOkay(Object^ sender, EventArgs^ e);
    void OnClose(Object^ sender, System::ComponentModel::CancelEventArgs^ e);
    void ResetSettingsComplete(String^ result, List<TreeNode^>^ nodes, List<JsonFilter^>^ filters);
	void OnLoad(Object^ sender, EventArgs^ e);
};
