use strict;
use warnings;

use t::MyTest;

subtest "docomo" => sub {
    test_mobile("DoCoMo/1.0/D501i");
};

subtest "j-phone" => sub {
    test_mobile(
"J-PHONE/5.0/V801SH[/1234] SA/AAAA Profile/MIDP-1.0 Configuration/CLDC-1.0 Ext-Profile/JSCL-1.1.0"
    );
};

subtest "vodafone" => sub {
    test_mobile(
"Vodafone/1.0/V702NK/NKJ001[/4567] Series60/2.6 Nokia6630/x.x.x Profile/MIDP-2.0 Configuration/CLDC-1.1"
    );
};

subtest "softbank" => sub {
    test_mobile(
"SoftBank/1.0/910T/TJ001/SN Browser/NetFront/3.3 Profile/MIDP-2.0 Configuration/CLDC-1.1"
    );
};

subtest "ddipocket" => sub {
    test_mobile(
"Mozilla/3.0 (DDIPOCKET;KYOCERA/AH-K3001V/1.5.2.8.000/0.1/C100) Opera 7.0"
    );
};

subtest "willcom" => sub {
    test_mobile("Mozilla/3.0(WILLCOM;SANYO/WX310SA/2;1/1/C128) NetFront/3.3");
    test_mobile(
"Mozilla/3.0(WILLCOM;KYOCERA/WX300K/1;1.0.2.8.000000/0.1/C100) Opera/7.0"
    );
};

subtest "emobile" => sub {
    test_mobile("emobile/1.0.0 (H11T; like Gecko; Wireless) NetFront/3.4");
};

subtest "kddi" => sub {
    test_mobile("KDDI-CA23 UP.Browser/5.2.0.1.126 (GUI) MMP/2.0");
};
done_testing;
