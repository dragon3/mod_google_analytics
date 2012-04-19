use strict;
use warnings;

use Test::More;
use LWP::UserAgent;

my $ua = LWP::UserAgent->new;

sub test_pc {
    my $agent          = shift;
    my $path           = shift || $ENV{TEST_PATH} || "/";
    my $account_number = shift || $ENV{TEST_ACCOUNT_NUMBER} || "";

    $ua->agent($agent);
    my $req = HTTP::Request->new( GET => 'http://localhost:38080' . $path );
    my $res = $ua->request($req);
    ok $res->is_success;
    my $expected = <<"__BODY__";
<html><body><h1>It works!</h1><script type="text/javascript"><!-- 
 var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
//--></script><script type="text/javascript"><!-- 
 try {var pageTracker = _gat._getTracker("${account_number}");pageTracker._trackPageview();} catch(err) {}; 
//--></script></body></html>
__BODY__
    chomp $expected;
    is $res->content, $expected;
}

sub test_mobile {
    my $agent          = shift;
    my $referer        = shift || $ENV{TEST_REFERER} || "";
    my $path           = shift || $ENV{TEST_PATH} || "/";
    my $account_number = shift || $ENV{TEST_MOBILE_ACCOUNT_NUMBER} || "";

    if ( $path =~ m|/$| ) {
        $path .= "index.html";
    }

    $ua->agent($agent);
    my $req = HTTP::Request->new( GET => 'http://localhost:38080' . $path );
    my $res = $ua->request($req);

    ok $res->is_success;
    like $res->content,
qr/<img src="\/ga\.php\?utmac=$account_number&amp;utmn=[0-9]+&amp;utmr=-&amp;utmp=$path&amp;guid=ON" \/><\/body>/;
}
