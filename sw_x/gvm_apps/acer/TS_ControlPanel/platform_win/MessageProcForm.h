#pragma once

#include <windows.h>
#include "WM_USER.h"
//#include "settings_form.cpp"

#include "ccd_manager.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace acpanel_win {

	/// <summary>
	/// Summary for MessageProcForm
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class MessageProcForm : public System::Windows::Forms::Form
	{
	public:
		MessageProcForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			dlOpenAcerTSEvent = nullptr;
		}
		void SetForms(System::Windows::Forms::Form^ form)
		{
			mSetupForm = form;
		}

		delegate void dlOpenAcerTS();
		dlOpenAcerTS^ dlOpenAcerTSEvent;

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MessageProcForm()
		{
			if (components)
			{
				delete components;
			}
		}		
	private: System::Windows::Forms::Button^  Btn_ShowSetupForm;
	private: System::Windows::Forms::Button^  Btn_HideSetupForm;

	protected: 

		System::Windows::Forms::Form^ mSetupForm;
		System::Collections::Generic::List<Int32>^ mDestHandles; 

		virtual void WndProc( Message% m ) override
		{
			
			switch ( m.Msg )
			{
				case WM_LAUNCH:
					{
						if(dlOpenAcerTSEvent != nullptr)
							dlOpenAcerTSEvent();
					}
				break;
				case WM_LOGIN:
				case WM_PSN:
					System::Diagnostics::Debug::WriteLine(L"SetupForm, handle: "+m.WParam+", msg: " + m.Msg);

					int nResult = 0;
					if( (m.Msg == WM_LOGIN && CcdManager::Instance->IsLoggedIn) ||
						(m.Msg == WM_PSN && CcdManager::Instance->IsLogInPSN) )
					{	
							nResult = 1;
					}

					if(m.WParam.ToInt32() !=0)
					{
						if(mDestHandles == nullptr)
						{
							mDestHandles = gcnew System::Collections::Generic::List<Int32>();
						}
						bool bFound = false;
						for(int i=0; i <mDestHandles->Count ; i++)
						{
							if(mDestHandles[i] == m.WParam.ToInt32())
								bFound = true;
						}
						if(!bFound)
							mDestHandles->Add(m.WParam.ToInt32());
					}

					for(int i=0; i <mDestHandles->Count ; i++)
					{
						HWND DestHandle =(HWND) mDestHandles[i];
						PostMessage(DestHandle, m.Msg, nResult, 0);
					}

					break;
			}
			Form::WndProc( m );
		}

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->Btn_ShowSetupForm = (gcnew System::Windows::Forms::Button());
			this->Btn_HideSetupForm = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// Btn_ShowSetupForm
			// 
			this->Btn_ShowSetupForm->Location = System::Drawing::Point(69, 12);
			this->Btn_ShowSetupForm->Name = L"Btn_ShowSetupForm";
			this->Btn_ShowSetupForm->Size = System::Drawing::Size(116, 23);
			this->Btn_ShowSetupForm->TabIndex = 0;
			this->Btn_ShowSetupForm->Text = L"Show SetupForm";
			this->Btn_ShowSetupForm->UseVisualStyleBackColor = true;
			this->Btn_ShowSetupForm->Click += gcnew System::EventHandler(this, &MessageProcForm::Btn_ShowSetupForm_Click);
			// 
			// Btn_HideSetupForm
			// 
			this->Btn_HideSetupForm->Location = System::Drawing::Point(69, 53);
			this->Btn_HideSetupForm->Name = L"Btn_HideSetupForm";
			this->Btn_HideSetupForm->Size = System::Drawing::Size(116, 23);
			this->Btn_HideSetupForm->TabIndex = 1;
			this->Btn_HideSetupForm->Text = L"Hide SetupForm";
			this->Btn_HideSetupForm->UseVisualStyleBackColor = true;
			this->Btn_HideSetupForm->Click += gcnew System::EventHandler(this, &MessageProcForm::Btn_HideSetupForm_Click);
			// 
			// MessageProcForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(256, 160);
			this->Controls->Add(this->Btn_HideSetupForm);
			this->Controls->Add(this->Btn_ShowSetupForm);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
			this->Location = System::Drawing::Point(-10000, -10000);
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"MessageProcForm";
			this->ShowIcon = false;
			this->ShowInTaskbar = false;
			this->SizeGripStyle = System::Windows::Forms::SizeGripStyle::Hide;
			this->Text = L"Acer TS MsgProc ";
			this->WindowState = System::Windows::Forms::FormWindowState::Minimized;
			this->ResumeLayout(false);

		}
#pragma endregion
	private: System::Void Btn_ShowSetupForm_Click(System::Object^  sender, System::EventArgs^  e) 
			 {
				if(mSetupForm != nullptr)
				{
					if(!mSetupForm->Visible)
						mSetupForm->Visible = true;
				}
			 }
	private: System::Void Btn_HideSetupForm_Click(System::Object^  sender, System::EventArgs^  e) 
			 {
				 if(mSetupForm != nullptr)
				{
					if(mSetupForm->Visible)
						mSetupForm->Visible = false;
				}
			 }
};
}
