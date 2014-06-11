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
#include "progress_dialog.h"

ProgressDialog::ProgressDialog()
{
    mLabel = gcnew Label();
    mLabel->Location = System::Drawing::Point(18, 8);
    mLabel->Size = System::Drawing::Size(160, 23);
    mLabel->TabIndex = 3;
    mLabel->Text = L"";

    mProgressBar = gcnew ProgressBar();
    mProgressBar->Location = System::Drawing::Point(18, 32);
    mProgressBar->Size = System::Drawing::Size(256, 8);
    mProgressBar->Step = 2;
    mProgressBar->TabIndex = 4;
    mProgressBar->Minimum = 0;
    mProgressBar->Maximum = 100;
	this->TopMost = true;

    // form

#pragma push_macro("ExtractAssociatedIcon")	//disable the definition of ExtractAssociatedIcon in ShellApi.
#undef ExtractAssociatedIcon

    this->Icon = System::Drawing::Icon::ExtractAssociatedIcon(Application::ExecutablePath);	

#pragma pop_macro("ExtractAssociatedIcon")

    this->SuspendLayout();
    this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
    this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
    this->BackColor = System::Drawing::SystemColors::Window;
    this->ClientSize = System::Drawing::Size(292, 48);
    this->Font = (gcnew System::Drawing::Font(L"Lucida Sans Unicode", 8.25F,
        System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
    this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
    this->MaximizeBox = false;
    this->MinimizeBox = false;
    this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
    this->Text = L"";
    this->Closing += gcnew System::ComponentModel::CancelEventHandler(this, &ProgressDialog::OnClose);
    this->ResumeLayout(false);
    this->Controls->Add(mLabel);
    this->Controls->Add(mProgressBar);
}

ProgressDialog::~ProgressDialog()
{
    // do nothing, memory is managed
}

void ProgressDialog::ProgressLabel::set(String^ value)
{
    mLabel->Text = value;
}

void ProgressDialog::ProgressValue::set(int value)
{
    mProgressBar->Value = value;
}

void ProgressDialog::OnClose(Object^ sender, System::ComponentModel::CancelEventArgs^ e)
{
    this->Hide();
    e->Cancel = true;
}
