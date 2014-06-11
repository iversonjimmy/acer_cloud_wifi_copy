package Vsds;

use URI;
use LWP 5.66;
use Digest::SHA;
use MIME::Base64;
use HTTP::Request;
use XML::Simple;

#my $debug = 1;
my $debug = 0;

# Input hashref:
#  domain : lab domain, defaults to 'pc-int.igware.net'
sub new {
    my ($type, $args) = @_;
    my $obj = {};
    bless $obj, $type;
    $obj->{domain} = defined($args->{domain}) ? $args->{domain} : 'pc-int.igware.net';
    return $obj;
}

sub _getUserAgent {
    my ($self, $args) = @_;

    if (!defined($self->{userAgent})) {
        $self->{userAgent} = LWP::UserAgent->new;
    }
    return $self->{userAgent};
}

# Input Params:
#  userId		: userId, as obtained from IAS
#  sessionHandle	: sessionHandle, as obtained from IAS
#  serviceTicket	: serviceTicket, generated from sessionSecret obtained from IAS
#  version		: API version number
# Output params:
#  errorCode
#  datasets	: ref to array of hash refs
sub listOwnedDataSets {
    my ($self, $args) = @_;

    defined($args->{userId}) or die "No userId in args\n";
    defined($args->{sessionHandle}) or die "No sessionHandle in args\n";
    defined($args->{serviceTicket}) or die "No serviceTicket in args\n";
    defined($args->{version}) or die "No version in args\n";

    my $httpReqBody = '<?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><ListOwnedDataSetsInput xmlns="urn:vsds.wsapi.broadon.com"><session><sessionHandle>'.$args->{sessionHandle}.'</sessionHandle><serviceTicket>'.$args->{serviceTicket}.'</serviceTicket></session><userId>'.$args->{userId}.'</userId><version>'.$args->{version}.'</version></ListOwnedDataSetsInput></soapenv:Body></soapenv:Envelope>';

    my $ua = $self->_getUserAgent();
    my $httpReq = HTTP::Request->new('POST',
                                     "https://www.$self->{domain}/vsds/services/VirtualStorageDirectoryServicesSOAP",
                                     [ 'SOAPAction' => '"urn:vsds.wsapi.broadon.com/ListOwnedDataSets"',
                                     ],
                                     $httpReqBody);
    my $httpRes = $ua->request($httpReq);
    $httpRes->is_success or die "FATAL: failed to POST VSDS.ListOwnedDataSets: " . $httpRes->status_line . "\n";

# sample response
#<?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><ListOwnedDataSetsOutput xmlns="urn:vsds.wsapi.broadon.com"><error><errorCode>0</errorCode></error><datasets><datasetId>8382584</datasetId><datasetName>My Cloud</datasetName><contentType>USERDATA</contentType><lastUpdated>1353673838000</lastUpdated><storageClusterName>My Cloud</storageClusterName><storageClusterHostName>vss-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>17144</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;00000000007FE878&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;vss-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;16957&lt;/port&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>USER</datasetType><clusterId>100</clusterId><userId>1330037</userId><primaryStorageId>3</primaryStorageId></datasets><datasets><datasetId>8382589</datasetId><datasetName>Contacts</datasetName><contentType>USERDATA</contentType><lastUpdated>1337933047000</lastUpdated><storageClusterName>My Cloud</storageClusterName><storageClusterHostName>vss-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>17144</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;00000000007FE87D&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;vss-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;16957&lt;/port&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>PIM_CONTACTS</datasetType><clusterId>100</clusterId><userId>1330037</userId><primaryStorageId>3</primaryStorageId></datasets><datasets><datasetId>15970209</datasetId><datasetName>Media MetaData</datasetName><contentType>USERDATA</contentType><lastUpdated>1348709774000</lastUpdated><storageClusterName>My Cloud</storageClusterName><storageClusterHostName>vss-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>17144</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;0000000000F3AFA1&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;vss-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;16957&lt;/port&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>USER_CONTENT_METADATA</datasetType><clusterId>100</clusterId><userId>1330037</userId><primaryStorageId>3</primaryStorageId></datasets><datasets><datasetId>15970210</datasetId><datasetName>Cloud Doc</datasetName><contentType>USERDATA</contentType><lastUpdated>1353702675000</lastUpdated><storageClusterName>My Cloud</storageClusterName><storageClusterHostName>www-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>443</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;0000000000F3AFA2&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VCS&lt;/protocol&gt;&lt;server&gt;www-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;443&lt;/port&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>CACHE</datasetType><clusterId>100</clusterId><userId>1330037</userId><primaryStorageId>26</primaryStorageId></datasets><datasets><datasetId>15970213</datasetId><datasetName>Music</datasetName><contentType>USERDATA</contentType><lastUpdated>1348183695000</lastUpdated><storageClusterName>PersonalStorageNode-6934B9872</storageClusterName><storageClusterHostName>vss-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>17144</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;0000000000F3AFA5&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;10.0.3.94&lt;/server&gt;&lt;port&gt;46830&lt;/port&gt;&lt;/route&gt;&lt;route&gt;&lt;routetype&gt;proxy&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;vss-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;16957&lt;/port&gt;&lt;clusterid&gt;00000006934B9872&lt;/clusterid&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>USER</datasetType><clusterId>28241008754</clusterId><userId>1330037</userId><primaryStorageId>0</primaryStorageId></datasets><datasets><datasetId>15970215</datasetId><datasetName>Device Storage</datasetName><contentType>USERDATA</contentType><lastUpdated>1347781550000</lastUpdated><storageClusterName>PersonalStorageNode-6934B9872</storageClusterName><storageClusterHostName>vss-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>17144</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;0000000000F3AFA7&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;10.0.3.94&lt;/server&gt;&lt;port&gt;46830&lt;/port&gt;&lt;/route&gt;&lt;route&gt;&lt;routetype&gt;proxy&lt;/routetype&gt;&lt;protocol&gt;VSS&lt;/protocol&gt;&lt;server&gt;vss-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;16957&lt;/port&gt;&lt;clusterid&gt;00000006934B9872&lt;/clusterid&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>FS</datasetType><clusterId>28241008754</clusterId><userId>1330037</userId><primaryStorageId>0</primaryStorageId></datasets><datasets><datasetId>19794360</datasetId><datasetName>PicStream</datasetName><contentType>USERDATA</contentType><lastUpdated>1353674144000</lastUpdated><storageClusterName>My Cloud</storageClusterName><storageClusterHostName>www-c100.pc-int.igware.net</storageClusterHostName><storageClusterPort>443</storageClusterPort><datasetLocation>&lt;object format=&quot;dataset&quot;&gt;&lt;uid&gt;0000000000144B75&lt;/uid&gt;&lt;did&gt;00000000012E09B8&lt;/did&gt;&lt;routes&gt;&lt;route&gt;&lt;routetype&gt;direct&lt;/routetype&gt;&lt;protocol&gt;VCS&lt;/protocol&gt;&lt;server&gt;www-c100.pc-int.igware.net&lt;/server&gt;&lt;port&gt;443&lt;/port&gt;&lt;/route&gt;&lt;/routes&gt;&lt;/object&gt;</datasetLocation><datasetType>CACHE</datasetType><clusterId>100</clusterId><userId>1330037</userId><primaryStorageId>26</primaryStorageId></datasets></ListOwnedDataSetsOutput></soapenv:Body></soapenv:Envelope>

    my $xs = XML::Simple->new(ForceArray => ['datasets']);
    my $httpResXml = $xs->XMLin($httpRes->content);
    defined($httpResXml->{'soapenv:Body'}) or die "No soapenv:Body element in HTTP response $httpRes->{content}";
    defined($httpResXml->{'soapenv:Body'}->{ListOwnedDataSetsOutput}) or die "No ListOwnedDataSetsOutput element in HTTP response $httpRes->{content}";
    my $listOwnedDataSetsOutput = $httpResXml->{'soapenv:Body'}->{ListOwnedDataSetsOutput};

    defined($listOwnedDataSetsOutput->{error}->{errorCode}) or die "No ErrorCode element in HTTP response $httpRes->{content}";
    
    my $res = {};
    $res->{errorCode} = $listOwnedDataSetsOutput->{error}->{errorCode};
    $res->{datasets} = $listOwnedDataSetsOutput->{datasets};

    return $res;
}

sub getDatasetDetail {
    my ($self, $args) = @_;

    defined($args->{userId})		or die "No userId in args\n";
    defined($args->{sessionHandle})	or die "No sessionHandle in args\n";
    defined($args->{serviceTicket})	or die "No serviceTicket in args\n";
    defined($args->{version})		or die "No version in args\n";
    defined($args->{datasetSpec})	or die "No datasetSpec in args";

    my $listOwnedDataSetsOutput = $self->listOwnedDataSets($args);
    ($listOwnedDataSetsOutput->{errorCode} == 0) or die "VSDS.ListOwnedDataSets failed: errorCode $listOwnedDataSetsOutput->{errorCode}";

    my $datasets = $listOwnedDataSetsOutput->{datasets};
    foreach (@$datasets) {
        if (($_->{datasetName} eq $args->{datasetSpec}) or
            ($_->{datasetId} eq $args->{datasetSpec})) {
            return $_;
        }
    }
    return undef;
}

sub getServiceTicket {
    my ($args) = @_;

    defined($args->{sessionSecret}) or die "No sessionSecret in args";

# vs service ticket is sha1(base64(sessionSecret) || "Virtual Storage")
    return encode_base64(Digest::SHA::sha1($args->{sessionSecret} . "Virtual Storage"), '');  # '' for no EOL in output
}

# Input Params:
#  userId		: userId, as obtained from IAS
#  storageClusterId     : 
#  sessionHandle	: sessionHandle, as obtained from IAS
#  serviceTicket	: serviceTicket, generated from sessionSecret obtained from IAS
#  version		: API version number
sub getUserStorageAddress {
    my ($self, $args) = @_;

    defined($args->{userId}) or die "No userId in args\n";
    defined($args->{storageClusterId}) or die "No storageClusterId in args\n";
    defined($args->{sessionHandle}) or die "No sessionHandle in args\n";
    defined($args->{serviceTicket}) or die "No serviceTicket in args\n";
    defined($args->{version}) or die "No version in args\n";

    my $httpReqBody = '<?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><GetUserStorageAddressInput xmlns="urn:vsds.wsapi.broadon.com"><session><sessionHandle>'.$args->{sessionHandle}.'</sessionHandle><serviceTicket>'.$args->{serviceTicket}.'</serviceTicket></session><userId>'.$args->{userId}.'</userId><storageClusterId>'.$args->{storageClusterId}.'</storageClusterId></GetUserStorageAddressInput></soapenv:Body></soapenv:Envelope>';

    my $ua = $self->_getUserAgent();
    my $httpReq = HTTP::Request->new('POST',
                                     "https://www.$self->{domain}/vsds/services/VirtualStorageDirectoryServicesSOAP",
                                     [ 'SOAPAction' => '"urn:vsds.wsapi.broadon.com/GetUserStorageAddress"',
                                     ],
                                     $httpReqBody);
    my $httpRes = $ua->request($httpReq);
    $httpRes->is_success or die "FATAL: failed to POST VSDS.GetUserStorageAddress: " . $httpRes->status_line . "\n";

# sample response
# <?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><GetUserStorageAddressOutput xmlns="urn:vsds.wsapi.broadon.com"><error><errorCode>0</errorCode></error><directAddress>10.0.3.20</directAddress><proxyAddress>vss-c100.pc-int.igware.net</proxyAddress><proxyPort>16957</proxyPort><internalDirectAddress>192.168.48.18</internalDirectAddress><directSecurePort>39088</directSecurePort><accessHandle>2032191245095117226</accessHandle><accessTicket>2m1KWv02djN99hHr6nHoQcak1lA=</accessTicket></GetUserStorageAddressOutput></soapenv:Body></soapenv:Envelope>

    my $xs = XML::Simple->new;
    my $httpResXml = $xs->XMLin($httpRes->content);
    defined($httpResXml->{'soapenv:Body'}) or die "No soapenv:Body element in HTTP response $httpRes->content";
    defined($httpResXml->{'soapenv:Body'}->{GetUserStorageAddressOutput}) or die "No GetUserStorageAddressOutput element in HTTP response $httpRes->content";
    my $getUserStorageAddressOutput = $httpResXml->{'soapenv:Body'}->{GetUserStorageAddressOutput};

    defined($getUserStorageAddressOutput->{error}->{errorCode}) or die "No ErrorCode element in HTTP response $httpRes->content";
    
    my $res = {};
    $res->{errorCode} = $getUserStorageAddressOutput->{error}->{errorCode};
    $res->{userStorageAddress} = {};
    if (defined($getUserStorageAddressOutput->{directAddress})) {
	$res->{userStorageAddress}->{directAddress} = $getUserStorageAddressOutput->{directAddress};
    }
    if (defined($getUserStorageAddressOutput->{directPort})) {
	$res->{userStorageAddress}->{directPort} = $getUserStorageAddressOutput->{directPort};
    }
    if (defined($getUserStorageAddressOutput->{proxyAddress})) {
	$res->{userStorageAddress}->{proxyAddress} = $getUserStorageAddressOutput->{proxyAddress};
    }
    if (defined($getUserStorageAddressOutput->{proxyPort})) {
	$res->{userStorageAddress}->{proxyPort} = $getUserStorageAddressOutput->{proxyPort};
    }
    if (defined($getUserStorageAddressOutput->{internalDirectAddress})) {
	$res->{userStorageAddress}->{internalDirectAddress} = $getUserStorageAddressOutput->{internalDirectAddress};
    }
    if (defined($getUserStorageAddressOutput->{directSecurePort})) {
	$res->{userStorageAddress}->{directSecurePort} = $getUserStorageAddressOutput->{directSecurePort};
    }
    if (defined($getUserStorageAddressOutput->{accessHandle})) {
	$res->{userStorageAddress}->{accessHandle} = $getUserStorageAddressOutput->{accessHandle};
    }
    if (defined($getUserStorageAddressOutput->{accessTicket})) {
	$res->{userStorageAddress}->{accessTicket} = $getUserStorageAddressOutput->{accessTicket};
    }

    return $res;
}

sub getLinkedDevices {
    my ($self, $args) = @_;

    defined($args->{userId}) or die "No userId in args\n";
    defined($args->{sessionHandle}) or die "No sessionHandle in args\n";
    defined($args->{serviceTicket}) or die "No serviceTicket in args\n";
    defined($args->{version}) or die "No version in args\n";

    my $httpReqBody = '<?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><GetLinkedDevicesInput xmlns="urn:vsds.wsapi.broadon.com"><session><sessionHandle>'.$args->{sessionHandle}.'</sessionHandle><serviceTicket>'.$args->{serviceTicket}.'</serviceTicket></session><userId>'.$args->{userId}.'</userId></GetLinkedDevicesInput></soapenv:Body></soapenv:Envelope>';

    my $ua = $self->_getUserAgent();
    my $httpReq = HTTP::Request->new('POST',
                                     "https://www.$self->{domain}/vsds/services/VirtualStorageDirectoryServicesSOAP",
                                     [ 'SOAPAction' => '"urn:vsds.wsapi.broadon.com/GetLinkedDevices"',
                                     ],
                                     $httpReqBody);
    my $httpRes = $ua->request($httpReq);
    $httpRes->is_success or die "FATAL: failed to POST VSDS.GetLinkedDevices: " . $httpRes->status_line . "\n";

    print "DEBUG: ".$httpRes->content."\n" if ($debug);

# sample response
# <?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><GetLinkedDevicesOutput xmlns="urn:vsds.wsapi.broadon.com"><error><errorCode>0</errorCode></error><devices><deviceId>25949351634</deviceId><deviceClass>Notebook</deviceClass><deviceName>ccd1</deviceName><isAcer>true</isAcer><hasCamera>true</hasCamera><osVersion>Linux</osVersion><protocolVersion>2</protocolVersion><featureMediaServerCapable>true</featureMediaServerCapable><featureRemoteFileAccessCapable>true</featureRemoteFileAccessCapable><featureFSDatasetTypeCapable>true</featureFSDatasetTypeCapable></devices><devices><deviceId>28323534352</deviceId><deviceClass>Notebook</deviceClass><deviceName>ccd2</deviceName><isAcer>true</isAcer><hasCamera>true</hasCamera><osVersion>Linux</osVersion><protocolVersion>2</protocolVersion></devices></GetLinkedDevicesOutput></soapenv:Body></soapenv:Envelope>

    my $xs = XML::Simple->new(ForceArray => ['devices']);
    my $httpResXml = $xs->XMLin($httpRes->content);
    defined($httpResXml->{'soapenv:Body'}) or die "No soapenv:Body element in HTTP response $httpRes->content";
    defined($httpResXml->{'soapenv:Body'}->{GetLinkedDevicesOutput}) or die "No GetLinkedDevicesOutput element in HTTP response $httpRes->content";
    my $getLinkedDevicesOutput = $httpResXml->{'soapenv:Body'}->{GetLinkedDevicesOutput};

    defined($getLinkedDevicesOutput->{error}->{errorCode}) or die "No ErrorCode element in HTTP response $httpRes->content";
    
    my $res = {};
    $res->{errorCode} = $getLinkedDevicesOutput->{error}->{errorCode};
    $res->{devices} = $getLinkedDevicesOutput->{devices};

    return $res;
}

1;
