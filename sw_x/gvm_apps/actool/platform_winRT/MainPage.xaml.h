//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace actool_winRT
{
	public ref class MainPage sealed
	{
	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    private:
        void btnResetDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void btnGenerate_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        void ShowMessageBox(Platform::String^ msg);
    };
}
