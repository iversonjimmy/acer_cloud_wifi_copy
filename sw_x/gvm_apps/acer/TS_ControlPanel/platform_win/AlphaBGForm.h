#pragma once


#include <windows.h>
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Windows;

#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "User32.lib")

namespace acpanel_win {
/// <summary>
	/// Summary for AlphaBGForm
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class AlphaBGForm : public System::Windows::Forms::Form
	{

	public:

	protected:
		bool m_bMouseDown;
		Point mouseDownLocation;

	protected: 
		System::Windows::Forms::Form^ mControlledWindow;
	private:


		delegate void dlButtonOK();
		event dlButtonOK^ dlButtonOKEvent;


	public:
		AlphaBGForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			long dwStyle = GetWindowLong((HWND)this->Handle.ToInt32(), GWL_EXSTYLE);

			dwStyle |= WS_EX_LAYERED;
			SetWindowLong((HWND)this->Handle.ToInt32(), GWL_EXSTYLE, dwStyle);

			String^ szPath = System::IO::Path::GetDirectoryName(System::Reflection::Assembly::GetExecutingAssembly()->Location);
			
			//szPath += L"\\images\\SignUp\\box_bg.png";
			szPath = L".\\images\\SignUp\\box_bg.png";
			
			SetBitmap(szPath);
			
			System::Drawing::Rectangle rc = System::Windows::Forms::SystemInformation::WorkingArea;
			this->Top = (rc.Height - this->Height) / 2;
			this->Left = (rc.Width - this->Width) / 2;

			Invalidate(rc);
		}

		void SetControlledWindow(System::Windows::Forms::Form^ controlledForm)
		{
			mControlledWindow = controlledForm;

			DWORD dwStyle = GetWindowLong((HWND)mControlledWindow->Handle.ToInt32(), GWL_STYLE);

			dwStyle ^= WS_POPUP;
			dwStyle |= WS_CHILD;
			SetWindowLong((HWND)mControlledWindow->Handle.ToInt32(), GWL_STYLE, dwStyle);

			//mControlledWindow->dlButtonOKEvent += gcnew DialogBoxControl::dlButtonOK(this, &MyDialogBox::OnButtonOK);
			mControlledWindow->Left = this->Left + (this->Width - mControlledWindow->Width)/2;
			mControlledWindow->Top = this->Top + (this->Height - mControlledWindow->Height)/2;
			//if(mControlledWindow->Visible)
			//{
			//	mControlledWindow->Hide();
			//	
			//}
			//bool visible = mControlledWindow->Visible;

			//mControlledWindow->Show(this);
			//mControlledWindow->BringToFront();

			mControlledWindow->Owner = this;
			mControlledWindow->VisibleChanged+=  gcnew System::EventHandler(this, &AlphaBGForm::controlWinVisibleChanged);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~AlphaBGForm()
		{
			if (components)
			{
				delete components;
			}
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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(AlphaBGForm::typeid));
			this->SuspendLayout();
			// 
			// AlphaBGForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(758, 558);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->Name = L"AlphaBGForm";
			this->RightToLeftLayout = true;
			this->ShowInTaskbar = false;
			this->Text = L"AlphaBGForm ";
			this->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &AlphaBGForm::panel1_MouseUp);
			this->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &AlphaBGForm::panel1_MouseDown);
			this->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &AlphaBGForm::panel1_MouseMove);
			this->ResumeLayout(false);

		}

		//void ShowDialogBox(String^ szMsg, String^ szTitle, IconType type);

		void SetBitmap(String^ szPath)
		{
			Bitmap bitmap = gcnew Bitmap(szPath);
			HDC ScreenDC = GetDC(0);
			HDC memDc = CreateCompatibleDC(ScreenDC);
			HBITMAP hBitmap;
			HBITMAP oldBitmap;

			try 
			{
				hBitmap = (HBITMAP)bitmap.GetHbitmap(Color::FromArgb(0)).ToInt32();
				oldBitmap = (HBITMAP)SelectObject(memDc, hBitmap);

				SIZE size = {bitmap.Width, bitmap.Height};
				POINT pointSource = {0, 0};
				POINT topPos = {Left, Top};
				BLENDFUNCTION blend;

				blend.BlendOp = AC_SRC_OVER;
				blend.BlendFlags = 0;
				blend.SourceConstantAlpha = 255;
				blend.AlphaFormat = AC_SRC_ALPHA;

				UpdateLayeredWindow((HWND)this->Handle.ToInt32(), ScreenDC, &topPos, &size, memDc, &pointSource, 0,  &blend, ULW_ALPHA);
			}
			finally
			{
				ReleaseDC(NULL, ScreenDC);
				SelectObject(memDc, oldBitmap);
				DeleteObject(hBitmap);
				DeleteDC(memDc);
			}	

		};
		
		//void StartLabel(String^ szMsg);
		//void OnButtonOK();

	protected:
		//virtual property System::Windows::Forms::CreateParams^ CreateParams
		//{
		//	System::Windows::Forms::CreateParams^ get() override
		//	{
		//	   System::Windows::Forms::CreateParams^ overrideParams = Form::CreateParams;
		//	   // Make your changes to overrideParams members here
		//	   overrideParams->ExStyle |= 0x00080000;
		//	   return overrideParams;
		//	}
		//}


		void panel1_MouseDown( Object^ sender, System::Windows::Forms::MouseEventArgs^ e )
		{
			// Update the mouse path with the mouse information
			mouseDownLocation = Point(e->X,e->Y);
         
			 switch ( e->Button )
			 {
				case ::MouseButtons::Left:
				   m_bMouseDown = true;
				   break;

				this->Focus();
				this->Invalidate();
			}
		}

	
		void panel1_MouseUp( Object^ sender, System::Windows::Forms::MouseEventArgs^ e )
		{
			if(mControlledWindow != nullptr)
				mControlledWindow->BringToFront();
			
			m_bMouseDown = false;
		}

		void panel1_MouseMove( Object^ sender, System::Windows::Forms::MouseEventArgs^ e )
		{
			if (!m_bMouseDown)
				return;
			// Update the mouse path that is drawn onto the Panel.
			int mouseX = e->X;
			int mouseY = e->Y;

			int cX = mouseX - mouseDownLocation.X;
			int cY = mouseY - mouseDownLocation.Y;

			this->Left +=cX;
			this->Top +=cY;
			if(mControlledWindow != nullptr)
			{
				mControlledWindow->Left = this->Left + (this->Width - mControlledWindow->Width)/2;
				mControlledWindow->Top = this->Top + (this->Height - mControlledWindow->Height)/2;
			}
		}

		private: System::Void mClosePictureBox_Click(System::Object^  sender, System::EventArgs^  e) 
			{
				mControlledWindow->Close();
				Close();
			 }
#pragma endregion

		 private: System::Void controlWinVisibleChanged(System::Object^  sender, System::EventArgs^  e) 
		 {
			 System::Windows::Forms::Form^ form = (System::Windows::Forms::Form^) (sender);
			 if(form->Visible == false)
				this->Visible = false;
			 else
			 {
				this->Visible=true;
				//mControlledWindow->Visible = true;
				//this->Show(mControlledWindow);
			 }
		 }
	};
}
