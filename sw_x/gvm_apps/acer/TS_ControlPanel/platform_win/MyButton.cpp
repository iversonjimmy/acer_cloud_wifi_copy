#include "StdAfx.h"
#include "MyButton.h"
using namespace System::Windows::Forms;



CMyButton::CMyButton(void)
{
	Init(false);
}
CMyButton::CMyButton(bool bUnlinkBtn)
{
	Init(bUnlinkBtn);
}

void CMyButton::Init(bool bShowUnlinkBtn)
{
	mbUnlink = bShowUnlinkBtn;

	//mBtn = gcnew System::Windows::Forms::Button();
	mBtn = gcnew System::Windows::Forms::Label();
	mPicL = gcnew PictureBox();
	mPicR = gcnew PictureBox();
	

	SuspendLayout();
	//mBtn->Click += gcnew System::EventHandler(this, &Form1::button1_Click);

	this->BackColor = System::Drawing::Color::FromArgb(255, 245, 245, 245);// System::Drawing::Color::Transparent;

	// 
	// mBtn
	// 
	//this->mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_n_bg.png");
	mBtn->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mBtn->Dock = System::Windows::Forms::DockStyle::Left;
	//mBtn->FlatAppearance->BorderSize = 0;
	mBtn->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
	mBtn->Location = System::Drawing::Point(1, 0);
	mBtn->Margin = System::Windows::Forms::Padding(1, 0, 0, 1);
	mBtn->Name = L"mBtn";
	mBtn->Size = System::Drawing::Size(100, 32);
	mBtn->TabIndex = 3;
	mBtn->Text = L"      1234567890";
	mBtn->TextAlign  = System::Drawing::ContentAlignment::MiddleCenter;

	mBtn->Font = (gcnew System::Drawing::Font(L"Arial", 11, System::Drawing::FontStyle::Regular));
	mBtn->ForeColor = System::Drawing::Color::FromArgb(255, 90,90,90);

	if(mbUnlink)
	{
		mBtn->Text = L"      1234567890";
		mBtn->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
	}
	else
	{
		mBtn->Text = L"1234567890";
		mBtn->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
	}

//	mBtn->UseVisualStyleBackColor = true;

	//mPicL
	mPicL->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mPicL->Dock = System::Windows::Forms::DockStyle::Left;
	mPicL->Name = L"mPicL";
	mPicL->Size = System::Drawing::Size(5, 32);
	mPicL->TabIndex = 4;
	mPicL->TabStop = false;

	//mPicR
	mPicR->BackColor = System::Drawing::Color::Transparent;
	mPicR->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mPicR->Dock = System::Windows::Forms::DockStyle::Right;
	mPicR->Margin = System::Windows::Forms::Padding(0);
	mPicR->Name = L"mPicR";
	mPicR->Size = System::Drawing::Size(5, 32);
	mPicR->TabIndex = 5;
	mPicR->TabStop = false;

	
	this->Controls->Add(mPicR);
	//mPicUnlink
	if(mbUnlink)
	{
		mPicUnlink = gcnew PictureBox();
		mPicUnlink->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
		mPicUnlink->Margin = System::Windows::Forms::Padding(0);
		mPicUnlink->Name = L"mPicUnlink";
		mPicUnlink->Size = System::Drawing::Size(27, 29);
		mPicUnlink->TabIndex = 5;
		mPicUnlink->TabStop = false;
		mPicUnlink->Location = System::Drawing::Point(mPicL->Size.Width, 0);

		mPicUnlink->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &CMyButton::MyMouseDown);
		mPicUnlink->MouseEnter += gcnew System::EventHandler(this, &CMyButton::MyMouseEnter);
		mPicUnlink->MouseHover += gcnew System::EventHandler(this, &CMyButton::MyMouseEnter);
		mPicUnlink->MouseLeave += gcnew System::EventHandler(this, &CMyButton::MyMouseLeave);
		mPicUnlink->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &CMyButton::MyMouseUp);
		mPicUnlink->Click += gcnew System::EventHandler(this, &CMyButton::MyMouseClicked);
	}
	this->Controls->Add(mPicUnlink);
	this->Controls->Add(mBtn);
	this->Controls->Add(mPicL);

	this->Location = System::Drawing::Point(122, 175);
	this->Margin = System::Windows::Forms::Padding(0);
	this->Name = L"panel1";
	this->Size = System::Drawing::Size(102, 32);
	this->TabIndex = 6;

	this->SizeChanged += gcnew System::EventHandler( this, &CMyButton::MySizeChanged );
	this->EnabledChanged += gcnew System::EventHandler( this, &CMyButton::MyEnabledChanged );
	mBtn->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &CMyButton::MyMouseDown);
	mBtn->MouseEnter += gcnew System::EventHandler(this, &CMyButton::MyMouseEnter);
	mBtn->MouseHover += gcnew System::EventHandler(this, &CMyButton::MyMouseEnter);
	mBtn->MouseLeave += gcnew System::EventHandler(this, &CMyButton::MyMouseLeave);
	mBtn->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &CMyButton::MyMouseUp);
	mBtn->Click += gcnew System::EventHandler(this, &CMyButton::MyMouseClicked);

	ResumeLayout(false);
	ButtonIsNormal();
}

void CMyButton::MyMouseClicked(System::Object^  sender, System::EventArgs^  e) 
{
	if(OnClickedEvent != nullptr)
	{
		OnClickedEvent();
	}
}
	
void CMyButton::MyEnabledChanged( System::Object^ sender, System::EventArgs^ e )
{
	if(this->Enabled)
		ButtonIsNormal();
	else 
		ButtonIsDisabled();
}

void CMyButton::MySizeChanged( System::Object^ sender, System::EventArgs^ e )
{
	this->mBtn->Size = System::Drawing::Size(this->Size.Width -10, this->Size.Height);
	mPicR->Size = System::Drawing::Size(5,  this->Size.Height);
	mPicL->Size = System::Drawing::Size(5,  this->Size.Height);
	if(mbUnlink)
		mPicUnlink->Size = System::Drawing::Size(27,  this->Size.Height);
}


void CMyButton::MyMouseLeave(System::Object^  sender, System::EventArgs^  e) 
{
	ButtonIsNormal();

}

void CMyButton::MyMouseDown( System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e )
{
	ButtonIsPressed();
}

void  CMyButton::MyMouseEnter(System::Object^  sender, System::EventArgs^  e) 
{
	ButtonIsHovered();
};


void CMyButton::MyMouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) 
{
	ButtonIsNormal();
}

void CMyButton::ButtonIsHovered()
{
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_h_r.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_h_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_h_bg.png");
	if(mbUnlink)
	{
		mPicUnlink->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_h_bg.png");
		mPicUnlink->Image = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_link_n.png");
	}

}

void CMyButton::ButtonIsPressed()
{
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_p_r.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_p_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_p_bg.png");
	if(mbUnlink)
	{
		mPicUnlink->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_p_bg.png");
		mPicUnlink->Image = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_link_n.png");
	}

}

void CMyButton::ButtonIsNormal()
{
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_n_r.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_n_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_n_bg.png");
	if(mbUnlink)
	{
		mPicUnlink->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_n_bg.png");
		mPicUnlink->Image = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_link_n.png");
	}
}

void CMyButton::ButtonIsDisabled()
{	
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_d_r.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_d_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_d_bg.png");
	if(mbUnlink)
	{
		mPicUnlink->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\SignUp\\btn_d_bg.png");
		mPicUnlink->Image = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_link_d.png");
	}
}
//
//CMyButton^ CMyButton::GetBtn::get()
//{
//	return mBtn;
//}

//System::String^ CMyButton::MyText::get()
//{
//	return mBtn->Text;
//}
//void CMyButton::MyText::set(System::String^ value)
//{
//	if(mbUnlink)
//		mBtn->Text = L"      "+value ;
//	else
//		mBtn->Text = value ;
//
//}

System::String^ CMyButton::Text::get()
{
	return mBtn->Text;
}
void CMyButton::Text::set(System::String^ value)
{
	if(mbUnlink)
		mBtn->Text = L"      "+value ;
	else
		mBtn->Text = value ;

}
