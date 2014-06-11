#include <windows.h>
#include <objbase.h>
#include <iostream>
#include <string>
#include <Shobjidl.h>

#using <Windows.winmd>

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Management::Deployment;
using namespace std;

void DisplayError(LPWSTR pszAPI)
{
    LPVOID lpvMessageBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), 
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
        (LPWSTR)&lpvMessageBuffer, 0, NULL);

    //
    //... now display this string
    //
    wprintf(L"ERROR: API        = %s.\n", pszAPI);
    wprintf(L"       error code = %d.\n", GetLastError());
    wprintf(L"       message    = %s.\n", (LPWSTR)lpvMessageBuffer);

    //
    // Free the buffer allocated by the system
    //
    LocalFree(lpvMessageBuffer);

    ExitProcess(GetLastError());
}

[MTAThread]
int __cdecl main(Platform::Array<String^>^ args)
{
    if (args->Length < 3)
    {
        wcout << "Error usage!!" << endl;
        wcout << "Usage: metro_app_utilities.exe add <packageUri>" << endl;
        wcout << "Usage: metro_app_utilities.exe remove <packetFullName>" << endl;
        wcout << "Usage: metro_app_utilities.exe launch <familyFullName>" << endl;
        return 1;
    }

    HANDLE completedEvent = nullptr;
    int returnValue = 0;
    String^ inputActionArg = args[1];
    String^ inputPackageArg = args[2];

    try
    {
        if (inputActionArg == ref new String(L"launch")) {
           	CoInitialize(NULL);
	        IApplicationActivationManager* paam = NULL;
	        HRESULT hr = E_FAIL;

            hr = CoCreateInstance(CLSID_ApplicationActivationManager, NULL,CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&paam));
		    if (FAILED(hr))
                goto end;

		    DWORD pid = 0;
            hr = paam->ActivateApplication(inputPackageArg->Data(), nullptr, AO_NONE, &pid);
		    if (FAILED(hr))
                goto end;

            wprintf(L"Activated  %s with pid %d\r\n", inputPackageArg->Data(), pid);

end:
		    if (paam)
                paam->Release();

	        CoUninitialize();
        }
        else if (inputActionArg == ref new String(L"add") 
         || inputActionArg == ref new String(L"remove")) {
            completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if (completedEvent == nullptr)
            {
                wcout << L"CreateEvent Failed, error code=" << GetLastError() << endl;
                returnValue = 1;
            }
            else
            {
                auto packageManager = ref new PackageManager();
                Windows::Foundation::IAsyncOperationWithProgress<Windows::Management::Deployment::DeploymentResult^, Windows::Management::Deployment::DeploymentProgress>^ deploymentOperation = nullptr;

                if (inputActionArg == ref new String(L"add")) {
                    auto packageUri = ref new Uri(inputPackageArg);
                    deploymentOperation = packageManager->AddPackageAsync(packageUri, nullptr, DeploymentOptions::None);
                    deploymentOperation->Completed =
                        ref new AsyncOperationWithProgressCompletedHandler<DeploymentResult^, DeploymentProgress>(
                        [&completedEvent](IAsyncOperationWithProgress<DeploymentResult^, DeploymentProgress>^ operation, AsyncStatus)
                    {
                        SetEvent(completedEvent);
                    });

                    wcout << L"Installing package " << inputPackageArg->Data() << endl;

                    wcout << L"Waiting for installation to complete..." << endl;
                }
                else if (inputActionArg == ref new String(L"remove")) {
                    deploymentOperation = packageManager->RemovePackageAsync(inputPackageArg);
                    deploymentOperation->Completed =
                        ref new AsyncOperationWithProgressCompletedHandler<DeploymentResult^, DeploymentProgress>(
                        [&completedEvent](IAsyncOperationWithProgress<DeploymentResult^, DeploymentProgress>^ operation, AsyncStatus)
                    {
                        SetEvent(completedEvent);
                    });

                    wcout << L"Removing package " << inputPackageArg->Data() << endl;

                    wcout << L"Waiting for removal to complete..." << endl;
                }

                WaitForSingleObject(completedEvent, INFINITE);

                if (completedEvent != nullptr)
                    CloseHandle(completedEvent);

                if (deploymentOperation->Status == AsyncStatus::Error)
                {
                    auto deploymentResult = deploymentOperation->GetResults();
                    wcout << L"Installation Error: " << deploymentOperation->ErrorCode.Value << endl;
                    wcout << L"Detailed Error Text: " << deploymentResult->ErrorText->Data() << endl;
                }
                else if (deploymentOperation->Status == AsyncStatus::Canceled)
                {
                    wcout << L"Installation Canceled" << endl;
                }
                else if (deploymentOperation->Status == AsyncStatus::Completed)
                {
                    wcout << L"Installation succeeded!" << endl;
                }
            }
        }
        else {
            wcout << "Wrong action!!" << endl;
            wcout << "Usage: metro_app_utilities.exe add <packageUri>" << endl;
            wcout << "Usage: metro_app_utilities.exe remove <packetFullName>" << endl;
            wcout << "Usage: metro_app_utilities.exe launch <familyFullName>" << endl;
            return 1;
        }
    }
    catch (Exception^ ex)
    {
        wcout << inputActionArg->Data() << L" package " << inputPackageArg->Data() << " failed, error message: " << ex->ToString()->Data() << endl;
        returnValue = 1;
    }

    return returnValue;
}
