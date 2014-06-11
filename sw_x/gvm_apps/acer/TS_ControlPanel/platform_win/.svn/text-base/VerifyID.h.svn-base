#pragma once

ref class CVerifyID :
public System::Windows::Forms::Panel
{
public:
	CVerifyID(void);
	void SetEmail(System::String^ email){if(email!= nullptr) mEmailLabel->Text = email;};
	private: System::Windows::Forms::Label^  mValidCodeLabel;
	private: System::Windows::Forms::Label^  mEmailLabel;
	private: System::Windows::Forms::Label^  mDescription;
	private: System::Windows::Forms::Label^  mTitle;
	private: System::Windows::Forms::TextBox^  mValidCodeTextBox;
};
