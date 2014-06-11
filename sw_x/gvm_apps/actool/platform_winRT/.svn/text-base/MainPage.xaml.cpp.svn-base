//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <ppltasks.h>
#include <string>

using namespace actool_winRT;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Popups;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Concurrency;

#define CCD_CONFIG_FILE_NAME L"ccd.conf"
#define CCD_CONFIG_TMPL_FILE_NAME L"ccd.conf.tmpl"
#define RESET_DEFAULT_MSG L"Config reset to default state. Please restart the acerCloud application."
#define GENERATED_MSG L"Config generated successfully. Please restart the acerCloud application."

MainPage::MainPage()
{
    InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    (void) e;	// Unused parameter
}

void actool_winRT::MainPage::ShowMessageBox(String^ msg)
{
    auto msgDlg = ref new Windows::UI::Popups::MessageDialog(msg, L"");
    msgDlg->ShowAsync();
}

void actool_winRT::MainPage::btnResetDefault_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    //get ccd.conf file from Music Library
    auto getAction = KnownFolders::MusicLibrary->GetFileAsync(ref new String(CCD_CONFIG_FILE_NAME));
    //if action Error, which means file not exsit, just show success msg & return
    if(getAction->Status == AsyncStatus::Error) {
        ShowMessageBox(ref new String(RESET_DEFAULT_MSG));
        return;
    }

    task<StorageFile^> getFileTask( getAction );
    getFileTask.then(
        [this] (StorageFile^ file) {
            //get StorageFile object of ccd.conf succeed, delete it
            auto deleteAction = file->DeleteAsync();
            if(deleteAction->Status == AsyncStatus::Error) {
                ShowMessageBox(ref new String(RESET_DEFAULT_MSG));
                return;
            }

            task<void> deleteTask( deleteAction );
            deleteTask.then(
                [this]() {
                    //delete succeed, show msg
                    ShowMessageBox(ref new String(RESET_DEFAULT_MSG));
            }   
            );
    }
    ).then(
        [this](task<void> preTask) {
            try {
                preTask.get();
            }
            catch(Exception^ e){
                //if action throws Exception, which means file not exsit, just show success msg & return
                ShowMessageBox(ref new String(RESET_DEFAULT_MSG));
                return;
            }
    }
    );

}

std::wstring ReplaceWString( const std::wstring& orignStr, const std::wstring& oldStr, const std::wstring& newStr ) 
{ 
    size_t pos = 0; 
    std::wstring tempStr = orignStr; 
    std::wstring::size_type newStrLen = newStr.length(); 
    std::wstring::size_type oldStrLen = oldStr.length(); 
    while(true) 
    { 
        pos = tempStr.find(oldStr, pos); 
        if (pos == std::wstring::npos) break;
        tempStr.replace(pos, oldStrLen, newStr);         
        pos += newStrLen;
    }
    return tempStr; 
}

void actool_winRT::MainPage::btnGenerate_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    //check if domain textbox is filled
    if(this->txtDomain->Text->IsEmpty()) {
        ShowMessageBox(L"Please fill in the domain!");
        return;
    }

    IAsyncOperation<StorageFile^>^ getAction = nullptr;
    try {
        //get ccd.conf.tmpl file from installed path of actool_winRT
        getAction = Windows::ApplicationModel::Package::Current->InstalledLocation->GetFileAsync(ref new String(CCD_CONFIG_TMPL_FILE_NAME));
    }
    catch(Exception^ e){
        //if action throws Exception, which means ccd.conf.tmpl file not exsit: show error msg
        String^ msg = L"Unable to find templete config file: "+ CCD_CONFIG_TMPL_FILE_NAME + L" in program folder!";
        ShowMessageBox(msg);
        return;
    }

    String^ domain = this->txtDomain->Text;
    String^ group = this->txtGroup->Text;
    task<StorageFile^> getFileTask( getAction );
    getFileTask.then(
        [this,domain,group] (StorageFile^ file) {
            //get ccd.conf.tmpl file succeed, try read all content from it
            task<String^>(FileIO::ReadTextAsync(file)).then(
                [this,domain,group](String^ fileContent) {
                    //read all content succeed
                    std::wstring wcontent(fileContent->Data());
                    //replace "${DOMAIN}" string to user input domain
                    std::wstring replaceDomain = ReplaceWString(wcontent,L"${DOMAIN}",domain->Data());
                    //replace "${GROUP}" string with user input
                    std::wstring replaceGroup = ReplaceWString(replaceDomain,L"${GROUP}",group->Data());
                    String^ newContent = ref new String( replaceGroup.c_str() );
                    //Create AsyncAgent Folder in Music Library
                    task<StorageFolder^>(KnownFolders::MusicLibrary->CreateFolderAsync(L"AcerCloud", CreationCollisionOption::OpenIfExists)).then(
                        [this,newContent](StorageFolder^ syncFolder) {
                            //create conf folder
                            task<StorageFolder^>(syncFolder->CreateFolderAsync(L"conf", CreationCollisionOption::OpenIfExists)).then(
                                [this,newContent](StorageFolder^ confFolder){
                                    //create ccd.conf file in Music Library
                                    task<StorageFile^>(confFolder->CreateFileAsync(ref new String(CCD_CONFIG_FILE_NAME), CreationCollisionOption::ReplaceExisting)).then(
                                        [this,newContent](StorageFile^ file) {
                                            //create ccd.conf file succeed, try write content with new domain to the file
                                            task<void>(FileIO::WriteTextAsync(file, newContent)).then(
                                                [this]() {
                                                    //write succeed, show success msg
                                                    ShowMessageBox(ref new String(GENERATED_MSG));
                                                }
                                            );
                                        }
                                    );
                                }
                            );
                        }
                    );
                }
            );
        }
    ).then(
        [this](task<void> preTask) {
            try {
                preTask.get();
            }
            catch(Exception^ e){
                //if action throws Exception, which means ccd.conf.tmpl file not exsit: show error msg
                String^ msg = L"Unable to find templete config file: "+ CCD_CONFIG_TMPL_FILE_NAME + L" in program folder!";
                ShowMessageBox(msg);
                return;
            }
        }
    );
}


