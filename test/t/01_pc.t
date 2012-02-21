use strict;
use warnings;

use t::MyTest;

subtest "ie6" => sub {
    test_pc("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1)");
};
subtest "ie7" => sub {
    test_pc("Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)");
};
subtest "ie8" => sub {
    test_pc(
"Mozilla/4.0 (compatible; GoogleToolbar 5.0.2124.2070; Windows 6.0; MSIE 8.0.6001.18241)"
    );
};
subtest "ie9" => sub {
    test_pc(
        "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)"
    );
};
subtest "chrome" => sub {
    test_pc(
"Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.7 (KHTML, like Gecko) Chrome/16.0.912.75 Safari/535.7"
    );
};
subtest "firefox" => sub {
    test_pc(
"Mozilla/5.0 (Windows NT 6.1; WOW64; rv:9.0.1) Gecko/20100101 Firefox/9.0.1"
    );
};
subtest "safari" => sub {
    test_pc(
"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/534.52.7 (KHTML, like Gecko) Version/5.1.2 Safari/534.52.7"
    );
};

done_testing;
