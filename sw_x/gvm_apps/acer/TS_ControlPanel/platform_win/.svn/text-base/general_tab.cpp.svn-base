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
#include "general_tab.h"
#include "settings_form.h"
#include "ccd_manager.h"
#include "localized_text.h"

#using <mscorlib.dll>
#using <Interop\Interop.IWshRuntimeLibrary.1.0.dll>

using namespace IWshRuntimeLibrary;
using namespace System::IO;
using namespace System::Runtime::InteropServices;

GeneralTab::GeneralTab(SettingsForm^ parent) : Panel()
{
    mParent = parent;

    // check boxes

    mNotifyCheckBox = gcnew CheckBox();
    mNotifyCheckBox->AutoSize = true;
    mNotifyCheckBox->Location = System::Drawing::Point(7, 19);
    mNotifyCheckBox->Size = System::Drawing::Size(112, 17);
    mNotifyCheckBox->TabIndex = 0;
    mNotifyCheckBox->Text = LocalizedText::Instance->NotifyCheckBox;
    mNotifyCheckBox->UseVisualStyleBackColor = true;
    mNotifyCheckBox->Click += gcnew EventHandler(this, &GeneralTab::OnClickNotify);

    mStartupCheckBox = gcnew CheckBox();
    mStartupCheckBox->AutoSize = true;
    mStartupCheckBox->Location = System::Drawing::Point(7, 44);
    mStartupCheckBox->Size = System::Drawing::Size(133, 17);
    mStartupCheckBox->TabIndex = 1;
    mStartupCheckBox->Text = LocalizedText::Instance->StartupCheckBox;
    mStartupCheckBox->UseVisualStyleBackColor = true;
    mStartupCheckBox->Click += gcnew EventHandler(this, &GeneralTab::OnClickStartup);

    // combo boxes

    mLanguageComboBox = gcnew ComboBox();
    mLanguageComboBox->FormattingEnabled = true;
    mLanguageComboBox->Items->AddRange(LocalizedText::Instance->LanguageList);
    mLanguageComboBox->Location = System::Drawing::Point(6, 19);
    mLanguageComboBox->Size = System::Drawing::Size(334, 21);
    mLanguageComboBox->TabIndex = 0;
    mLanguageComboBox->SelectedIndex = 0;
    mLanguageComboBox->DropDownStyle = ComboBoxStyle::DropDownList;
    mLanguageComboBox->SelectedValueChanged += gcnew EventHandler(this, &GeneralTab::OnChangeLanguage);

    // group boxes

    mLanguageGroupBox = gcnew GroupBox();
    mLanguageGroupBox->Controls->Add(mLanguageComboBox);
    mLanguageGroupBox->Location = System::Drawing::Point(3, 81);
    mLanguageGroupBox->Size = System::Drawing::Size(346, 57);
    mLanguageGroupBox->TabIndex = 7;
    mLanguageGroupBox->TabStop = false;
    mLanguageGroupBox->Text = LocalizedText::Instance->LanguageGroupBox;

    mMiscGroupBox = gcnew GroupBox();
    mMiscGroupBox->Controls->Add(mStartupCheckBox);
    mMiscGroupBox->Controls->Add(mNotifyCheckBox);
    mMiscGroupBox->Location = System::Drawing::Point(3, 0);
    mMiscGroupBox->Size = System::Drawing::Size(346, 75);
    mMiscGroupBox->TabIndex = 5;
    mMiscGroupBox->TabStop = false;

    // panel

    this->Controls->Add(mLanguageGroupBox);
    this->Controls->Add(mMiscGroupBox);
    this->Location = System::Drawing::Point(117, 12);
    this->Size = System::Drawing::Size(352, 258);
    this->TabIndex = 9;

    this->Reset();
}

GeneralTab::~GeneralTab()
{
    // do nothing, memory is managed
}

void GeneralTab::Apply()
{
    CcdManager::Instance->ShowNotify = mNotifyCheckBox->Checked;

    // enable/disable startup
    try {
        String^ linkPath = Environment::GetFolderPath(
            Environment::SpecialFolder::Startup) + L"\\MyCloud.lnk";

        if (mStartupCheckBox->Checked == true) {
            cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1) { L'\\' };
            cli::array<String^>^ tokens = Application::ExecutablePath->Split(separator);

            String^ workDir = L"";
            for (int i = 0; i < tokens->Length - 1; i++) {
                workDir += tokens[i] + L"\\";
            }

            WshShell^ shell = gcnew WshShell();
            IWshShortcut^ link = dynamic_cast<IWshShortcut^>(shell->CreateShortcut(linkPath));
            link->TargetPath = Application::ExecutablePath;
            link->WorkingDirectory = workDir;
            link->Save();
        } else {
            System::IO::File::Delete(linkPath);
        }
    } catch (Exception^) {
    }

    switch (mLanguageComboBox->SelectedIndex) {
    case 1: CcdManager::Instance->Language = L"Spanish"; break;
    case 2: CcdManager::Instance->Language = L"Chinese"; break;
    default:
    case 0: CcdManager::Instance->Language = L"English"; break;
    }
}

void GeneralTab::OnClickNotify(Object^ sender, EventArgs^ e)
{
    mParent->SetApplyButton(true);
}

void GeneralTab::OnClickStartup(Object^ sender, EventArgs^ e)
{
    mParent->SetApplyButton(true);
}

void GeneralTab::OnChangeLanguage(Object^ sender, EventArgs^ e)
{
    mParent->SetApplyButton(true);
}

void GeneralTab::Reset()
{
    mNotifyCheckBox->Checked = CcdManager::Instance->ShowNotify;
    mStartupCheckBox->Checked = System::IO::File::Exists(
        Environment::GetFolderPath(Environment::SpecialFolder::Startup) + L"\\MyCloud.lnk");

    if (CcdManager::Instance->Language == L"Spanish") {
        mLanguageComboBox->SelectedIndex = 1;
    } else if (CcdManager::Instance->Language == L"Chinese") {
        mLanguageComboBox->SelectedIndex = 2;
    } else {
        mLanguageComboBox->SelectedIndex = 0;
    }
}
