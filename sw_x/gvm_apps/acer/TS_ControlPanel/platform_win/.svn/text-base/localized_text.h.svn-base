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

public ref class LocalizedText 
{
public:
    static property LocalizedText^ Instance
    {
        LocalizedText^ get()
        {
            return mInstance;
        }
    }

    // async operations
    property cli::array<String^>^ ApplySettingsProgressList { cli::array<String^>^ get(); }
    property cli::array<String^>^ LoginProgressList { cli::array<String^>^ get(); }
    property cli::array<String^>^ LogoutProgressList { cli::array<String^>^ get(); }
    property cli::array<String^>^ RegisterProgressList { cli::array<String^>^ get(); }
    property cli::array<String^>^ ResetSettingsProgressList { cli::array<String^>^ get(); }

    // common
    property String^ FormTitle { String^ get(); }
    property String^ FilterLabel { String^ get(); }

    // filter treeview
    property String^ TreeNodePlaceholder { String^ get(); }
    property String^ UnavailableDataset { String^ get(); }

    // settings
    property String^ ApplyButton { String^ get(); }
    property String^ CancelButton { String^ get(); }
    property String^ OkayButton { String^ get(); }
    property cli::array<String^>^ TabList { cli::array<String^>^ get(); }

    // settings: account tab
    property String^ AccountInfoGroupBox { String^ get(); }
    property String^ DeviceNameGroupBox { String^ get(); }
    property String^ LogoutButton { String^ get(); }
    property String^ LogoutLabel { String^ get(); }

    // settings: camera tab
    property String^ SyncCheckBox { String^ get(); }

    // settings: general tab
    property String^ LanguageGroupBox { String^ get(); }
    property cli::array<String^>^ LanguageList { cli::array<String^>^ get(); }
    property String^ NotifyCheckBox { String^ get(); }
    property String^ StartupCheckBox { String^ get(); }

    // setup
    property String^ ExitQuestion { String^ get(); }
    property String^ FinishButton { String^ get(); }
    property String^ NextButton { String^ get(); }
    property String^ SignInButton { String^ get(); }
    property String^ PreviousButton { String^ get(); }
    property String^ IAcceptButton { String^ get(); }
	property String^ SubmitButton { String^ get(); }

    // setup: finish panel
    property String^ FinishTitleLabel { String^ get(); }
    property String^ DownTSLabel { String^ get(); }
    property String^ FinishDescription { String^ get(); }
	property String^ AndroidLabel { String^ get(); }
	property String^ OtherLabel { String^ get(); }
	property String^ AndroidDescription { String^ get(); }
	property String^ OtherDescription { String^ get(); }

    // setup: control panel
    property String^ FilterTitle { String^ get(); }
	property String^ SyncItemCaptionLabel { String^ get(); }
	property String^ UnlinkButton{String^ get();}
	property String^ DeviceMgrButton{String^ get();}
	property String^ ControlPanelButton{String^ get();}
	property String^ SyncItemsPanelButton{String^ get();}
	property String^ DeviceMgrDescription{String^ get();}
	property String^ ControlPanelDescription{String^ get();}


    // setup: login/register panel
    property String^ AgreeLabel { String^ get(); }
    property String^ EnterDetail { String^ get(); }
    property String^ AndLabel { String^ get(); }
    property String^ ConfirmPasswordLabel { String^ get(); }
    property String^ EnterAccountInfo { String^ get(); }
    property String^ DeviceNameLabel { String^ get(); }
    property String^ ReenterEmailLabel { String^ get(); }
    property String^ EmailLabel { String^ get(); }
    property String^ ForgotPasswordLabel { String^ get(); }
    property String^ CreateAccountLabel { String^ get(); }
    property String^ PrivacyLabel { String^ get(); }
    property String^ LoginTitle { String^ get(); }
	property String^ WelcomeTitle { String^ get(); }
	property String^ SignupDescription { String^ get(); }
	property String^ RegisterDescription { String^ get(); }
    property String^ PasswordLabel { String^ get(); }
    property String^ RegisterTitle { String^ get(); }
    property String^ RegisterKeyLabel { String^ get(); }
    property String^ Min8Chars { String^ get(); }
    property String^ RememberAccountLabel { String^ get(); }
    property String^ SyncServerLabel { String^ get(); }
    property String^ NoAcerAccountLabel { String^ get(); }
	
    property String^ TermsServiceLabel { String^ get(); }
    property String^ UsernameLabel { String^ get(); }
    property String^ EnterInfoLabel { String^ get(); }

    // setup: start panel
    property String^ LogoLabel { String^ get(); }
    property String^ NewUserRadioButton { String^ get(); }
    property String^ OldUserRadioButton { String^ get(); }

    // tray context menu
    property String^ AllProgressItem { String^ get(); }
    property String^ DownloadProgressItem { String^ get(); }
    property String^ LogoutItem { String^ get(); }
    property String^ OpenFolderItem { String^ get(); }
    property String^ PauseSyncItem { String^ get(); }
    property String^ QuitItem { String^ get(); }
    property String^ QuotaSpaceItem { String^ get(); }
    property String^ ResumeSyncItem { String^ get(); }
    property String^ SettingsItem { String^ get(); }
	property String^ OpenAcerTS { String^ get(); }
    property String^ UploadProgressItem { String^ get(); }
    property String^ WebsiteItem { String^ get(); }

    //setup: forget password
    property String^ ForgetPasswordLabel { String^ get(); }
    property String^ ForgetPasswordDescription { String^ get(); }
//    property String^ EmailLabel { String^ get(); }

	//messsage box: Non-Acer
	property String^ NonAcerCaption { String^ get(); }
	property String^ NonAcerDescription { String^ get(); }

private:
    static LocalizedText^ mInstance = gcnew LocalizedText();

    LocalizedText();
    ~LocalizedText();
};
