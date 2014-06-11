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
#include "tray_icon.h"
#include "util_funcs.h"
#include "settings_form.h"
#include "ccd_manager.h"
#include "async_operations.h"
#include "logger.h"
#include "localized_text.h"

#using <System.dll>

using namespace System::ComponentModel;
using namespace System::Diagnostics;
using namespace System::Drawing;
using namespace System::Reflection;

TrayIcon::TrayIcon(SettingsForm^ parent, IContainer^ container)
{
    mParent = parent;

    mSpaceItem = gcnew ToolStripMenuItem();
    mSpaceItem->Text = LocalizedText::Instance->QuotaSpaceItem;
    mSpaceItem->Enabled = false;

    mFolderItem = gcnew ToolStripMenuItem();
    mFolderItem->Text = LocalizedText::Instance->OpenFolderItem;
    mFolderItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickFolder);

    mLogoutItem = gcnew ToolStripMenuItem();
    mLogoutItem->Text = LocalizedText::Instance->LogoutItem;
    mLogoutItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickLogout);

    mProgressItem = gcnew ToolStripMenuItem();
    mProgressItem->Text = LocalizedText::Instance->AllProgressItem;
    mProgressItem->Enabled = false;

    mProgressItem2 = gcnew ToolStripMenuItem();
    mProgressItem2->Enabled = false;

    mQuitItem = gcnew ToolStripMenuItem();
    mQuitItem->Text = LocalizedText::Instance->QuitItem;
    mQuitItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickQuit);

    mSettingsItem = gcnew ToolStripMenuItem();
    mSettingsItem->Text = LocalizedText::Instance->SettingsItem;
    mSettingsItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickSettings);

    mSyncItem = gcnew ToolStripMenuItem();
    mSyncItem->Text = LocalizedText::Instance->PauseSyncItem;
    mSyncItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickSync);

    mWebsiteItem = gcnew ToolStripMenuItem();
    mWebsiteItem->Text = LocalizedText::Instance->WebsiteItem;
    mWebsiteItem->Click += gcnew EventHandler(this, &TrayIcon::OnClickWebsite);

    mOpenAcerTS = gcnew ToolStripMenuItem();
    mOpenAcerTS->Text = LocalizedText::Instance->OpenAcerTS;
    mOpenAcerTS->Click += gcnew EventHandler(this, &TrayIcon::OnClickOpenAcerTS);

    mContextMenu = gcnew ContextMenuStrip();
    //mContextMenu->Items->Add(mSpaceItem);
    //mContextMenu->Items->Add(mProgressItem);
    //mContextMenu->Items->Add(L"-");
    //mContextMenu->Items->Add(mFolderItem);
	mContextMenu->Items->Add(mOpenAcerTS);
    mContextMenu->Items->Add(mWebsiteItem);
    mContextMenu->Items->Add(mSyncItem);
    //mContextMenu->Items->Add(mSettingsItem);
    mContextMenu->Items->Add(L"-");
    mContextMenu->Items->Add(mLogoutItem);
    mContextMenu->Items->Add(mQuitItem);
    mContextMenu->Opening += gcnew CancelEventHandler(this, &TrayIcon::Opening);

    mIcon = gcnew NotifyIcon(container);
    mIcon->Text = LocalizedText::Instance->FormTitle;
    mIcon->Visible = true;

#pragma push_macro("ExtractAssociatedIcon")
#undef ExtractAssociatedIcon

    //this->Icon = System::Drawing::Icon::ExtractAssociatedIcon(Application::ExecutablePath);	
	mIcon->Icon = Icon::ExtractAssociatedIcon(Application::ExecutablePath);

#pragma pop_macro("ExtractAssociatedIcon")



    mIcon->ContextMenuStrip = mContextMenu;
    mIcon->MouseClick += gcnew MouseEventHandler(this, &TrayIcon::OnMouseClick);
	mIcon->DoubleClick += gcnew EventHandler(this, &TrayIcon::OnClickOpenAcerTS);

    // async operations

    AsyncOperations::Instance->LogoutComplete += gcnew AsyncCompleteHandler(this, &TrayIcon::LogoutComplete);

    mLogoutInProgress = false;
}

TrayIcon::~TrayIcon()
{
    // do nothing, memory is managed
}

void TrayIcon::LogoutComplete(String^ result)
{
    mLogoutInProgress = false;

    if (result != L"") {
		//if( !result->Contains(L"14118") )
		{
			MessageBox::Show(result);
			return;
		}
    }

    mParent->ShowSetup();
}

void TrayIcon::OnClickFolder(Object^ sender, EventArgs^ e)
{
    UtilFuncs::ShowPath(CcdManager::Instance->Location + L"\\My Cloud");
}

void TrayIcon::OnClickLogout(Object^ sender, EventArgs^ e)
{
    mLogoutInProgress = true;
	try
	{
		AsyncOperations::Instance->Logout();
	}
	catch(Exception^ e)
	{
		
	}
}

void TrayIcon::OnClickQuit(Object^ sender, EventArgs^ e)
{
    Application::Exit();
}

void TrayIcon::OnClickSettings(Object^ sender, EventArgs^ e)
{
    mParent->Reset();
}

void TrayIcon::OnClickSync(Object^ sender, EventArgs^ e)
{
    try {
        CcdManager::Instance->IsSync = !CcdManager::Instance->IsSync;

        if (CcdManager::Instance->IsSync == true) {
            mSyncItem->Text = LocalizedText::Instance->PauseSyncItem;
        } else {
            mSyncItem->Text = LocalizedText::Instance->ResumeSyncItem;
        }
    } catch (Exception^) {
    }
}

void TrayIcon::OnClickWebsite(Object^ sender, EventArgs^ e)
{
    UtilFuncs::ShowUrl(CcdManager::Instance->Url);
}

void TrayIcon::OnClickOpenAcerTS(Object^ sender, EventArgs^ e)
{
	mParent->OpenAcerTS();
}


//void TrayIcon::OnClickTestNotify(Object^ sender, EventArgs^ e)
//{
//    if (CcdManager::Instance->ShowNotify == true) {
//        mIcon->ShowBalloonTip(1000, "Dare Taken", "You should not have done that!", ToolTipIcon::Info);
//    }
//}

void TrayIcon::Opening(Object^ sender, CancelEventArgs^ e)
{
    //if (mLogoutInProgress == true || CcdManager::Instance->IsLoggedIn == false) {
    //    e->Cancel = true;
    //    return;
    //}

    mContextMenu->Items->Clear();
    //mContextMenu->Items->Add(mSpaceItem);

    //int numDown = CcdManager::Instance->NumFilesDownload;
    //int numUp = CcdManager::Instance->NumFilesUpload;

    //if (numDown > 0 && numUp > 0) {
    //    mProgressItem->Text = String::Format(LocalizedText::Instance->DownloadProgressItem, numDown);
    //    mProgressItem2->Text = String::Format(LocalizedText::Instance->UploadProgressItem, numUp);
    //    mContextMenu->Items->Add(mProgressItem);
    //    mContextMenu->Items->Add(mProgressItem2);
    //} else if (numDown > 0) {
    //    mProgressItem->Text = String::Format(LocalizedText::Instance->DownloadProgressItem, numDown);
    //    mContextMenu->Items->Add(mProgressItem);
    //} else if (numUp > 0) {
    //    mProgressItem->Text = String::Format(LocalizedText::Instance->UploadProgressItem, numUp);
    //    mContextMenu->Items->Add(mProgressItem);
    //} else {
    //    mProgressItem->Text = LocalizedText::Instance->AllProgressItem;
    //    mContextMenu->Items->Add(mProgressItem);
    //}

    //mContextMenu->Items->Add(L"-");
    //mContextMenu->Items->Add(mFolderItem);
	mContextMenu->Items->Add(mOpenAcerTS);
    mContextMenu->Items->Add(mWebsiteItem);
    mContextMenu->Items->Add(mSyncItem);
    //mContextMenu->Items->Add(mSettingsItem);
    mContextMenu->Items->Add(L"-");
	
	if(CcdManager::Instance->IsLoggedIn)		
		mContextMenu->Items->Add(mLogoutItem);
    mContextMenu->Items->Add(mQuitItem);
}

void TrayIcon::OnMouseClick(Object^ sender, MouseEventArgs^ e)
{
    // invoke context menu on left click
    if (e->Button == MouseButtons::Left) {
        MethodInfo^ mi = mIcon->GetType()->GetMethod("ShowContextMenu",
            static_cast<BindingFlags>(BindingFlags::NonPublic | BindingFlags::Instance));
        mi->Invoke(mIcon, nullptr);
    }
}
