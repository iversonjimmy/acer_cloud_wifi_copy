//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace App1
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
    [Windows::UI::Xaml::Data::Bindable]
	public ref class MainPage sealed
	{
	public:
		MainPage();
        property Platform::String^ VPLTestResult; 

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

    private:
        void Button_Click_Start(Platform::Object^ Sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
