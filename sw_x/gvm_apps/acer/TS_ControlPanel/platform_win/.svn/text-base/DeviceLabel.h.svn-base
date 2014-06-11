#pragma once

#define DEVICE_LABEL_WIDTH	126
#define DEVICE_LABEL_HEIGHT	110

ref class CDeviceLabel:
public  System::Windows::Forms::Label
{
public:
	static System::String^ WINDOWS_PC = "WindowsPC";

	CDeviceLabel(void)
	{
		mbAndroid = false;
		mbAcer = false;
		mbOnline = false;
		mbCloudPC = false;
		mbSelected = false;
		mDeviceType = WINDOWS_PC;
		DeviceName = "Default Name";

		Font = (gcnew System::Drawing::Font(L"Arial", 11));
		ForeColor = System::Drawing::Color::White;
		TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
		this->Size = System::Drawing::Size(DEVICE_LABEL_WIDTH,DEVICE_LABEL_HEIGHT);
	};

	System::Int64 mDeviceID;
	System::String^ mDeviceType;
	bool mbAndroid;
	bool mbAcer;
	bool mbOnline;
	bool mbCloudPC;
	bool mbSelected;

	property System::String^ DeviceName{
										System::String^ get() 
										{
											return mDeviceName;
										} ; 
										void set(System::String^ value)
										{
											mDeviceName = value;
											if(value->Length >10)												
												this->Text = L"\r\n\r\n"+value->Substring(0, 10) + "...";
											else
												this->Text = L"\r\n\r\n"+value;
										};}


	void UpdateImg()
	{
		System::String^ fileName = L".\\images\\ControlPanel\\thumb_";
		if(mbCloudPC)
			fileName = fileName + L"cloud_";

		if(mDeviceType == WINDOWS_PC)
			fileName = fileName + L"notebook_";
		else
			fileName = fileName + L"phone_";

		if(mbOnline)
			fileName = fileName + L"online_";
		else
			fileName = fileName + L"offline_";

		if(mbSelected)
		{
			fileName = fileName + L"h.png";
			ForeColor = System::Drawing::Color::White;
		}
		else
		{
			fileName = fileName + L"n.png";
			ForeColor = System::Drawing::Color::FromArgb(255, 120, 120, 120);
		}

		this->BackgroundImage = System::Drawing::Image::FromFile(fileName);
	}

protected:
	
	System::String^ mDeviceName;

};