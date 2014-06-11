#pragma once

ref class CMyButton :
public  System::Windows::Forms::Panel
{
public:
	CMyButton(void);
	CMyButton(bool bUnlinkBtn);

	delegate void OnClickedHandler();	
    OnClickedHandler^ OnClickedEvent;

	//property System::String^ MyText{System::String^ get(); void set(System::String^ value);};
	virtual property System::String^ Text{System::String^ get()override; void set(System::String^ value)override;}

	//property CMyButton^ GetBtn{CMyButton^ get();};
protected:
	System::Windows::Forms::Label^ mBtn;
	System::Windows::Forms::PictureBox^ mPicL;
	System::Windows::Forms::PictureBox^ mPicR;
	System::Windows::Forms::PictureBox^ mPicUnlink;

	void MyMouseDown( System::Object^ /*sender*/, System::Windows::Forms::MouseEventArgs^ e );
	void MyMouseEnter(System::Object^  sender, System::EventArgs^  e) ;
	void MyMouseLeave(System::Object^  sender, System::EventArgs^  e);
	void MyMouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);
	void MySizeChanged( System::Object^ /*sender*/, System::EventArgs^ /*e*/ );
	void MyEnabledChanged( System::Object^ sender, System::EventArgs^ e );
	void MyMouseClicked( System::Object^ /*sender*/, System::EventArgs^ /*e*/ );
	
	void ButtonIsHovered();
	void ButtonIsPressed();
	void ButtonIsNormal();
	void ButtonIsDisabled();
private:
	bool mbUnlink;
	void Init(bool bShowUnlinkBtn);

};
