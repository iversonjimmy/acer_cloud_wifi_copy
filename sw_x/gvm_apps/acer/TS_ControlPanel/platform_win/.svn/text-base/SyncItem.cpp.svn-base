#include "StdAfx.h"
#include "SyncItem.h"

CSyncItem::CSyncItem(void)
{
	this->pictureBox_Line = (gcnew System::Windows::Forms::PictureBox());
	this->mCheckBox = (gcnew System::Windows::Forms::PictureBox());
	this->mPic = (gcnew System::Windows::Forms::PictureBox());
	this->mLabel = (gcnew System::Windows::Forms::Label());
	this->mTouchPadPanel = (gcnew System::Windows::Forms::Label());
	this->mButton = (gcnew CMyButton());

	this->BackColor = System::Drawing::Color::Transparent;
	this->Controls->Add(this->mButton);
	this->Controls->Add(this->mCheckBox);
	this->Controls->Add(this->mLabel);
	this->Controls->Add(this->mPic);
	this->Controls->Add(this->pictureBox_Line);
	this->Controls->Add(this->mTouchPadPanel);	
	this->Size = System::Drawing::Size(688, 52);
	this->EnabledChanged += gcnew System::EventHandler(this, &CSyncItem::EnabledIsChanged);

	// 
	// pictureBox_Line
	// 
	this->pictureBox_Line->BackColor = System::Drawing::Color::FromArgb(255,178,178,178);
	this->pictureBox_Line->Location = System::Drawing::Point(3, 5);
	this->pictureBox_Line->Name = L"pictureBox_Line";
	this->pictureBox_Line->Size = System::Drawing::Size(690, 1);
	this->pictureBox_Line->TabIndex = 2;
	this->pictureBox_Line->TabStop = false;
	// 
	// mCheckBox
	// 
	this->mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_n.png");
	this->mCheckBox->Location = System::Drawing::Point(11, 23);
	this->mCheckBox->Name = L"mCheckBox";
	this->mCheckBox->Size = System::Drawing::Size(14, 14);
	this->mCheckBox->TabIndex = 3;
	this->mCheckBox->TabStop = false;
	this->mCheckBox->MouseLeave += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseLeave);
	this->mCheckBox->Click += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_Click);
	this->mCheckBox->MouseHover += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseHover);

	// 
	// mPic
	// 
	this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_contacts_32_.png");
	this->mPic->Location = System::Drawing::Point(40, 16);
	this->mPic->Name = L"mPic";
	this->mPic->Size = System::Drawing::Size(32, 32);
	this->mPic->TabIndex = 4;
	this->mPic->TabStop = false;
	this->mPic->MouseLeave += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseLeave);
	this->mPic->Click += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_Click);
	this->mPic->MouseHover += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseHover);

	// 
	// mLabel
	// 
	this->mLabel->AutoSize = true;
	this->mLabel->Font = (gcnew System::Drawing::Font(L"Arial", 12));
	this->mLabel->ForeColor = System::Drawing::Color::FromArgb(255,90,90,90);
	this->mLabel->Location = System::Drawing::Point(89, 23);
	this->mLabel->Name = L"mLabel";
	this->mLabel->Size = System::Drawing::Size(105, 18);
	this->mLabel->TabIndex = 5;
	this->mLabel->Text = L"Contacts sync";



	// 
	// mTouchPadPanel
	// 
	this->mTouchPadPanel->Location = System::Drawing::Point(6, 9);
	this->mTouchPadPanel->Name = L"mTouchPadPanel";
	this->mTouchPadPanel->Size = System::Drawing::Size(80, 34);
	this->mTouchPadPanel->TabIndex = 6;
	this->mTouchPadPanel->MouseLeave += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseLeave);
	this->mTouchPadPanel->Click += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_Click);
	this->mTouchPadPanel->MouseHover += gcnew System::EventHandler(this, &CSyncItem::mTouchPadPanel_MouseHover);
	// 
	// mButton
	// 
	this->mButton->Font = (gcnew System::Drawing::Font(L"Arial", 11.25F));
	this->mButton->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(90)), 
		static_cast<System::Int32>(static_cast<System::Byte>(90)));
	this->mButton->Location = System::Drawing::Point(550, 19);
	this->mButton->Name = L"mButton";
	this->mButton->Size = System::Drawing::Size(93, 30);
	this->mButton->TabIndex = 7;
	this->mButton->Text = L"button1";
	//this->mButton->UseVisualStyleBackColor = true;

}


void CSyncItem::mTouchPadPanel_MouseHover(System::Object^  sender, System::EventArgs^  e) 
{
	if(ImageStatus != StatusType::Checked)
		ImageStatus = StatusType::Hover;
}

void CSyncItem::mTouchPadPanel_MouseLeave(System::Object^  sender, System::EventArgs^  e) 
{
	if(ImageStatus != StatusType::Checked)
		ImageStatus = StatusType::Normal;
}

void CSyncItem::EnabledIsChanged(System::Object^  sender, System::EventArgs^  e) 
{
	if(ImageStatus != StatusType::Disabled)
		ImageStatus= StatusType::Disabled;
	else
		ImageStatus= StatusType::Normal;
}


void CSyncItem::mTouchPadPanel_Click(System::Object^  sender, System::EventArgs^  e) 
{	
	if(mStatus == StatusType::Checked)
	{
		ImageStatus = StatusType::Normal;
	}
	else
	{
		ImageStatus = StatusType::Checked;
	}
}


void CSyncItem::ImageStatus::set(StatusType value)
{		
	switch(value)
	{
		case StatusType::Checked:
			mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_p.png");
		break;
		case StatusType::Disabled:
			mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_d.png");
		break;
		case StatusType::Focus:
			mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_f.png");
		break;
		case StatusType::Hover:
			mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_h.png");
		break;
		case StatusType::Normal:
			mCheckBox->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\btn_checkbox_n.png");
		break;
	}

	mStatus = value;
	
	//不要在這裡改變treenode的內容, 會有exception.
	//if(mNode != nullptr)
	//{
	//	if(value == StatusType::Checked)
	//		mNode->Checked = true;
	//	else
	//		mNode->Checked = false;
	//}


}

void CSyncItem::ChangeIconType::set(IconType value)
{
	switch(value)
	{
		case IconType::Contacts:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_contacts_32_.png");
		break;
		case IconType::Browswer:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_browser_32_.png");
		break;
		case IconType::Calendar:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_calendar_32_.png");
		break;
		case IconType::Camera:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_camera_roll_32_.png");
		break;
		case IconType::NoteBook:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_notebook_32_.png");
		break;
		case IconType::CloudPC:
			this->mPic->BackgroundImage = System::Drawing::Image::FromFile(L".\\images\\ControlPanel\\icon_control_cloud_pc_32_.png");
		break;
	}
}

bool CSyncItem::IsChecked::get()
{
	if(mStatus == StatusType::Checked)
	{
		return true;
	}
	else 
		return false;
}

void CSyncItem::IsChecked::set(bool value)
{
	
	if(value)
	{
		ImageStatus = StatusType::Checked;		
	}
	else
	{
		ImageStatus = StatusType::Normal;
	}
}