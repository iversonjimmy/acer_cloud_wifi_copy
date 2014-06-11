//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//  
#include "stdafx.h"

#include "PlayerOptions.h"
#include "settings_form.h"
#include "util_funcs.h"
#include "AlphaBGForm.h"
#include "MessageProcForm.h"
#include "logger.h"
#include "WM_USER.h"
//
//using namespace System::Collections::Generic;
//using namespace System::Diagnostics;
//using namespace System::IO;
//using namespace System::Runtime::InteropServices;
//using namespace System::Threading;


static bool checkState()
{
	cli::array<System::Diagnostics::Process^>^ list = System::Diagnostics::Process::GetProcesses();

    // if app already running do nothing
    for (int i = 0; i < list->Length; i++) {
		if (list[i]->ProcessName == System::Diagnostics::Process::GetCurrentProcess()->ProcessName
            && list[i]->Id != System::Diagnostics::Process::GetCurrentProcess()->Id) 
		{			
			PostMessage(FindWindow(nullptr, L"Acer TS MsgProc "), WM_LAUNCH, 0, 0);
            return false;
        }
    }

    // kill all ccd.exe processes
    for (int i = 0; i < list->Length; i++) {
        if (list[i]->ProcessName == L"ccd") {
            list[i]->Kill();
            list[i]->WaitForExit();
        }
    }

    return true;
}

static void ccdExit(Object^ sender, EventArgs^ e)
{
    Logger::Instance->WriteLine("ccd.exe has exited...");
    Application::Exit();
}

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
    // MUST BE FIRST
    // start app in a clean state
    if (checkState() == false) {
	    return 0;
    }

    Logger::Instance->Init();

    // wait for network to be up
    Logger::Instance->Write(L"waiting for network...");
    while (System::Net::NetworkInformation::NetworkInterface::GetIsNetworkAvailable() == false) {
		System::Threading::Thread::Sleep(1000);
    }
    Logger::Instance->WriteLine(L"done");

   

    System::Diagnostics::Process^ ccd = gcnew System::Diagnostics::Process();
	ccd->StartInfo->FileName = System::Environment::GetFolderPath(System::Environment::SpecialFolder::ProgramFiles);
    ccd->StartInfo->FileName += L"\\iGware\\SyncAgent\\ccd.exe";
	ccd->StartInfo->Arguments = L"\"" + System::Environment::GetFolderPath(System::Environment::SpecialFolder::LocalApplicationData);
    ccd->StartInfo->Arguments += L"\\iGware\\SyncAgent\"";
    ccd->StartInfo->UseShellExecute = false;
    ccd->StartInfo->CreateNoWindow = true;
    ccd->EnableRaisingEvents = true;
	ccd->Exited += gcnew System::EventHandler(&ccdExit);

    Logger::Instance->WriteLine("starting " + ccd->StartInfo->FileName + " " + ccd->StartInfo->Arguments + "...");
    ccd->Start();

    try {
        Logger::Instance->WriteLine("waiting 1 second for ccd initialization...");
		System::Threading::Thread::Sleep(1000);

        // HACK
        CcdManager::Instance->LoadSettings();

        // log in previous user if any
        try {
            if (CcdManager::Instance->UserName->Length > 0) {
                CcdManager::Instance->Login(CcdManager::Instance->UserName);
                if (CcdManager::Instance->IsLoggedIn == true
                    && CcdManager::Instance->IsDeviceLinked == false) {
                    CcdManager::Instance->Logout(true);
                }
                if (CcdManager::Instance->IsLoggedIn == true) {
                    CcdManager::Instance->UpdateDataSets();
                }
            }
        } catch (Exception^) {
        }

	    Logger::Instance->WriteLine("start GUI application...");

        // Enabling Windows XP visual effects before any controls are created
		
	    Application::EnableVisualStyles();
	    Application::SetCompatibleTextRenderingDefault(false);		

		SettingsForm^ settingsForm = gcnew SettingsForm();
		acpanel_win::AlphaBGForm^ alphaForm = gcnew acpanel_win::AlphaBGForm();
		alphaForm->SetControlledWindow( settingsForm->GetSetupForm);
			
		acpanel_win::MessageProcForm^ msgProcForm = gcnew acpanel_win::MessageProcForm();
		msgProcForm->Show();
		msgProcForm->SetForms(settingsForm);
		msgProcForm->dlOpenAcerTSEvent += gcnew acpanel_win::MessageProcForm::dlOpenAcerTS(settingsForm, &SettingsForm::OpenAcerTS);
		

	
	

		if (CcdManager::Instance->IsLoggedIn)
		{
			//settingsForm->GetSetupForm->Show(alphaForm);
			settingsForm->GetSetupForm->Visible = false;
			alphaForm->Visible = false;
		}
		else
		{
			alphaForm->Show();
			if(settingsForm->GetSetupForm->Visible)
				settingsForm->GetSetupForm->Hide();
			settingsForm->GetSetupForm->Show(alphaForm);
			settingsForm->GetSetupForm->BringToFront();		
		}

		Application::Run();
		//Application::Run(alphaForm);
        //Application::Run(gcnew SettingsForm());

        // log out
        if (CcdManager::Instance->IsLoggedIn == true) {
            CcdManager::Instance->Logout(false);
        }
    } catch (Exception^ e) {
        // prevents major crash so we can exit gracefully
        MessageBox::Show(e->Message);
    }

    if (ccd->HasExited == false) {
        Logger::Instance->WriteLine("killing ccd.exe...");
        ccd->EnableRaisingEvents = false; // prevents IO exception
        ccd->Kill();
    }

    Logger::Instance->Quit();

	return 0;
}

/*
// for testing windows form functionality

#include "Form1.h"

[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
    // Enabling Windows XP visual effects before any controls are created
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew Form1());

	return 0;
}
*/
