#ifndef _MSC_VER
#error "This should not be built for this platform"
#endif

#define DEBUG_NLM_EVENTS 1

#include "NlmEventSink.hpp"

#include "ans_connection.hpp"
#include "netman.hpp"
#include "cache.h"

#include "vpl_th.h"
#include "vpl_thread.h"
#include "vpl_time.h"
#include "vpl_lazy_init.h"

#include <string>

// To defer sending setup-sleep-request for n ms after received network change event.
#define WAIT_NETWORK_CHANGED_EVENT_TIMEOUT 1000

// It's the maximum interval of deferring sending out setup-sleep-request.
#define MAX_WAIT_NETWORK_CHANGED_EVENT_INTERVAL 15000

static VPLThread_t          m_monitorNetworkThread         = VPLTHREAD_INVALID;
static VPLLazyInitMutex_t   m_monitorNetworkMutex          = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitCond_t    m_monitorNetworkCond           = VPLLAZYINITCOND_INIT;
static bool                 m_isRunning                    = false;
static VPLTime_t            m_firstNetworkChangedTimestamp = VPLTIME_INVALID;
static VPLTime_t            m_lastNetworkChangedTimestamp  = VPLTIME_INVALID;
static VPLTime_t            m_waitTime                     = VPLTIME_FROM_MILLISEC(WAIT_NETWORK_CHANGED_EVENT_TIMEOUT);
static VPLTime_t            m_maxIntervalTime              = VPLTIME_FROM_MILLISEC(MAX_WAIT_NETWORK_CHANGED_EVENT_INTERVAL);
static bool                 m_delayNetworkChange           = false;

static void* monitorNetworkChangeThread(void* unused);
static void processNetworkChange() {
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
    m_delayNetworkChange = true;
    m_lastNetworkChangedTimestamp = VPLTime_GetTimeStamp();
    if (m_firstNetworkChangedTimestamp == VPLTIME_INVALID) {
        m_firstNetworkChangedTimestamp = m_lastNetworkChangedTimestamp;
    }
    VPLCond_Signal(VPLLazyInitCond_GetCond(&m_monitorNetworkCond));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
}

#if DEBUG_NLM_EVENTS
static std::string nlmConnectivityStr(NLM_CONNECTIVITY nlmConnectivity)
{
    std::string result = "";
    if (nlmConnectivity == NLM_CONNECTIVITY_DISCONNECTED) {
        result = "DISCONNECTED";
        goto done;
    }

    if (nlmConnectivity & NLM_CONNECTIVITY_IPV4_NOTRAFFIC) {
        result += "IPV4_NOTRAFFIC, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV4_SUBNET) {
        result += "IPV4_SUBNET, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV4_LOCALNETWORK) {
        result += "IPV4_LOCALNETWORK, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV4_INTERNET) {
        result += "IPV4_INTERNET, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV6_NOTRAFFIC) {
        result += "IPV6_NOTRAFFIC, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV6_SUBNET) {
        result += "IPV6_SUBNET, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV6_LOCALNETWORK) {
        result += "IPV6_LOCALNETWORK, ";
    }
    if (nlmConnectivity & NLM_CONNECTIVITY_IPV6_INTERNET) {
        result += "IPV6_INTERNET, ";
    }
    // Chop off any extra ", "
    if (result.size() > 1) {
        result.resize(result.size() - 2);
    }
done:
    return result;
}

static std::string nlmNetworkPropertyChangeStr(NLM_NETWORK_PROPERTY_CHANGE nlmNetworkPropertyChange)
{
    std::string result = "";
    if (nlmNetworkPropertyChange & NLM_NETWORK_PROPERTY_CHANGE_CONNECTION) {
        result += "CONNECTION, ";
    }
    if (nlmNetworkPropertyChange & NLM_NETWORK_PROPERTY_CHANGE_DESCRIPTION) {
        result += "DESCRIPTION, ";
    }
    if (nlmNetworkPropertyChange & NLM_NETWORK_PROPERTY_CHANGE_NAME) {
        result += "NAME, ";
    }
    if (nlmNetworkPropertyChange & NLM_NETWORK_PROPERTY_CHANGE_CATEGORY_VALUE) {
        result += "VALUE, ";
    }
    // Chop off any extra ", "
    if (result.size() > 1) {
        result.resize(result.size() - 2);
    }
    return result;
}

static std::string nlmConnectionPropertyChangeStr(NLM_CONNECTION_PROPERTY_CHANGE nlmConnectionPropertyChange)
{
    std::string result = "";
    if (nlmConnectionPropertyChange & NLM_CONNECTION_PROPERTY_CHANGE_AUTHENTICATION) {
        result += "AUTHENTICATION, ";
    }
    // Chop off any extra ", "
    if (result.size() > 1) {
        result.resize(result.size() - 2);
    }
    return result;
}

std::string WinGuidToStr(GUID guid)
{
    char temp[48];
    snprintf(temp, ARRAY_SIZE_IN_BYTES(temp),
            "{"FMTX032"-"FMTX016"-"FMTX016"-"FMTX08""FMTX08"-"FMTX08""FMTX08""FMTX08""FMTX08""FMTX08""FMTX08"}",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5],
            guid.Data4[6], guid.Data4[7]);
    return std::string(temp);
}

#  if 0
static std::string nlmNetworkCategoryStr(NLM_NETWORK_CATEGORY nlmNetworkCategory)
{
    std::string result = "";
    if (nlmNetworkCategory & NLM_NETWORK_CATEGORY_PUBLIC) {
        result += "PUBLIC, ";
    }
    if (nlmNetworkCategory & NLM_NETWORK_CATEGORY_PRIVATE) {
        result += "PRIVATE, ";
    }
    if (nlmNetworkCategory & NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED) {
        result += "DOMAIN AUTHENTICATED, ";
    }
    // Chop off any extra ", "
    if (result.size() > 1) {
        result.resize(result.size() - 2);
    }
    return result;
}

static const char* nlmDomainTypeStr(NLM_DOMAIN_TYPE nlmDomainType)
{
    switch(nlmDomainType) {
    case NLM_DOMAIN_TYPE_NON_DOMAIN_NETWORK:
        return "NON_DOMAIN_NETWORK";
    case NLM_DOMAIN_TYPE_DOMAIN_NETWORK:
        return "DOMAIN_NETWORK";
    case NLM_DOMAIN_TYPE_DOMAIN_AUTHENTICATED:
        return "DOMAIN_AUTHENTICATED";
    default:
        return "?";
    }
}

/// Prints out the properties for a given INetwork
void ShowInfoForNetwork(INetwork* pNetwork)
{
    HRESULT hr = S_OK;

    CComBSTR szName;
    hr = pNetwork->GetName(&szName);
    if (SUCCEEDED(hr))
    {
       wprintf(L"Network Name                   : %s\n", szName);
    }

    CComBSTR szDescription;
    hr = pNetwork->GetDescription(&szDescription);
    if (SUCCEEDED(hr))
    {
       wprintf(L"Network Description            : %s\n", szDescription);
    }

    VARIANT_BOOL bNetworkIsConnected;
    hr = pNetwork->get_IsConnected(&bNetworkIsConnected);
    if (SUCCEEDED(hr))
    {
        LPCWSTR sBool = (bNetworkIsConnected == VARIANT_TRUE) ? L"TRUE" : L"FALSE";
        wprintf(L"Connected to Network          : %s\n", sBool);
    }

    VARIANT_BOOL bNetworkIsConnectedToInternet;
    hr = pNetwork->get_IsConnectedToInternet(&bNetworkIsConnectedToInternet);
    if (SUCCEEDED(hr))
    {
        LPCWSTR sBool = (bNetworkIsConnectedToInternet == VARIANT_TRUE) ? L"TRUE" : L"FALSE";
        wprintf(L"Connected to Internet          : %s\n", sBool);
    }

    // IMPORTANT: Connectivity is a bitmask
    {
        NLM_CONNECTIVITY nlmConnectivity;
        hr = pNetwork->GetConnectivity(&nlmConnectivity);
        if( hr == S_OK) {
            nlmConnectivityStr(nlmConnectivity).c_str();
        }
    }


    //Whether this network is connected to domain network directly
    NLM_DOMAIN_TYPE DomainType;
    hr = pNetwork->GetDomainType( &DomainType );
    if ( hr == S_OK )
    {
        wprintf(L"Network domain type: " );
        PrintDomainTypeString( DomainType );
    }
    else
    {
        wprintf (L"INetwork::GetDomainType failed, hr = %#x\n", hr );
    }


    NLM_NETWORK_CATEGORY Category;
    hr = pNetwork->GetCategory( &Category );
    if ( hr == S_OK )
    {
        wprintf(L"Network Category: " );
        PrintCategoryString ( Category );
    }
    else
    {
        wprintf (L"INetwork::GetCategory failed, hr = %#x\n", hr );
    }

    GUID gdNetworkId;
    hr = pNetwork->GetNetworkId(&gdNetworkId);
    if (SUCCEEDED(hr))
    {
       wprintf(L"Network Id                     : %s\n", GetSzGuid(gdNetworkId));
    }

    CComPtr<IEnumNetworkConnections> pNetworkConnections;
    hr = pNetwork->GetNetworkConnections(&pNetworkConnections);
    if (SUCCEEDED(hr))
    {
        ShowNetworkConnections(pNetworkConnections);
    }
}
#define  NUM_NETWORK 3    // number of networks to fetch at one time
// Enumerates over all the connected networks and dump out information
// on each one of them
void ShowNetworks()
{
    HRESULT hr = S_OK;
    HRESULT hrCoinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hrCoinit) || (RPC_E_CHANGED_MODE == hrCoinit))
    {
        // scope for CComPtr<INetworkListManager>
        {
            CComPtr<INetworkListManager> pNLM;
            hr = CoCreateInstance(CLSID_NetworkListManager, NULL,
                CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
            if (SUCCEEDED(hr))
            {
                CComPtr<IEnumNetworks> pEnumNetworks;

                // Enumerate connected networks
                hr = pNLM->GetNetworks(NLM_ENUM_NETWORK_CONNECTED, &pEnumNetworks);
                if (SUCCEEDED(hr))
                {
                    INetwork* pNetworks[NUM_NETWORK];
                    ULONG cFetched = 0;
                    BOOL  bDone = FALSE;
                    while (!bDone)
                    {
                        hr = pEnumNetworks->Next(_countof(pNetworks), pNetworks, &cFetched);
                        if (SUCCEEDED(hr) && (cFetched > 0))
                        {
                            for (ULONG i = 0; i < cFetched; i++)
                            {
                                ShowInfoForNetwork(pNetworks[i]);
                                pNetworks[i]->Release();
                            }
                        }
                        else
                        {
                            bDone = TRUE;
                        }
                    }
                }
            }
            else
            {
                wprintf( L"Failed to CoCreate INetworkListManager");
            }
        }

        if (RPC_E_CHANGED_MODE != hrCoinit)
        {
            CoUninitialize();
        }
    }
    else
    {
        hr = hrCoinit;
    }
}

// Prints out all the properties related to a specific INetworkConnection
void ShowInfoForNetworkConnection(INetworkConnection* pNetworkConnection)
{
    HRESULT hr = S_OK;

    GUID gdConnectionId;
    hr = pNetworkConnection->GetConnectionId(&gdConnectionId);
    if (SUCCEEDED(hr))
    {
        wprintf(L"    Connection Id              : %s\n", GetSzGuid(gdConnectionId));
    }

    GUID gdAdapterId;
    hr = pNetworkConnection->GetAdapterId(&gdAdapterId);
    if (SUCCEEDED(hr))
    {
        BSTR szAdapterIDName =  GetSzGuid(gdAdapterId);
        wprintf(L"    Adapter Id                 : %s\n", szAdapterIDName);
    }


    VARIANT_BOOL bNetworkIsConnected;
    hr = pNetworkConnection->get_IsConnected(&bNetworkIsConnected);
    if (SUCCEEDED(hr))
    {
        LPCWSTR sBool = (bNetworkIsConnected == VARIANT_TRUE) ? L"TRUE" : L"FALSE";
        wprintf(L"    Connected to Network       : %s\n", sBool);
    }

    VARIANT_BOOL bNetworkIsConnectedToInternet;
    hr = pNetworkConnection->get_IsConnectedToInternet(&bNetworkIsConnectedToInternet);
    if (SUCCEEDED(hr))
    {
        LPCWSTR sBool = (bNetworkIsConnectedToInternet == VARIANT_TRUE) ? L"TRUE" : L"FALSE";
        wprintf(L"    Connected to Internet      : %s\n", sBool);
    }

    // IMPORTANT: Connectivity is a bitmask
    NLM_CONNECTIVITY fConnectivity;
    hr = pNetworkConnection->GetConnectivity(&fConnectivity);
    if (SUCCEEDED(hr))
    {
       wprintf(L"    Connectivity bitmask       : 0x%08x (%s)\r\n", fConnectivity, NLM_CONNECTIVITY_ToString(fConnectivity));
    }

    NLM_DOMAIN_TYPE eDomainType;
    hr = pNetworkConnection->GetDomainType(&eDomainType);
    if ( hr == S_OK )
    {
        wprintf(L"    Connection domain type: " );
        PrintDomainTypeString( eDomainType );
    }
}
#define  NUM_CONNECTION 3    // number of networks to fetch at one time
// Cycles through all NetworkConnections in pEnum and pass them to ShowInfoForNetworkConnection
void ShowNetworkConnections(IEnumNetworkConnections* pEnum)
{
    HRESULT hr = S_OK;

    INetworkConnection* pNetworkConnections[NUM_CONNECTION];
    ULONG cFetched = 0;
    BOOL  bDone = FALSE;
    while (!bDone)
    {
        hr = pEnum->Next(_countof(pNetworkConnections), pNetworkConnections, &cFetched);
        if (SUCCEEDED(hr) && (cFetched > 0))
        {
            for (ULONG i = 0; i < cFetched; i++)
            {
                ShowInfoForNetworkConnection(pNetworkConnections[i]);
                pNetworkConnections[i]->Release();
            }
        }
        else
        {
            bDone = TRUE;
        }
    }
}

static HRESULT
queryNlmIsConnected(VPL_BOOL* isConnectedLocal_out, VPL_BOOL* isConnectedInternet_out)
{
    *isConnectedLocal_out = FALSE;
    *isConnectedInternet_out = FALSE;

    HRESULT hr = S_OK;
    HRESULT hrCoinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hrCoinit) || (RPC_E_CHANGED_MODE == hrCoinit)) {
        // Explicit scope for CComPtr<INetworkListManager>
        {
            CComPtr<INetworkListManager> pNLM;
            hr = CoCreateInstance(CLSID_NetworkListManager,
                    NULL, CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
            if (SUCCEEDED(hr)) {
                VARIANT_BOOL bIsConnected;
                HRESULT hrtemp;

                hrtemp = pNLM->get_IsConnected(&bIsConnected);
                if (SUCCEEDED(hrtemp)) {
                    *isConnectedLocal_out = (bIsConnected == VARIANT_TRUE);
                }

                hrtemp = pNLM->get_IsConnectedToInternet(&bIsConnected);
                if (SUCCEEDED(hrtemp)) {
                    *isConnectedInternet_out = (bIsConnected == VARIANT_TRUE);
                }
            }
        } // pNLM->Release() gets called

        if (RPC_E_CHANGED_MODE != hrCoinit) {
            CoUninitialize();
        }
    }
    else {
        hr = hrCoinit;
    }
    return hr;
}
#endif // if 0 (unused code)

#endif

/// Creates the NlmEventSink object and starts a thread to perform an advise on it.
HRESULT NlmEventSink::StartListeningForEvents(REFIID riid, NlmEventSink** ppEventResponse)
{
    if (!ppEventResponse) {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    // Create our NlmEventSink object that will be used to advise to the Connection
    // point
    NlmEventSink *pEventResponse = new CComObject<NlmEventSink>();
    if (pEventResponse) {
        pEventResponse->AddRef();
        pEventResponse->m_riid = riid;

        // If you also want to use NlmEventSink in this thread, you should 
        // use CoMarshalInterThreadInterfaceInStream and pass the IStream* 
        // instead of the NlmEventSink* here.
        //
        // As it is, the NlmEventSink is only initialized and used on
        // the thread being created (inside ListenForEvents), 
        // and we can avoid the cross-apartment marshalling.
        pEventResponse->m_hThread = CreateThread(NULL, 
                                            0, 
                                            &StartListeningForEventsThread, 
                                            pEventResponse, 
                                            0, 
                                            &(pEventResponse->m_dwThreadId));

        if (pEventResponse->m_hThread == INVALID_HANDLE_VALUE) {
            DWORD dwError = GetLastError();
            hr = HRESULT_FROM_WIN32(dwError);
        }

        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
        if (SUCCEEDED(hr) && !m_isRunning) {
            int rc;
            VPLThread_attr_t thread_attr;

            m_isRunning = true;
            rc = VPLThread_AttrInit(&thread_attr);
            rc = VPLThread_Create(&m_monitorNetworkThread,
                                  monitorNetworkChangeThread, NULL,
                                  &thread_attr, "monitorNetworkChangeThread");
            if(rc != VPL_OK) {
                LOG_ERROR("VPLThread_Create:%d", rc);
            }
            VPLThread_AttrDestroy(&thread_attr);
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));

        if (SUCCEEDED(hr)) {
            *ppEventResponse = pEventResponse;
            (*ppEventResponse)->AddRef();
        }

        pEventResponse->Release();
    }
    else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/// This is our thread entry proc for the thread that will listen on events.
DWORD WINAPI NlmEventSink::StartListeningForEventsThread(LPVOID pArg)
{
    NlmEventSink* pThis = reinterpret_cast<NlmEventSink*>(pArg);
    HRESULT hr = S_OK;
    HRESULT hrCoinit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrCoinit) || (RPC_E_CHANGED_MODE == hrCoinit)) {
        hr = pThis->ListenForEvents();
        if (RPC_E_CHANGED_MODE != hrCoinit) {
            CoUninitialize();
        }
    }
    else {
        hr = hrCoinit;
    }
    pThis->m_readyForQuit = true;
    return hr;
}

void* monitorNetworkChangeThread(void* unused)
{
    m_delayNetworkChange = false;
    LOG_INFO("Monitoring NetworkChangedEvents thread started");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
    while (m_isRunning) {
        VPLTime_t waitTimeout = VPL_TIMEOUT_NONE;
        if (m_delayNetworkChange) {
            VPLTime_t currTime = VPLTime_GetTimeStamp();
            LOG_INFO("Got NetworkChangedEvents,  currTime:"FMTu64"  first:"FMTu64"  last:"FMTu64, currTime, m_firstNetworkChangedTimestamp, m_lastNetworkChangedTimestamp);
            if (VPLTime_DiffClamp(currTime, m_firstNetworkChangedTimestamp) > m_maxIntervalTime ||
                VPLTime_DiffClamp(currTime, m_lastNetworkChangedTimestamp) > m_waitTime) {

                // Send sleep-setup-request
                LOG_INFO("call NetMan_NetworkChange()");
                NetMan_NetworkChange();
                m_delayNetworkChange = false;
                m_firstNetworkChangedTimestamp = VPLTIME_INVALID;
                m_lastNetworkChangedTimestamp = VPLTIME_INVALID;

            } else {
                waitTimeout = MIN((m_lastNetworkChangedTimestamp + m_waitTime) - currTime, (m_firstNetworkChangedTimestamp + m_maxIntervalTime) - currTime);
            }
        }

        LOG_INFO("waitTimeout:"FMTu64, waitTimeout);
        VPLCond_TimedWait(VPLLazyInitCond_GetCond(&m_monitorNetworkCond),
                                                  VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex),
                                                  waitTimeout);
        LOG_INFO("after waitTimeout.");
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
    LOG_INFO("Monitoring NetworkChangedEvents thread stopped");
    return NULL;
}

/// The main listener function. Listens, and waits on a Message Loop.
/// We stop this thread by posting WM_QUIT to it.
HRESULT NlmEventSink::ListenForEvents()
{
    HRESULT hr;
    {
        CComPtr<INetworkListManager> pNLM;
        hr = CoCreateInstance(CLSID_NetworkListManager, NULL,
                CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
        if (FAILED(hr)) {
            LOG_ERROR("CoCreateInstance(CLSID_NetworkListManager) failed: "FMT_HRESULT, hr);
            goto out;
        }
        CComPtr<IConnectionPointContainer> pCpc;
        hr = pNLM->QueryInterface(IID_IConnectionPointContainer, (void**)&pCpc);
        if (FAILED(hr)) {
            LOG_ERROR("QueryInterface(IID_IConnectionPointContainer) failed: "FMT_HRESULT, hr);
            goto out;
        }
        hr = pCpc->FindConnectionPoint(m_riid, &m_pConnectionPoint);
        if (FAILED(hr)) {
            LOG_ERROR("FindConnectionPoint failed: "FMT_HRESULT, hr);
            goto out;
        }
        CComPtr<IUnknown> pSink;
        hr = this->QueryInterface(IID_IUnknown, (void**)&pSink);
        if (FAILED(hr)) {
            LOG_ERROR("QueryInterface failed: "FMT_HRESULT, hr);
            goto out;
        }
        hr = m_pConnectionPoint->Advise(pSink, &m_dwCookie);
        if (FAILED(hr)) {
            LOG_ERROR("Advise failed: "FMT_HRESULT, hr);
            goto out;
        }
        LOG_INFO("NlmEventSink::ListenForEvents ready");
        m_readyForQuit = true;
        BOOL bRet;
        MSG msg;
        while((bRet = GetMessage(&msg, NULL, 0, 0 )) != 0) {
            if (bRet == -1) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
out:
    LOG_INFO("Exiting NlmEventSink::ListenForEvents");
    return hr;
}

/// Stops the event listener thread by posting WM_QUIT to it, then
/// wait for it to exit, and return the exit code.
HRESULT NlmEventSink::StopListeningForEvents()
{
    HRESULT hr = S_OK;
    while (!m_readyForQuit) {
        // This condition should rarely be hit (only when we abort during startup), and
        // m_readyForQuit should be set soon after the other thread is allowed to run.
        // So, a simple spin and sleep should be fine here.
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(10));
    }
    if (m_pConnectionPoint != NULL) {
        hr = m_pConnectionPoint->Unadvise(m_dwCookie);
    }
    if (m_hThread != INVALID_HANDLE_VALUE) {
        PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);
        LOG_INFO("Waiting for NlmEventSink thread");
        WaitForSingleObject(m_hThread, INFINITE);
        DWORD dwExitCode;
        GetExitCodeThread(m_hThread, &dwExitCode);
        hr = dwExitCode;
        CloseHandle(m_hThread);
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
    if (m_isRunning) {
        m_delayNetworkChange = false;
        m_isRunning = false;
        m_firstNetworkChangedTimestamp = VPLTIME_INVALID;
        m_lastNetworkChangedTimestamp = VPLTIME_INVALID;
        VPLCond_Signal(VPLLazyInitCond_GetCond(&m_monitorNetworkCond));
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&m_monitorNetworkMutex));
    return hr;
}

static void processConnectivityChange(NLM_CONNECTIVITY newConnectivity)
{
    // Filter out the connectivity types that we don't care about.
    static const int MASK = ~(NLM_CONNECTIVITY_DISCONNECTED |
            NLM_CONNECTIVITY_IPV4_NOTRAFFIC | NLM_CONNECTIVITY_IPV6_NOTRAFFIC |
            NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV6_SUBNET);
    int filtered = newConnectivity & MASK;
    if (filtered != 0) {
        ANSConn_ReportNetworkConnected();

        // Report it to tunnel layer too
        LocalServers_ReportNetworkConnected();
    }
}

//--------------------------------
// Callbacks
//--------------------------------

/// Implements INetworkListManagerEvents::ConnectivityChanged.
STDMETHODIMP NlmEventSink::ConnectivityChanged(NLM_CONNECTIVITY NewConnectivity)
{
    // NLM_CONNECTIVITY_DISCONNECTED
    //     The underlying network interfaces have no connectivity to any network.
    // NLM_CONNECTIVITY_IPV4_NOTRAFFIC
    //     There is connectivity to a network, but the service cannot detect any IPv4 Network Traffic.
    // NLM_CONNECTIVITY_IPV6_NOTRAFFIC
    //     There is connectivity to a network, but the service cannot detect any IPv6 Network Traffic.
    // NLM_CONNECTIVITY_IPV4_SUBNET
    //     There is connectivity to the local subnet using the IPv4 protocol.
    // NLM_CONNECTIVITY_IPV4_LOCALNETWORK
    //     There is connectivity to a routed network using the IPv4 protocol.
    // NLM_CONNECTIVITY_IPV4_INTERNET
    //     There is connectivity to the Internet using the IPv4 protocol.
    // NLM_CONNECTIVITY_IPV6_SUBNET
    //     There is connectivity to the local subnet using the IPv6 protocol.
    // NLM_CONNECTIVITY_IPV6_LOCALNETWORK
    //     There is connectivity to a local network using the IPv6 protocol.
    // NLM_CONNECTIVITY_IPV6_INTERNET
    //     There is connectivity to the Internet using the IPv6 protocol.
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: Device-wide connectivity change: 0x"FMTx32" (%s)", NewConnectivity, nlmConnectivityStr(NewConnectivity).c_str());
#else
    LOG_INFO("NLM EVENT: Device-wide connectivity change: 0x"FMTx32, NewConnectivity);
#endif

    processNetworkChange();
    processConnectivityChange(NewConnectivity);
    return S_OK;
}

/// Implements INetworkEvents::NetworkAdded.
STDMETHODIMP NlmEventSink::NetworkAdded(GUID networkId)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: Network added: %s", WinGuidToStr(networkId).c_str());

#  if 0
    if (s_pNLM == NULL) {
        LOG_ERROR("");
    } else {
        CComPtr<INetwork> pNetwork;
        HRESULT hr = s_pNLM->GetNetwork(networkId, &pNetwork);
        if (SUCCEEDED(hr)) {
            ShowInfoForNetwork(pNetwork);
        }
    }
#  endif
#else
    LOG_INFO("NLM EVENT: Network added");
#endif

    processNetworkChange();
    return S_OK;
}

/// Implements INetworkEvents::NetworkDeleted.
STDMETHODIMP NlmEventSink::NetworkDeleted(GUID networkId)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: Network deleted: %s", WinGuidToStr(networkId).c_str());
#else
    LOG_INFO("NLM EVENT: Network deleted");
#endif

    processNetworkChange();
    return S_OK;
}

/// Implements INetworkEvents::NetworkConnectivityChanged.
STDMETHODIMP NlmEventSink::NetworkConnectivityChanged(GUID networkId,  NLM_CONNECTIVITY newConnectivity)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: Network connectivity change: %s 0x"FMTx32" (%s)",
            WinGuidToStr(networkId).c_str(), newConnectivity, nlmConnectivityStr(newConnectivity).c_str());
#else
    LOG_INFO("NLM EVENT: Network connectivity change");
#endif

    processNetworkChange();
    processConnectivityChange(newConnectivity);
    return S_OK;
}

/// Implements INetworkEvents::NetworkPropertyChange.
STDMETHODIMP NlmEventSink::NetworkPropertyChanged(GUID networkId, NLM_NETWORK_PROPERTY_CHANGE flags)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: Network property change: %s 0x"FMTx32" (%s)",
            WinGuidToStr(networkId).c_str(), flags, nlmNetworkPropertyChangeStr(flags).c_str());
#else
    LOG_INFO("NLM EVENT: Network property change");
#endif

    processNetworkChange();
    return S_OK;
}

/// Implements INetworkConnectionEvents::NetworkConnectionConnectivityChanged.
STDMETHODIMP NlmEventSink::NetworkConnectionConnectivityChanged(GUID networkId,  NLM_CONNECTIVITY newConnectivity)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: NetworkConnection connectivity change: %s 0x"FMTx32" (%s)",
            WinGuidToStr(networkId).c_str(), newConnectivity, nlmConnectivityStr(newConnectivity).c_str());
#else
    LOG_INFO("NLM EVENT: NetworkConnection connectivity change");
#endif

    processNetworkChange();
    processConnectivityChange(newConnectivity);
    return S_OK;
}

/// Implements INetworkConnectionEvents::NetworkConnectionPropertyChange.
STDMETHODIMP NlmEventSink::NetworkConnectionPropertyChanged(GUID networkId, NLM_CONNECTION_PROPERTY_CHANGE flags)
{
#if DEBUG_NLM_EVENTS
    LOG_INFO("NLM EVENT: NetworkConnection property change: %s 0x"FMTx32" (%s)",
            WinGuidToStr(networkId).c_str(), flags, nlmConnectionPropertyChangeStr(flags).c_str());
#else
    LOG_INFO("NLM EVENT: NetworkConnection property change");
#endif

    processNetworkChange();
    ANSConn_ReportNetworkConnected();
    return S_OK;
}

//-------------------------------------------------------
