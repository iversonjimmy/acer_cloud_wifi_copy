#pragma once

ref class CTabButton :
public  System::Windows::Forms::Panel
{
public:
	CTabButton(void);
	//property bool IsMediaChange{bool get(); void set(bool value);};
	void Selected();
	void Unselected();
	//property System::Windows::Forms::Button^ GetBtn{System::Windows::Forms::Button^ get();};
	virtual property System::String^ Text{System::String^ get()override; void set(System::String^ value)override;}

	delegate void OnClickedHandler();	
    OnClickedHandler^ OnClickedEvent;

protected:
	//System::Windows::Forms::Button^ mBtn;
	System::Windows::Forms::Label^ mBtn;
	System::Windows::Forms::PictureBox^ mPicL;
	System::Windows::Forms::PictureBox^ mPicR;

	//void MyMouseDown( System::Object^ /*sender*/, System::Windows::Forms::MouseEventArgs^ e );
	//void MyMouseEnter(System::Object^  sender, System::EventArgs^  e) ;
	//void MyMouseLeave(System::Object^  sender, System::EventArgs^  e);
	//void MyMouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);
	void MySizeChanged( System::Object^ /*sender*/, System::EventArgs^ /*e*/ );
	void MyMouseClicked(System::Object^ /*sender*/, System::EventArgs^ /*e*/ );
	

	

};
