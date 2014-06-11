#pragma once
#include "MyButton.h"

	public enum class IconType
	{
		Contacts,
		Calendar,
		Camera,
		Browswer,
		NoteBook,
		CloudPC,
	};

ref class CSyncItem :
public System::Windows::Forms::Panel
{

	enum class StatusType
	{
		Disabled,
		Normal,
		Hover,
		Checked,
		Focus,
	};


public:
	CSyncItem(void);
	int mnIndexInList;
	property StatusType ImageStatus{StatusType get() {return mStatus; }; void set(StatusType value);}
	property IconType ChangeIconType{void set(IconType value);}
	property System::String^ LabelText{ System::String^ get(){return mLabel->Text;};  void set(System::String^ value){mLabel->Text = value;}; }
	property bool IsChecked{bool get() ; void set(bool value);}
	property CMyButton^ GetButton{ CMyButton^ get() {  return mButton;}; }
	
protected:
	void mTouchPadPanel_MouseHover(System::Object^  sender, System::EventArgs^  e) ;
	void mTouchPadPanel_MouseLeave(System::Object^  sender, System::EventArgs^  e) ;
	void mTouchPadPanel_Click(System::Object^  sender, System::EventArgs^  e) ;
	void EnabledIsChanged(System::Object^  sender, System::EventArgs^  e) ;
private: 	
	System::Windows::Forms::PictureBox^  pictureBox_Line;
	System::Windows::Forms::Label^  mTouchPadPanel;
	System::Windows::Forms::Label^  mLabel;
	System::Windows::Forms::PictureBox^  mPic;
	System::Windows::Forms::PictureBox^  mCheckBox;
	CMyButton^  mButton;

	StatusType mStatus;
};
