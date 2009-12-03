#include <stdio.h>
#include <stdlib.h>

#include "mod_google_analytics.h"

int main(int argc, const char *argv[])
{
	enum  Carrier carrier;

	carrier = UNKNOWN;
    fprintf( stdout, "Unknown : %d\n", UNKNOWN );

	carrier = DOCOMO;
    fprintf( stdout, "DoCoMo : %d\n", carrier );

	carrier = KDDI;
    fprintf( stdout, "Kddi : %d\n", carrier );

	carrier = SOFTBANK;
    fprintf( stdout, "SoftBank : %d\n", carrier );

	carrier = WILLCOM;
    fprintf( stdout, "Willcom : %d\n", carrier );

	exit(0);
}



