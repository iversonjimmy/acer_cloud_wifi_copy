#!/usr/bin/perl -w

use Ias;
use Vsds;
use Getopt::Long;
use Data::Dumper;

if (@ARGV < 2) {
    print "Usage: $0 [--domain domain] [--no-din][--no-dex][--no-p2p] username password\n";
    exit 0;
}

#my $debug = 1;

my $router0_public_ip = "10.50.3.8";
my $router0_public_dev = "eth0";
my $router1_public_ip = "10.50.3.6";
my $router1_public_dev = "eth1";
my $router_cloudpc_link_ip = "192.168.1.1";
#ROUTER_CLOUDPC_LINK_DEV=eth1
my $router_client_link_ip = "192.168.2.1";
#ROUTER_CLIENT_LINK_DEV=eth2

my $cloudpc_net = "192.168.1.0/24";
my $cloudpc_gw = "192.168.1.1";
my $cloudpc_ip = "192.168.1.2";

my $client_net = "192.168.2.0/24";
my $client_gw = "192.168.2.1";
my $client_ip = "192.168.2.2";


my $domain = 'pc-int.igware.net';
my $no_din = 0;
my $no_dex = 0;
my $no_p2p = 0;
GetOptions('domain' => \$domain, 'no-din' => \$no_din, 'no-dex' => \$no_dex, 'no-p2p' => \$no_p2p);

my $username = shift @ARGV;
my $password = shift @ARGV;

my $version  = '2.0';

my $ias = Ias->new({ 'domain' => $domain });
my $loginOutput = $ias->login({ 'username' => $username,
                                'password' => $password });
($loginOutput->{errorCode} == 0) or die "IAS.Login failed: errorCode=$loginOutput->{errorCode}";

my $serviceTicket = Vsds::getServiceTicket({ 'sessionSecret' => $loginOutput->{sessionSecret} });

my $vsds = Vsds->new({'domain' => $domain});
my $getLinkedDevicesOutput = $vsds->getLinkedDevices({ 'userId'           => $loginOutput->{userId},
						       'sessionHandle'    => $loginOutput->{sessionHandle},
                                                       'serviceTicket'    => $serviceTicket,
                                                       'version'          => $version });
($getLinkedDevicesOutput->{errorCode} == 0) or die "VSDS.GetLinkedDevices failed: errorCode $getLinkedDevicesOutput->{errorCode}";

my $devices = $getLinkedDevicesOutput->{devices};
my $numOfDevices = @$devices;
#print "$numOfDevices device(s)\n";

my $cloudpc;
foreach my $dev (@$devices) {
    if (defined($dev->{featureMediaServerCapable}) and $dev->{featureMediaServerCapable}) {
	$cloudpc = $dev;
	#print "DEBUG: found cloudpc: devId $dev->{deviceId}\n"
    }
}
defined($cloudpc) or die "ERROR: no cloudpc found\n";

my $getUserStorageAddressOutput = $vsds->getUserStorageAddress({ 'userId'           => $loginOutput->{userId},
								 'storageClusterId' => $cloudpc->{deviceId},
								 'sessionHandle'    => $loginOutput->{sessionHandle},
								 'serviceTicket'    => $serviceTicket,
								 'version'          => $version });
($getUserStorageAddressOutput->{errorCode} == 0) or die "VSDS.GetUserStorageAddress failed: errorCode $getUserStorageAddressOutput->{errorCode}";

my $userStorageAddress = $getUserStorageAddressOutput->{userStorageAddress};

#print Dumper($userStorageAddress);
# sample output
# $VAR1 = {
#           'accessTicket' => '2m1KWv02djN99hHr6nHoQcak1lA=',
#           'directSecurePort' => '49278',
#           'proxyAddress' => 'vss-c100.pc-int.igware.net',
#           'directAddress' => '10.0.3.164',
#           'internalDirectAddress' => '192.168.48.18',
#           'proxyPort' => '16957',
#           'accessHandle' => '2032191245095117226'
# };

# $userStorageAddress->{directAddress} contains the DEX address
# $userStorageAddress->{directSecurePort} contains the DEX port

#print "CloudPC DEX at $userStorageAddress->{directAddress}:$userStorageAddress->{directSecurePort}\n";
my $cloudpc_dex_port = $userStorageAddress->{directSecurePort};



# flush user chains
system "iptables -F din";
system "iptables -t nat -F dex";


# DEX and/or P2P connections from client to cloudpc
if ($no_dex && $no_p2p) {
    # no need to set up port forwarding
} elsif ($no_dex && ! $no_p2p) {
    system "iptables -t nat -A dex -p tcp ! --dport $cloudpc_dex_port -j DNAT --to-destination $cloudpc_ip";
} elsif (! $no_dex && $no_p2p) {
    system "iptables -t nat -A dex -p tcp --dport $cloudpc_dex_port -j DNAT --to-destination $cloudpc_ip";
} else {  # ! $no_dex && ! $no_p2p
    system "iptables -t nat -A dex -p tcp -j DNAT --to-destination $cloudpc_ip";
}


# DIN connection from client to cloudpc
# To specifically disable DIN, we check the ctstate for DNAT
if ($no_din) {
    system "iptables -A din -m conntrack ! --ctstate DNAT -j DROP";
    system "iptables -A din -m conntrack ! --ctstate DNAT -j DROP";
}
