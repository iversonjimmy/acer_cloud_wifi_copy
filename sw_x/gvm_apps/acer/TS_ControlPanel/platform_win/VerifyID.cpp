#include "StdAfx.h"
#include "VerifyID.h"

CVerifyID::CVerifyID(void)
{
			this->mValidCodeLabel = (gcnew System::Windows::Forms::Label());
			this->mDescription = (gcnew System::Windows::Forms::Label());
			this->mTitle = (gcnew System::Windows::Forms::Label());
			this->mEmailLabel = (gcnew System::Windows::Forms::Label());
			this->mValidCodeTextBox = (gcnew System::Windows::Forms::TextBox());

			this->BackColor = System::Drawing::Color::FromArgb(255, 245, 245, 245); 
			this->Controls->Add(this->mValidCodeTextBox);
			this->Controls->Add(this->mValidCodeLabel);
			this->Controls->Add(this->mEmailLabel);
			this->Controls->Add(this->mDescription);
			this->Controls->Add(this->mTitle);
			this->Name = L"panel1";
			this->Size = System::Drawing::Size(728, 424);
			this->Location = System::Drawing::Point(14, 67);
			this->TabIndex = 4;

			// 
			// mValidCodeLabel
			// 
			this->mValidCodeLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mValidCodeLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mValidCodeLabel->Location = System::Drawing::Point(27, 128);
			this->mValidCodeLabel->Name = L"mValidCodeLabel";
			this->mValidCodeLabel->Size = System::Drawing::Size(599, 47);
			this->mValidCodeLabel->TabIndex = 1;
			this->mValidCodeLabel->Text = L"Enter the validation code into the following text field and press Next to continu" 
				L"e.";
			// 
			// mDescription
			// 
			this->mDescription->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mDescription->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mDescription->Location = System::Drawing::Point(27, 51);
			this->mDescription->Name = L"mDescription";
			this->mDescription->Size = System::Drawing::Size(618, 38);
			this->mDescription->TabIndex = 1;
			this->mDescription->Text = L"We\'ve sent a validation code to the following email address:";
			// 
			// mTitle
			// 
			this->mTitle->Font = (gcnew System::Drawing::Font(L"Arial", 18));
			this->mTitle->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mTitle->Location = System::Drawing::Point(25, 18);
			this->mTitle->Name = L"mTitle";
			this->mTitle->Size = System::Drawing::Size(282, 33);
			this->mTitle->TabIndex = 1;
			this->mTitle->Text = L"Verify acer ID";
			// 
			// mEmailLabel
			// 
			this->mEmailLabel->Font = (gcnew System::Drawing::Font(L"Arial", 11));
			this->mEmailLabel->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mEmailLabel->Location = System::Drawing::Point(27, 85);
			this->mEmailLabel->Size = System::Drawing::Size(507, 29);
			this->mEmailLabel->Name = L"mEmailLabel";
			this->mEmailLabel->TabIndex = 1;
			this->mEmailLabel->Text = L"%emailAddress%";
			this->mEmailLabel->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;

			// 
			// mValidCodeTextBox
			// 
			this->mValidCodeTextBox->Font = (gcnew System::Drawing::Font(L"Arial", 12.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(0)));
			this->mValidCodeTextBox->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), 
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)));
			this->mValidCodeTextBox->Location = System::Drawing::Point(30, 175);
			this->mValidCodeTextBox->Name = L"mValidCodeTextBox";
			this->mValidCodeTextBox->Size = System::Drawing::Size(355, 27);
			this->mValidCodeTextBox->TabIndex = 3;
}
