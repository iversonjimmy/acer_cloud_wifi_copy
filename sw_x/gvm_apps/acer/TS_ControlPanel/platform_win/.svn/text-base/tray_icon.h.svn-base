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
using namespace System::ComponentModel;
using namespace System::Windows::Forms;

ref class SettingsForm;

public ref class TrayIcon
{
public:
    TrayIcon(SettingsForm^ parent, IContainer^ container);
    ~TrayIcon();

private:
    bool							mLogoutInProgress;
    SettingsForm^			mParent;
    NotifyIcon^				mIcon;
    ContextMenuStrip^  mContextMenu;
    ToolStripMenuItem^ mFolderItem;
    ToolStripMenuItem^ mLogoutItem;
    ToolStripMenuItem^ mProgressItem;
    ToolStripMenuItem^ mProgressItem2;
    ToolStripMenuItem^ mQuitItem;
    ToolStripMenuItem^ mSettingsItem;
	ToolStripMenuItem^ mOpenAcerTS;
    ToolStripMenuItem^ mSeparatorSpace;
    ToolStripMenuItem^ mSeparatorQuit;
    ToolStripMenuItem^ mSpaceItem;
    ToolStripMenuItem^ mSyncItem;
    ToolStripMenuItem^ mWebsiteItem;

    void LogoutComplete(String^ result);
    void OnClickFolder(Object^ sender, EventArgs^ e);
    void OnClickLogout(Object^ sender, EventArgs^ e);
    void OnClickQuit(Object^ sender, EventArgs^ e);
    void OnClickSettings(Object^ sender, EventArgs^ e);
    void OnClickSync(Object^ sender, EventArgs^ e);
    void OnClickWebsite(Object^ sender, EventArgs^ e);
    void OnClickOpenAcerTS(Object^ sender, EventArgs^ e);
    void OnMouseClick(Object^ sender, MouseEventArgs^ e);
    void Opening(Object^ sender, CancelEventArgs^ e);	


};
