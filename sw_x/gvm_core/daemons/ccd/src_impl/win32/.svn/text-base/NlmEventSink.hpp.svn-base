#pragma once

//============================================================================
/// @file
/// Class to receive events from the Windows Network List Manager (NLM).
/// Based on sample code from
/// http://archive.msdn.microsoft.com/NLM/Release/ProjectReleases.aspx?ReleaseId=3011
//============================================================================

#include "vplex_plat.h"
#include "log.h"

#include <atlbase.h>
#include <atlcom.h>
#include <Objbase.h>
#include <Netlistmgr.h>

// TODO: put this somewhere more appropriate
#include <string>
std::string WinGuidToStr(GUID guid);

/// NlmEventSink class; implements COM interfaces for receiving NLM events.
class ATL_NO_VTABLE NlmEventSink :
    public CComObjectRootEx<CComSingleThreadModel>,
    public INetworkListManagerEvents,
    public INetworkEvents,
    public INetworkConnectionEvents
{
private:
    DWORD                     m_dwCookie;
    CComPtr<IConnectionPoint> m_pConnectionPoint;
    GUID                      m_riid;
    HANDLE                    m_hThread;
    DWORD                     m_dwThreadId;
    /// If StopListeningForEvents sends the WM_QUIT before the thread is ready, the message will
    /// be lost and StopListeningForEvents will get stuck waiting for the thread to exit.
    volatile bool             m_readyForQuit;

    HRESULT ListenForEvents();
    static DWORD WINAPI StartListeningForEventsThread(LPVOID pArg);

public:
    static HRESULT StartListeningForEvents(REFIID riid, NlmEventSink** ppEventResponse);
    HRESULT StopListeningForEvents();

protected:
    NlmEventSink()
    {
         m_dwCookie = 0;
         m_riid = GUID_NULL;
         m_hThread = NULL;
         m_dwThreadId = 0;
         m_readyForQuit = false;
    }

    DECLARE_NO_REGISTRY()

    BEGIN_COM_MAP(NlmEventSink)
        COM_INTERFACE_ENTRY(INetworkListManagerEvents)
        COM_INTERFACE_ENTRY(INetworkEvents)
        COM_INTERFACE_ENTRY(INetworkConnectionEvents)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct()
    {
        return S_OK;
    }

    void FinalRelease()
    {
    }

    // INetworkListManagerEvents
    STDMETHOD(ConnectivityChanged)( NLM_CONNECTIVITY NewConnectivity);

    // INetworkEvents
    STDMETHOD(NetworkAdded)(GUID networkId);
    STDMETHOD(NetworkDeleted)(GUID networkId);
    STDMETHOD(NetworkConnectivityChanged)(GUID networkId,  NLM_CONNECTIVITY newConnectivity);
    STDMETHOD(NetworkPropertyChanged)(GUID networkId, NLM_NETWORK_PROPERTY_CHANGE flags);

    // INetworkConnectionEvents
    STDMETHOD(NetworkConnectionConnectivityChanged)(GUID networkId,  NLM_CONNECTIVITY newConnectivity);
    STDMETHOD(NetworkConnectionPropertyChanged)(GUID networkId, NLM_CONNECTION_PROPERTY_CHANGE flags);
};

