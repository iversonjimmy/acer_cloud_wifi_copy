package Ias;

use URI;
use LWP 5.66;
use HTTP::Request;
use Time::HiRes;
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

# Input hashref:
#  username	: username, no defaults
#  password	: password, defaults to 'password'
# Output hashref:
#  errorCode		: content of ErrorCode element
#  userId		: content of UserId element
#  sessionHandle	: content of SessionHandle element
#  sessionSecret	: content of SessionSecret element
#  storageClusterId	: content of StorageClusterId element
sub login {
    my ($self, $args) = @_;

    defined($args->{username}) or die "FATAL: username missing\n";
    my $username = $args->{username};
    my $password = defined($args->{password}) ? $args->{password} : 'password';
    my $timestamp = int(Time::HiRes::time() * 1000.0);

    my $httpReqBody = '<?xml version="1.0" encoding="UTF-8"?><soapenv:Envelope xmlns:soapenv="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"><soapenv:Body><Login xmlns="urn:ias.wsapi.broadon.com"><Version>2.0</Version><MessageId>fumi-'.$timestamp.'</MessageId><Region>US</Region><Country>US</Country><Language>en</Language><Username>'.$username.'</Username><Namespace>acer</Namespace><Password>'.$password.'</Password><ModuleId>0</ModuleId></Login></soapenv:Body></soapenv:Envelope>';

    my $ua = $self->_getUserAgent();
    my $httpReq = HTTP::Request->new('POST',
                                     "https://www.$self->{domain}/ias/services/IdentityAuthenticationSOAP",
                                     [ 'SOAPAction' => '"urn:ias.wsapi.broadon.com/Login"',
                                     ],
                                     $httpReqBody);
    my $httpRes = $ua->request($httpReq);
    $httpRes->is_success or die "FATAL: failed to POST IAS.Login: " . $httpRes->status_line . "\n";

    print "DEBUG: ".$httpRes->content."\n" if ($debug);

    my $xs = XML::Simple->new;
    my $httpResXml = $xs->XMLin($httpRes->content);
    defined($httpResXml->{'soapenv:Body'}) or die "No soapenv:Body element in HTTP response ".$httpRes->content;
    defined($httpResXml->{'soapenv:Body'}->{LoginResponse}) or die "No LoginResponse element in HTTP response ".$httpRes->content;
    my $loginResponse = $httpResXml->{'soapenv:Body'}->{LoginResponse};

    defined($loginResponse->{ErrorCode}) or die "No ErrorCode element in HTTP response ".$httpRes->content;

    my $res = {};
    $res->{errorCode}		= $loginResponse->{ErrorCode};
    $res->{userId}		= $loginResponse->{UserId};
    $res->{sessionHandle}	= $loginResponse->{SessionHandle};
    $res->{sessionSecret}	= $loginResponse->{SessionSecret};
    $res->{storageClusterId}	= $loginResponse->{StorageClusterId};

    return $res;
}

1;
