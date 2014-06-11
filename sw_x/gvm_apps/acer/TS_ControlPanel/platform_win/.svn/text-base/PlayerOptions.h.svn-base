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

#include "util_funcs.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Diagnostics;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

/// <summary>
/// Summary for Form1
///
/// WARNING: If you change the name of this class, you will need to change the
///          'Resource File Name' property for the managed resource compiler tool
///          associated with all .resx files this class depends on.  Otherwise,
///          the designers will not be able to interact properly with localized
///          resources associated with this form.
/// </summary>
public ref class PlayerOptionForm : public System::Windows::Forms::Form
{

public:
	PlayerOptionForm(void)
	{
		InitializeComponent();

		mWinLibOldChecked = false;
		mWMPOldChecked=false;
		miTuneOldCheck=false;

		GetPlayerOption();

	}
public:
	void SetPlayers(int nWMP, int nWinLib, int niTune )
	 {
			String^ settingFilPath = UtilFuncs::GetSettingFileFullPath();

			WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_WMP, nWMP.ToString() ,  settingFilPath);
						 
			WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_Win, nWinLib.ToString() ,  settingFilPath);

			WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_iTune, niTune.ToString(),  settingFilPath);
	}

protected:
	/// <summary>
	/// Clean up any resources being used.
	/// </summary>
	~PlayerOptionForm()
	{
	}
private: System::Windows::Forms::CheckBox^  checkBox_WinLib;
protected: 
private: System::Windows::Forms::CheckBox^  checkBox_WMP;
private: System::Windows::Forms::CheckBox^  checkBox_iTune;
private: System::Windows::Forms::Button^  Btn_OK;

private: System::Windows::Forms::Button^  Btn_Cancel;

private:
    Panel^ mPanel;
	
	bool		mWinLibOldChecked;
	bool		mWMPOldChecked;
	bool		miTuneOldCheck;
	bool		mIsSettingChanged;

private:
	/// <summary>
	/// Required designer variable.
	/// </summary>


#pragma region Windows Form Designer generated code
	/// <summary>
	/// Required method for Designer support - do not modify
	/// the contents of this method with the code editor.
	/// </summary>
	void InitializeComponent(void)
	{
		this->checkBox_WinLib = (gcnew System::Windows::Forms::CheckBox());
		this->checkBox_WMP = (gcnew System::Windows::Forms::CheckBox());
		this->checkBox_iTune = (gcnew System::Windows::Forms::CheckBox());
		this->Btn_OK = (gcnew System::Windows::Forms::Button());
		this->Btn_Cancel = (gcnew System::Windows::Forms::Button());
		this->SuspendLayout();
		// 
		// checkBox_WinLib
		// 
		this->checkBox_WinLib->AutoSize = true;
		this->checkBox_WinLib->Location = System::Drawing::Point(22, 13);
		this->checkBox_WinLib->Name = L"checkBox_WinLib";
		this->checkBox_WinLib->Size = System::Drawing::Size(123, 19);
		this->checkBox_WinLib->TabIndex = 0;
		this->checkBox_WinLib->Text = L"Windows Libraries";
		this->checkBox_WinLib->UseVisualStyleBackColor = true;
		// 
		// checkBox_WMP
		// 
		this->checkBox_WMP->AutoSize = true;
		this->checkBox_WMP->Location = System::Drawing::Point(22, 47);
		this->checkBox_WMP->Name = L"checkBox_WMP";
		this->checkBox_WMP->Size = System::Drawing::Size(142, 19);
		this->checkBox_WMP->TabIndex = 1;
		this->checkBox_WMP->Text = L"Windows Media Player";
		this->checkBox_WMP->UseVisualStyleBackColor = true;
		// 
		// checkBox_iTune
		// 
		this->checkBox_iTune->AutoSize = true;
		this->checkBox_iTune->Location = System::Drawing::Point(22, 81);
		this->checkBox_iTune->Name = L"checkBox_iTune";
		this->checkBox_iTune->Size = System::Drawing::Size(56, 19);
		this->checkBox_iTune->TabIndex = 2;
		this->checkBox_iTune->Text = L"iTune";
		this->checkBox_iTune->UseVisualStyleBackColor = true;
		// 
		// Btn_OK
		// 
		this->Btn_OK->Location = System::Drawing::Point(195, 86);
		this->Btn_OK->Name = L"Btn_OK";
		this->Btn_OK->Size = System::Drawing::Size(75, 23);
		this->Btn_OK->TabIndex = 3;
		this->Btn_OK->Text = L"OK";
		this->Btn_OK->UseVisualStyleBackColor = true;
		this->Btn_OK->Click += gcnew System::EventHandler(this, &PlayerOptionForm::Btn_OK_Click);
		// 
		// Btn_Cancel
		// 
		this->Btn_Cancel->Location = System::Drawing::Point(282, 86);
		this->Btn_Cancel->Name = L"Btn_Cancel";
		this->Btn_Cancel->Size = System::Drawing::Size(75, 23);
		this->Btn_Cancel->TabIndex = 4;
		this->Btn_Cancel->Text = L"Cancel";
		this->Btn_Cancel->UseVisualStyleBackColor = true;
		this->Btn_Cancel->Click += gcnew System::EventHandler(this, &PlayerOptionForm::Btn_Cancel_Click);
		// 
		// PlayerOptionForm
		// 
		this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
		this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
		this->BackColor = System::Drawing::SystemColors::Window;
		this->ClientSize = System::Drawing::Size(379, 120);
		this->Controls->Add(this->Btn_Cancel);
		this->Controls->Add(this->Btn_OK);
		this->Controls->Add(this->checkBox_iTune);
		this->Controls->Add(this->checkBox_WMP);
		this->Controls->Add(this->checkBox_WinLib);
		this->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 8.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
			static_cast<System::Byte>(0)));
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->Name = L"PlayerOptionForm";
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
		this->Text = L"Player Optoins";
		this->ResumeLayout(false);

		if(!IsiTuneInstalled())
			checkBox_iTune->Hide();

		this->PerformLayout();
	}
#pragma endregion

private:
	void GetPlayerOption()
	{
		String^ settingFilPath = UtilFuncs::GetSettingFileFullPath();
		
		System::Text::StringBuilder^ buf = gcnew System::Text::StringBuilder(100);

		//First time, set the player options.
		if(!System::IO::File::Exists(settingFilPath))
			mIsSettingChanged = true;

		//Get Windows Library setting.
		GetPrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_Win, L"NONE", buf, 100,  settingFilPath);
		if( buf->ToString()->Equals(L"NONE")  || buf->ToString()->Equals(L"1") )
		{
			checkBox_WinLib->Checked = mWinLibOldChecked = true;
		}

		GetPrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_WMP, L"NONE", buf, 100,  settingFilPath);
		if( buf->ToString()->Equals(L"1") )
		{
			checkBox_WMP->Checked = mWMPOldChecked = true;
		}
		
		GetPrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_iTune, L"NONE", buf, 100,  settingFilPath);
		if( buf->ToString()->Equals(L"1") )
		{
			checkBox_iTune->Checked = miTuneOldCheck = true;
		}
	}
private: System::Void Btn_OK_Click(System::Object^  sender, System::EventArgs^  e) 		 
		 {
			String^ settingFilPath = UtilFuncs::GetSettingFileFullPath();

			if(checkBox_WMP->Checked)
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_WMP, "1" ,  settingFilPath);
			else
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_WMP, "0",  settingFilPath);
							 
			if(checkBox_WinLib->Checked)
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_Win, "1" ,  settingFilPath);
			else
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_Win, "0",  settingFilPath);

			if(checkBox_iTune->Checked)
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_iTune, "1" ,  settingFilPath);
			else
				WritePrivateProfileString(INI_SEC_PlayerSetting, INI_KEY_iTune, "0",  settingFilPath);


			//Player setting is launched first time, and it is set true,
			//If it is true, it does not set again.
			if(!mIsSettingChanged)
				mIsSettingChanged = !(mWinLibOldChecked == checkBox_WinLib->Checked) || 
													!(mWMPOldChecked == checkBox_WMP->Checked) ||
													!(miTuneOldCheck == checkBox_iTune->Checked);

			this->Close();
		 }

private: System::Void Btn_Cancel_Click(System::Object^  sender, System::EventArgs^  e) 
		 {
			 //If cancel is clicked, the setting is abort.
			mIsSettingChanged = false;
			 this->Close();
		 }
		 public: property bool IsSettingChanged
		{
				bool get()
				{
					return mIsSettingChanged;
				}
		 }

		 private: bool IsiTuneInstalled()
				  {
					  Microsoft::Win32::RegistryKey ^ rk = Microsoft::Win32::Registry::CurrentUser;
					  Microsoft::Win32::RegistryKey^ iTuneKey = rk->OpenSubKey("Software\\Apple Computer, Inc.\\iTunes", false);
					  rk->Close();
						if (iTuneKey == nullptr)
							return false;
						else 
						{

							iTuneKey->Close();
							return true;
						}
				  
				  }
};
