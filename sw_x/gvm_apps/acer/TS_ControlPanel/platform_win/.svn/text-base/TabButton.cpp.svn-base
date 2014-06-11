#include "StdAfx.h"
#include "TabButton.h"
using namespace System::Windows::Forms;


CTabButton::CTabButton(void)
{
	mBtn = gcnew System::Windows::Forms::Label();//Button();
	mPicL = gcnew PictureBox();
	mPicR = gcnew PictureBox();

	//mBtn->Click += gcnew System::EventHandler(this, &Form1::button1_Click);

	this->BackColor = System::Drawing::Color::Transparent;

	// 
	// mBtn
	// 	
	mBtn->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mBtn->Dock = System::Windows::Forms::DockStyle::Left;
//	mBtn->FlatAppearance->BorderSize = 0;
	//mBtn->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
	mBtn->Location = System::Drawing::Point(1, 0);
	mBtn->Margin = System::Windows::Forms::Padding(6, 0, 0, 12);
	mBtn->Name = L"mBtn";
	mBtn->Size = System::Drawing::Size(144, 33);
	mBtn->TabIndex = 3;
	mBtn->Text = L"mBtn";
	mBtn->TextAlign  = System::Drawing::ContentAlignment::MiddleCenter;
//	mBtn->UseVisualStyleBackColor = true;

	mBtn->Font = (gcnew System::Drawing::Font(L"Arial", 11, System::Drawing::FontStyle::Regular));
	mBtn->ForeColor = System::Drawing::Color::FromArgb(255, 90,90,90);

	//mPicL
	
	mPicL->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mPicL->Dock = System::Windows::Forms::DockStyle::Left;	
	mPicL->Name = L"mPicL";
	mPicL->Size = System::Drawing::Size(6, 33);
	mPicL->TabIndex = 4;
	mPicL->TabStop = false;

	//mPicR
	mPicR->BackColor = System::Drawing::Color::Transparent;
	mPicR->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
	mPicR->Dock = System::Windows::Forms::DockStyle::Right;	
	mPicR->Margin = System::Windows::Forms::Padding(0);
	mPicR->Name = L"mPicR";
	mPicR->Size = System::Drawing::Size(6, 33);
	mPicR->TabIndex = 5;
	mPicR->TabStop = false;
	// 
	// panel1
	// 
	this->Controls->Add(this->mPicR);
	this->Controls->Add(this->mBtn);
	this->Controls->Add(mPicL);	
	this->Margin = System::Windows::Forms::Padding(0);
	this->Name = L"panel1";
	this->Size = System::Drawing::Size(146, 33);
	this->TabIndex = 6;

	this->SizeChanged += gcnew System::EventHandler( this, &CTabButton::MySizeChanged );

	mBtn->Click += gcnew System::EventHandler(this, &CTabButton::MyMouseClicked);

	//mBtn->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &CTabButton::MyMouseDown);
	//mBtn->MouseEnter += gcnew System::EventHandler(this, &CTabButton::MyMouseEnter);
	//mBtn->MouseHover += gcnew System::EventHandler(this, &CTabButton::MyMouseEnter);
	//mBtn->MouseLeave += gcnew System::EventHandler(this, &CTabButton::MyMouseLeave);
	//mBtn->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &CTabButton::MyMouseUp);

	Unselected();
}

void CTabButton::MySizeChanged( System::Object^ sender, System::EventArgs^ e )
{
	this->mBtn->Size = System::Drawing::Size(this->Size.Width -12, this->Size.Height);
	mPicR->Size = System::Drawing::Size(6,  this->Size.Height);
	mPicL->Size = System::Drawing::Size(6,  this->Size.Height);
}


//void CTabButton::MyMouseLeave(System::Object^  sender, System::EventArgs^  e) 
//{
//	ButtonIsNormal();
//
//}
//
//void CTabButton::MyMouseDown( System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e )
//{
//	ButtonIsPressed();
//}
//
//void  CTabButton::MyMouseEnter(System::Object^  sender, System::EventArgs^  e) 
//{
//	ButtonIsHovered();
//};
//
//
//void CTabButton::MyMouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) 
//{
//	ButtonIsNormal();
//}

void CTabButton::Selected()
{
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_selected_R.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_selected_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_selected_bg.png");

	mBtn->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
}

void CTabButton::Unselected()
{
	mPicR->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_unselected_R.png");
	mPicL->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_unselected_l.png");
	mBtn->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\tab_unselected_bg.png");

	mBtn->ForeColor = System::Drawing::Color::FromArgb(255, 150, 150, 150);

}

//Button^ CTabButton::GetBtn::get()
//{
//	return mBtn;
//}


void CTabButton::MyMouseClicked(System::Object^  sender, System::EventArgs^  e) 
{
	if(OnClickedEvent != nullptr)
	{
		OnClickedEvent();
	}
}

System::String^ CTabButton::Text::get()
{
	return mBtn->Text;
}

void CTabButton::Text::set(System::String^ value)
{
		mBtn->Text = value ;
}
