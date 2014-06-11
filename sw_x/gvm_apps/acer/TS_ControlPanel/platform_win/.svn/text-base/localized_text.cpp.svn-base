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
#include "localized_text.h"
#include "util_funcs.h"

LocalizedText::LocalizedText()
{
}

LocalizedText::~LocalizedText()
{
    // do nothing, memory is managed
}

// async operations

cli::array<String^>^ LocalizedText::ApplySettingsProgressList::get()
{
    cli::array<String^>^ list = gcnew cli::array<String^>(2);
    list[0] = L"Apply Settings...";
    list[1] = L"Done";
    return list;
}

cli::array<String^>^ LocalizedText::LoginProgressList::get()
{
    cli::array<String^>^ list = gcnew cli::array<String^>(7);
    list[0] = L"Login...";
    list[1] = L"Start media server...";
    list[2] = L"Link device...";
    list[3] = L"Set device name...";
    list[4] = L"Set location...";
    list[5] = L"Update data sets...";
    list[6] = L"Done";
    return list;
}

cli::array<String^>^ LocalizedText::LogoutProgressList::get()
{
    cli::array<String^>^ list = gcnew cli::array<String^>(3);
    list[0] = L"Unlink device...";
    list[1] = L"Log out...";
    list[2] = L"Done";
    return list;
}

cli::array<String^>^ LocalizedText::RegisterProgressList::get()
{
    cli::array<String^>^ list = gcnew cli::array<String^>(8);
    list[0] = L"Create account...";
    list[1] = L"Login...";
    list[2] = L"Start media server...";
    list[3] = L"Link device...";
    list[4] = L"Set device name...";
    list[5] = L"Set location...";
    list[6] = L"Update data sets...";
    list[7] = L"Done";
    return list;
}

cli::array<String^>^ LocalizedText::ResetSettingsProgressList::get()
{
    cli::array<String^>^ list = gcnew cli::array<String^>(2);
    list[0] = L"Retrieve Settings...";
    list[1] = L"Done";
    return list;
}

// common

String^ LocalizedText::FormTitle::get()
{
    return L"acerCloud Main App ";// + UtilFuncs::GetVersion();
}

String^ LocalizedText::FilterLabel::get()
{
    return L"Checked folders will sync to {0}.";
}

// filter treeview

String^ LocalizedText::TreeNodePlaceholder::get()
{
    return L"Updating...";
}

String^ LocalizedText::UnavailableDataset::get()
{
    return L" (Directory listing unavailable)";
}

// settings

String^ LocalizedText::ApplyButton::get()
{
    return L"Apply";
}

String^ LocalizedText::CancelButton::get()
{
    return L"Cancel";
}

String^ LocalizedText::OkayButton::get()
{
    return L"OK";
}

cli::array<String^>^ LocalizedText::TabList::get()
{
    cli::array<String^>^ tabs = gcnew cli::array<String^>(3);
    tabs[0] = L"General";
    tabs[1] = L"Account";
    tabs[2] = L"Sync Filter";
    return tabs;
}

// settings: account tab

String^ LocalizedText::AccountInfoGroupBox::get()
{
    return L"Account Information";
}

String^ LocalizedText::DeviceNameGroupBox::get()
{
    return L"Computer Name";
}

String^ LocalizedText::LogoutButton::get()
{
    return L"Log out...";
}

String^ LocalizedText::LogoutLabel::get()
{
    return L"This computer is currently logged into {0}\'s account.";
}

// settings: camera tab

String^ LocalizedText::SyncCheckBox::get()
{
    return L"Download your camera roll to this computer.";
}

// settings: general tab

String^ LocalizedText::LanguageGroupBox::get()
{
    return L"Language";
}

cli::array<String^>^ LocalizedText::LanguageList::get()
{
    cli::array<String^>^ languages = gcnew cli::array<String^>(3);
    languages[0] = L"English";
    languages[1] = L"Spanish";
    languages[2] = L"Chinese";
    return languages;
}

String^ LocalizedText::NotifyCheckBox::get()
{
    return L"Show notifications";
}

String^ LocalizedText::StartupCheckBox::get()
{
    return L"Start on system startup";
}

// setup

String^ LocalizedText::ExitQuestion::get()
{
    return L"Are you sure you want to exit?";
}

String^ LocalizedText::FinishButton::get()
{
    return L"Finish";
}

String^ LocalizedText::NextButton::get()
{
    return L"Next";
}
String^ LocalizedText::SignInButton::get()
{
    return L"Sign in";
}

String^ LocalizedText::PreviousButton::get()
{
    return L"Previous";
}
String^ LocalizedText::IAcceptButton::get()
{
    return L"Sign up";
}

String^ LocalizedText::SubmitButton ::get()
{
    return L"Submit";
}

// setup: Control panel

String^ LocalizedText::FilterTitle::get()
{
    return L"acerCloud Sync Filter";
}

String^ LocalizedText::SyncItemCaptionLabel::get()
{
    return L"Data & synchronization";
}

String^ LocalizedText::UnlinkButton::get()
{
    return L"Unlink";
}

String^ LocalizedText::DeviceMgrButton::get()
{
    return L"Device Management";
}

String^ LocalizedText::ControlPanelButton::get()
{
    return L"Control Panel";
}

String^ LocalizedText::SyncItemsPanelButton::get()
{
    return L"Sync";
}

String^ LocalizedText::DeviceMgrDescription::get()
{
	return L"acerCloud automatically syncs and share your data, you can add acer devices and two non-acer devices to acerCloud.";
}

String^ LocalizedText::ControlPanelDescription::get()
{
	return L"Selece items to sync or share in the cloud";
}

// setup: finish panel

String^ LocalizedText::FinishTitleLabel::get()
{
    return L"Verify acerCloud Account!";
}
String^ LocalizedText::DownTSLabel::get()
{
    return L"Download acerCloud Application for Your Devices";
}

String^ LocalizedText::FinishDescription::get()
{
    return L"Your acerCloud account is almost done. We will send account validation confirm form via email. Therefor you need receive your email account for account validation. Thank you very much for taking the time to register your acerCloud account.";
}
String^ LocalizedText::AndroidLabel::get()
{
    return L"Android Devices";
}
String^ LocalizedText::OtherLabel::get()
{
    return L"Others Devices";
}

String^ LocalizedText::AndroidDescription::get()
{
    return L"Do you want to sync your Android devices with acerCloud? Download acerCloud application from Android Market. Now you can scan this QR codes via your devices to download acerCloud Application in Android Market.";
}

String^ LocalizedText::OtherDescription::get()
{
	return L"Do you want to sync your devices with acerCloud? Download acerCloud application from Acer server http://www.aer.com.tw/download/.....";
}


// setup: login/register panel

String^ LocalizedText::AgreeLabel::get()
{
    return L"By proceeding you agree to our ";
}

String^ LocalizedText::EnterDetail::get()
{
    return L"Enter your details ";
}

String^ LocalizedText::AndLabel::get()
{
    return L"and";
}

String^ LocalizedText::ConfirmPasswordLabel::get()
{
    return L"Confirm Password:";
}

String^ LocalizedText::EnterAccountInfo::get()
{
    return L"Enter your account information";
}
String^ LocalizedText::ReenterEmailLabel::get()
{
    return L"Re-enter Your Email";
}

String^ LocalizedText::DeviceNameLabel::get()
{
    return L"Computer Name:";
}

String^ LocalizedText::EmailLabel::get()
{
    return L"Your Email";
}

String^ LocalizedText::ForgotPasswordLabel::get()
{
    return L"Forgot password?";
}
String^ LocalizedText::CreateAccountLabel::get()
{
    return L"Create an Acer account now?";
}

String^ LocalizedText::PrivacyLabel::get()
{
    return L"Privacy Policy";
}

String^ LocalizedText::LoginTitle::get()
{
    return L"Log into acerCloud Account";
}

String^ LocalizedText::WelcomeTitle::get()
{
    return L"Welcome to use acerCloud";
}

String^ LocalizedText::SignupDescription::get()
{
    return L"acerCloud automatically sync your data across all your computers, mobile phones and tablets, and share information among devices through acerCloud. If you already have a acerCloud account , you can sign in to start synchronization and share your data easily.";
}
String^ LocalizedText::RegisterDescription::get()
{
    return L"To use acerCloud Service, you must enter the name, password and email the basic information required to regiser with your acerCloud account";
}

String^ LocalizedText::PasswordLabel::get()
{
    return L"Password";
}

String^ LocalizedText::RegisterTitle::get()
{
    return L"Register acerCloud Account";
}

String^ LocalizedText::RegisterKeyLabel::get()
{
    return L"Registration Key:";
}
String^ LocalizedText::Min8Chars::get()
{
    return L"(8-characters minimum)";
}

String^ LocalizedText::RememberAccountLabel::get()
{
    return L"Please Remember my account.";
}
String^ LocalizedText::SyncServerLabel::get()
{
    return L"Use this computer as your media sync server.";
}
String^ LocalizedText::NoAcerAccountLabel::get()
{
    return L"Don't have an Acer Account.";
}

String^ LocalizedText::TermsServiceLabel::get()
{
    return L"Terms of Service";
}

String^ LocalizedText::UsernameLabel::get()
{
    return L"User Name:";
}
String^ LocalizedText::EnterInfoLabel::get()
{
    return L"Enter account information";
}

// setup: start panel

String^ LocalizedText::LogoLabel::get()
{
    return L"acerCloud";
}

String^ LocalizedText::NewUserRadioButton::get()
{
    return L"Create a new account.";
}

String^ LocalizedText::OldUserRadioButton::get()
{
    return L"Log into an existing account.";
}

// tray context menu

String^ LocalizedText::AllProgressItem::get()
{
    return L"All files are up to date";
}

String^ LocalizedText::DownloadProgressItem::get()
{
    return L"Downloading {0} files...";
}

String^ LocalizedText::LogoutItem::get()
{
    return L"Log out...";
}

String^ LocalizedText::OpenFolderItem::get()
{
    return L"Open acerCloud folder";
}

String^ LocalizedText::PauseSyncItem::get()
{
    return L"Pause Syncing";
}

String^ LocalizedText::OpenAcerTS::get()
{
    return L"Open acerCloud";
}


String^ LocalizedText::QuitItem::get()
{
    return L"Quit";
}



String^ LocalizedText::QuotaSpaceItem::get()
{
    return L"10.5% of 2.0 GB used";
}

String^ LocalizedText::ResumeSyncItem::get()
{
    return L"Resume Syncing";
}

String^ LocalizedText::SettingsItem::get()
{
    return L"Settings...";
}

String^ LocalizedText::UploadProgressItem::get()
{
    return L"Uploading {0} files...";
}

String^ LocalizedText::WebsiteItem::get()
{
    return L"Launch acerCloud website";
}

//setup: forget password
String^ LocalizedText::ForgetPasswordLabel::get()
{
	    return L"Forgot Your Password?";
}

String^ LocalizedText::ForgetPasswordDescription::get()
{
	return L"Please enter your email to get a password rest message sent to your password. After receiving this email, click on the embedded link and follow this instructions to get your password reset. If you do not have an email set for your account you need to contact your account administrator.";
}


//messgebox:: Non-Acer
String^ LocalizedText::NonAcerCaption::get()
{
	    return L"Limit Non_Acer Devices to Sign up";
}
String^ LocalizedText::NonAcerDescription::get()
{
	    return L"acerCloud does not support Non-Acer devices to sing up accounts. You should start to sign up from Acer devices first.";
}

//String^ LocalizedText::EmailLabel::get()
//{
//	return L"Your Email Address";
//}
