//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <string>

using namespace dx_remote_agent;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
    InitializeComponent();

    //create_picstream_path();
    //clean_cc();
    //startccd();

    myListener.RunDetached();

    this->txtHostName->Text = L"HostName(ComputerName): ";
    this->txtPort->Text = L"Port: 24000";
    this->txtIPv4->Text = L"IPv4: \n";
    this->txtIPv6->Text = L"IPv6: \n";

    auto hostnames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();
    for (unsigned int i = 0; i < hostnames->Size; ++i)
    {    
        Platform::String ^adapter = L"\tAdapterID: ";
        Platform::String ^networkid = L"\tNetworkID: ";
        if (hostnames->GetAt(i)->IPInformation != nullptr && hostnames->GetAt(i)->IPInformation->NetworkAdapter != nullptr) {
            adapter += hostnames->GetAt(i)->IPInformation->NetworkAdapter->NetworkAdapterId.ToString();
            adapter += L"\n\t IanaInterfaceType: ";
            std::wstring wstrIanaType = GetIFTypeString((IF_TYPE)hostnames->GetAt(i)->IPInformation->NetworkAdapter->IanaInterfaceType);
            adapter += ref new Platform::String(wstrIanaType.c_str());

            adapter += L" (";
            adapter += hostnames->GetAt(i)->IPInformation->NetworkAdapter->IanaInterfaceType.ToString();
            adapter += L")";

            if (hostnames->GetAt(i)->IPInformation->NetworkAdapter->NetworkItem != nullptr) {
                networkid += hostnames->GetAt(i)->IPInformation->NetworkAdapter->NetworkItem->NetworkId.ToString();
                networkid += L"\n";
            }
        }

        if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::DomainName) {
            Platform::String ^psHostName = hostnames->GetAt(i)->DisplayName;
            std::wstring wstrHostName = std::wstring(psHostName->Begin(), psHostName->End());
            if (wstrHostName.find_last_of(L".local") == std::wstring::npos && wstrHostName.find_last_of(L".local") != wstrHostName.length() - 6) {
                this->txtHostName->Text += hostnames->GetAt(i)->DisplayName;
            }
        }
        else if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::Ipv4) {
            this->txtIPv4->Text += (L"\tAddress: " + hostnames->GetAt(i)->DisplayName + L"\n");
            this->txtIPv4->Text += (adapter + L"\n");
            this->txtIPv4->Text += (networkid + L"\n");
        }
        else if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::Ipv6) {
            this->txtIPv6->Text += (L"\tAddress: " + hostnames->GetAt(i)->DisplayName + L"\n");
            this->txtIPv6->Text += (adapter + L"\n");
            this->txtIPv6->Text += (networkid + L"\n");
        }

        
    }
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
    (void) e;    // Unused parameter
}

std::wstring MainPage::GetIFTypeString(IF_TYPE type)
{
    switch (type)
    {
    case IF_TYPE_OTHER:
        return std::wstring(L"IF_TYPE_OTHER");
        break;
    case IF_TYPE_REGULAR_1822:
        return std::wstring(L"IF_TYPE_REGULAR_1822");
        break;
    case IF_TYPE_HDH_1822:
        return std::wstring(L"IF_TYPE_HDH_1822");
        break;
    case IF_TYPE_DDN_X25:
        return std::wstring(L"IF_TYPE_DDN_X25");
        break;
    case IF_TYPE_RFC877_X25:
        return std::wstring(L"IF_TYPE_RFC877_X25");
        break;
    case IF_TYPE_ETHERNET_CSMACD:
        return std::wstring(L"IF_TYPE_ETHERNET_CSMACD");
        break;
    case IF_TYPE_IS088023_CSMACD:
        return std::wstring(L"IF_TYPE_IS088023_CSMACD");
        break;
    case IF_TYPE_ISO88024_TOKENBUS:
        return std::wstring(L"IF_TYPE_ISO88024_TOKENBUS");
        break;
    case IF_TYPE_ISO88025_TOKENRING:
        return std::wstring(L"IF_TYPE_ISO88025_TOKENRING");
        break;
    case IF_TYPE_ISO88026_MAN:
        return std::wstring(L"IF_TYPE_ISO88026_MAN");
        break;
    case IF_TYPE_STARLAN:
        return std::wstring(L"IF_TYPE_STARLAN");
        break;
    case IF_TYPE_PROTEON_10MBIT:
        return std::wstring(L"IF_TYPE_PROTEON_10MBIT");
        break;
    case IF_TYPE_PROTEON_80MBIT:
        return std::wstring(L"IF_TYPE_PROTEON_80MBIT");
        break;
    case IF_TYPE_HYPERCHANNEL:
        return std::wstring(L"IF_TYPE_HYPERCHANNEL");
        break;
    case IF_TYPE_FDDI:
        return std::wstring(L"IF_TYPE_FDDI");
        break;
    case IF_TYPE_LAP_B:
        return std::wstring(L"IF_TYPE_LAP_B");
        break;
    case IF_TYPE_SDLC:
        return std::wstring(L"IF_TYPE_SDLC");
        break;
    case IF_TYPE_DS1:
        return std::wstring(L"IF_TYPE_DS1");
        break;
    case IF_TYPE_E1:
        return std::wstring(L"IF_TYPE_E1");
        break;
    case IF_TYPE_BASIC_ISDN:
        return std::wstring(L"IF_TYPE_BASIC_ISDN");
        break;
    case IF_TYPE_PRIMARY_ISDN:
        return std::wstring(L"IF_TYPE_PRIMARY_ISDN");
        break;
    case IF_TYPE_PROP_POINT2POINT_SERIAL:
        return std::wstring(L"IF_TYPE_PROP_POINT2POINT_SERIAL");
        break;
    case IF_TYPE_PPP:
        return std::wstring(L"IF_TYPE_PPP");
        break;
    case IF_TYPE_SOFTWARE_LOOPBACK:
        return std::wstring(L"IF_TYPE_SOFTWARE_LOOPBACK");
        break;
    case IF_TYPE_EON:
        return std::wstring(L"IF_TYPE_EON");
        break;
    case IF_TYPE_ETHERNET_3MBIT:
        return std::wstring(L"IF_TYPE_ETHERNET_3MBIT");
        break;
    case IF_TYPE_NSIP:
        return std::wstring(L"IF_TYPE_NSIP");
        break;
    case IF_TYPE_SLIP:
        return std::wstring(L"IF_TYPE_SLIP");
        break;
    case IF_TYPE_ULTRA:
        return std::wstring(L"IF_TYPE_ULTRA");
        break;
    case IF_TYPE_DS3:
        return std::wstring(L"IF_TYPE_DS3");
        break;
    case IF_TYPE_SIP:
        return std::wstring(L"IF_TYPE_SIP");
        break;
    case IF_TYPE_FRAMERELAY:
        return std::wstring(L"IF_TYPE_FRAMERELAY");
        break;
    case IF_TYPE_RS232:
        return std::wstring(L"IF_TYPE_RS232");
        break;
    case IF_TYPE_PARA:
        return std::wstring(L"IF_TYPE_PARA");
        break;
    case IF_TYPE_ARCNET:
        return std::wstring(L"IF_TYPE_ARCNET");
        break;
    case IF_TYPE_ARCNET_PLUS:
        return std::wstring(L"IF_TYPE_ARCNET_PLUS");
        break;
    case IF_TYPE_ATM:
        return std::wstring(L"IF_TYPE_ATM");
        break;
    case IF_TYPE_MIO_X25:
        return std::wstring(L"IF_TYPE_MIO_X25");
        break;
    case IF_TYPE_SONET:
        return std::wstring(L"IF_TYPE_SONET");
        break;
    case IF_TYPE_X25_PLE:
        return std::wstring(L"IF_TYPE_X25_PLE");
        break;
    case IF_TYPE_ISO88022_LLC:
        return std::wstring(L"IF_TYPE_ISO88022_LLC");
        break;
    case IF_TYPE_LOCALTALK:
        return std::wstring(L"IF_TYPE_LOCALTALK");
        break;
    case IF_TYPE_SMDS_DXI:
        return std::wstring(L"IF_TYPE_SMDS_DXI");
        break;
    case IF_TYPE_FRAMERELAY_SERVICE:
        return std::wstring(L"IF_TYPE_FRAMERELAY_SERVICE");
        break;
    case IF_TYPE_V35:
        return std::wstring(L"IF_TYPE_V35");
        break;
    case IF_TYPE_HSSI:
        return std::wstring(L"IF_TYPE_HSSI");
        break;
    case IF_TYPE_HIPPI:
        return std::wstring(L"IF_TYPE_HIPPI");
        break;
    case IF_TYPE_MODEM:
        return std::wstring(L"IF_TYPE_MODEM");
        break;
    case IF_TYPE_AAL5:
        return std::wstring(L"IF_TYPE_AAL5");
        break;
    case IF_TYPE_SONET_PATH:
        return std::wstring(L"IF_TYPE_SONET_PATH");
        break;
    case IF_TYPE_SONET_VT:
        return std::wstring(L"IF_TYPE_SONET_VT");
        break;
    case IF_TYPE_SMDS_ICIP:
        return std::wstring(L"IF_TYPE_SMDS_ICIP");
        break;
    case IF_TYPE_PROP_VIRTUAL:
        return std::wstring(L"IF_TYPE_PROP_VIRTUAL");
        break;
    case IF_TYPE_PROP_MULTIPLEXOR:
        return std::wstring(L"IF_TYPE_PROP_MULTIPLEXOR");
        break;
    case IF_TYPE_IEEE80212:
        return std::wstring(L"IF_TYPE_IEEE80212");
        break;
    case IF_TYPE_FIBRECHANNEL:
        return std::wstring(L"IF_TYPE_FIBRECHANNEL");
        break;
    case IF_TYPE_HIPPIINTERFACE:
        return std::wstring(L"IF_TYPE_HIPPIINTERFACE");
        break;
    case IF_TYPE_FRAMERELAY_INTERCONNECT:
        return std::wstring(L"IF_TYPE_FRAMERELAY_INTERCONNECT");
        break;
    case IF_TYPE_AFLANE_8023:
        return std::wstring(L"IF_TYPE_AFLANE_8023");
        break;
    case IF_TYPE_AFLANE_8025:
        return std::wstring(L"IF_TYPE_AFLANE_8025");
        break;
    case IF_TYPE_CCTEMUL:
        return std::wstring(L"IF_TYPE_CCTEMUL");
        break;
    case IF_TYPE_FASTETHER:
        return std::wstring(L"IF_TYPE_FASTETHER");
        break;
    case IF_TYPE_ISDN:
        return std::wstring(L"IF_TYPE_ISDN");
        break;
    case IF_TYPE_V11:
        return std::wstring(L"IF_TYPE_V11");
        break;
    case IF_TYPE_V36:
        return std::wstring(L"IF_TYPE_V36");
        break;
    case IF_TYPE_G703_64K:
        return std::wstring(L"IF_TYPE_G703_64K");
        break;
    case IF_TYPE_G703_2MB:
        return std::wstring(L"IF_TYPE_G703_2MB");
        break;
    case IF_TYPE_QLLC:
        return std::wstring(L"IF_TYPE_QLLC");
        break;
    case IF_TYPE_FASTETHER_FX:
        return std::wstring(L"IF_TYPE_FASTETHER_FX");
        break;
    case IF_TYPE_CHANNEL:
        return std::wstring(L"IF_TYPE_CHANNEL");
        break;
    case IF_TYPE_IEEE80211:
        return std::wstring(L"IF_TYPE_IEEE80211");
        break;
    case IF_TYPE_IBM370PARCHAN:
        return std::wstring(L"IF_TYPE_IBM370PARCHAN");
        break;
    case IF_TYPE_ESCON:
        return std::wstring(L"IF_TYPE_ESCON");
        break;
    case IF_TYPE_DLSW:
        return std::wstring(L"IF_TYPE_DLSW");
        break;
    case IF_TYPE_ISDN_S:
        return std::wstring(L"IF_TYPE_ISDN_S");
        break;
    case IF_TYPE_ISDN_U:
        return std::wstring(L"IF_TYPE_ISDN_U");
        break;
    case IF_TYPE_LAP_D:
        return std::wstring(L"IF_TYPE_LAP_D");
        break;
    case IF_TYPE_IPSWITCH:
        return std::wstring(L"IF_TYPE_IPSWITCH");
        break;
    case IF_TYPE_RSRB:
        return std::wstring(L"IF_TYPE_RSRB");
        break;
    case IF_TYPE_ATM_LOGICAL:
        return std::wstring(L"IF_TYPE_ATM_LOGICAL");
        break;
    case IF_TYPE_DS0:
        return std::wstring(L"IF_TYPE_DS0");
        break;
    case IF_TYPE_DS0_BUNDLE:
        return std::wstring(L"IF_TYPE_DS0_BUNDLE");
        break;
    case IF_TYPE_BSC:
        return std::wstring(L"IF_TYPE_BSC");
        break;
    case IF_TYPE_ASYNC:
        return std::wstring(L"IF_TYPE_ASYNC");
        break;
    case IF_TYPE_CNR:
        return std::wstring(L"IF_TYPE_CNR");
        break;
    case IF_TYPE_ISO88025R_DTR:
        return std::wstring(L"IF_TYPE_ISO88025R_DTR");
        break;
    case IF_TYPE_EPLRS:
        return std::wstring(L"IF_TYPE_EPLRS");
        break;
    case IF_TYPE_ARAP:
        return std::wstring(L"IF_TYPE_ARAP");
        break;
    case IF_TYPE_PROP_CNLS:
        return std::wstring(L"IF_TYPE_PROP_CNLS");
        break;
    case IF_TYPE_HOSTPAD:
        return std::wstring(L"IF_TYPE_HOSTPAD");
        break;
    case IF_TYPE_TERMPAD:
        return std::wstring(L"IF_TYPE_TERMPAD");
        break;
    case IF_TYPE_FRAMERELAY_MPI:
        return std::wstring(L"IF_TYPE_FRAMERELAY_MPI");
        break;
    case IF_TYPE_X213:
        return std::wstring(L"IF_TYPE_X213");
        break;
    case IF_TYPE_ADSL:
        return std::wstring(L"IF_TYPE_ADSL");
        break;
    case IF_TYPE_RADSL:
        return std::wstring(L"IF_TYPE_RADSL");
        break;
    case IF_TYPE_SDSL:
        return std::wstring(L"IF_TYPE_SDSL");
        break;
    case IF_TYPE_VDSL:
        return std::wstring(L"IF_TYPE_VDSL");
        break;
    case IF_TYPE_ISO88025_CRFPRINT:
        return std::wstring(L"IF_TYPE_ISO88025_CRFPRINT");
        break;
    case IF_TYPE_MYRINET:
        return std::wstring(L"IF_TYPE_MYRINET");
        break;
    case IF_TYPE_VOICE_EM:
        return std::wstring(L"IF_TYPE_VOICE_EM");
        break;
    case IF_TYPE_VOICE_FXO:
        return std::wstring(L"IF_TYPE_VOICE_FXO");
        break;
    case IF_TYPE_VOICE_FXS:
        return std::wstring(L"IF_TYPE_VOICE_FXS");
        break;
    case IF_TYPE_VOICE_ENCAP:
        return std::wstring(L"IF_TYPE_VOICE_ENCAP");
        break;
    case IF_TYPE_VOICE_OVERIP:
        return std::wstring(L"IF_TYPE_VOICE_OVERIP");
        break;
    case IF_TYPE_ATM_DXI:
        return std::wstring(L"IF_TYPE_ATM_DXI");
        break;
    case IF_TYPE_ATM_FUNI:
        return std::wstring(L"IF_TYPE_ATM_FUNI");
        break;
    case IF_TYPE_ATM_IMA:
        return std::wstring(L"IF_TYPE_ATM_IMA");
        break;
    case IF_TYPE_PPPMULTILINKBUNDLE:
        return std::wstring(L"IF_TYPE_PPPMULTILINKBUNDLE");
        break;
    case IF_TYPE_IPOVER_CDLC:
        return std::wstring(L"IF_TYPE_IPOVER_CDLC");
        break;
    case IF_TYPE_IPOVER_CLAW:
        return std::wstring(L"IF_TYPE_IPOVER_CLAW");
        break;
    case IF_TYPE_STACKTOSTACK:
        return std::wstring(L"IF_TYPE_STACKTOSTACK");
        break;
    case IF_TYPE_VIRTUALIPADDRESS:
        return std::wstring(L"IF_TYPE_VIRTUALIPADDRESS");
        break;
    case IF_TYPE_MPC:
        return std::wstring(L"IF_TYPE_MPC");
        break;
    case IF_TYPE_IPOVER_ATM:
        return std::wstring(L"IF_TYPE_IPOVER_ATM");
        break;
    case IF_TYPE_ISO88025_FIBER:
        return std::wstring(L"IF_TYPE_ISO88025_FIBER");
        break;
    case IF_TYPE_TDLC:
        return std::wstring(L"IF_TYPE_TDLC");
        break;
    case IF_TYPE_GIGABITETHERNET:
        return std::wstring(L"IF_TYPE_GIGABITETHERNET");
        break;
    case IF_TYPE_HDLC:
        return std::wstring(L"IF_TYPE_HDLC");
        break;
    case IF_TYPE_LAP_F:
        return std::wstring(L"IF_TYPE_LAP_F");
        break;
    case IF_TYPE_V37:
        return std::wstring(L"IF_TYPE_V37");
        break;
    case IF_TYPE_X25_MLP:
        return std::wstring(L"IF_TYPE_X25_MLP");
        break;
    case IF_TYPE_X25_HUNTGROUP:
        return std::wstring(L"IF_TYPE_X25_HUNTGROUP");
        break;
    case IF_TYPE_TRANSPHDLC:
        return std::wstring(L"IF_TYPE_TRANSPHDLC");
        break;
    case IF_TYPE_INTERLEAVE:
        return std::wstring(L"IF_TYPE_INTERLEAVE");
        break;
    case IF_TYPE_FAST:
        return std::wstring(L"IF_TYPE_FAST");
        break;
    case IF_TYPE_IP:
        return std::wstring(L"IF_TYPE_IP");
        break;
    case IF_TYPE_DOCSCABLE_MACLAYER:
        return std::wstring(L"IF_TYPE_DOCSCABLE_MACLAYER");
        break;
    case IF_TYPE_DOCSCABLE_DOWNSTREAM:
        return std::wstring(L"IF_TYPE_DOCSCABLE_DOWNSTREAM");
        break;
    case IF_TYPE_DOCSCABLE_UPSTREAM:
        return std::wstring(L"IF_TYPE_DOCSCABLE_UPSTREAM");
        break;
    case IF_TYPE_A12MPPSWITCH:
        return std::wstring(L"IF_TYPE_A12MPPSWITCH");
        break;
    case IF_TYPE_TUNNEL:
        return std::wstring(L"IF_TYPE_TUNNEL");
        break;
    case IF_TYPE_COFFEE:
        return std::wstring(L"IF_TYPE_COFFEE");
        break;
    case IF_TYPE_CES:
        return std::wstring(L"IF_TYPE_CES");
        break;
    case IF_TYPE_ATM_SUBINTERFACE:
        return std::wstring(L"IF_TYPE_ATM_SUBINTERFACE");
        break;
    case IF_TYPE_L2_VLAN:
        return std::wstring(L"IF_TYPE_L2_VLAN");
        break;
    case IF_TYPE_L3_IPVLAN:
        return std::wstring(L"IF_TYPE_L3_IPVLAN");
        break;
    case IF_TYPE_L3_IPXVLAN:
        return std::wstring(L"IF_TYPE_L3_IPXVLAN");
        break;
    case IF_TYPE_DIGITALPOWERLINE:
        return std::wstring(L"IF_TYPE_DIGITALPOWERLINE");
        break;
    case IF_TYPE_MEDIAMAILOVERIP:
        return std::wstring(L"IF_TYPE_MEDIAMAILOVERIP");
        break;
    case IF_TYPE_DTM:
        return std::wstring(L"IF_TYPE_DTM");
        break;
    case IF_TYPE_DCN:
        return std::wstring(L"IF_TYPE_DCN");
        break;
    case IF_TYPE_IPFORWARD:
        return std::wstring(L"IF_TYPE_IPFORWARD");
        break;
    case IF_TYPE_MSDSL:
        return std::wstring(L"IF_TYPE_MSDSL");
        break;
    case IF_TYPE_IEEE1394:
        return std::wstring(L"IF_TYPE_IEEE1394");
        break;
    case IF_TYPE_RECEIVE_ONLY:
        return std::wstring(L"IF_TYPE_RECEIVE_ONLY");
        break;
    case IF_TYPE_WWANPP:
        return std::wstring(L"IF_TYPE_WWANPP");
        break;
    case IF_TYPE_WWANPP2:
        return std::wstring(L"IF_TYPE_WWANPP2");
        break;
    default:
        return std::wstring(L"UNKNOWN");
        break;
    }
}
