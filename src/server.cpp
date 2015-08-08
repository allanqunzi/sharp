/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

/**
#ifdef _WIN32
# include <Windows.h>
# define sleep( seconds) Sleep( seconds * 1000);
#else

#endif
*/
#include <unistd.h>
#include "sharp.h"


const unsigned MAX_ATTEMPTS = 50;
const unsigned SLEEP_TIME = 10;

using namespace sharp;

int main(int argc, char** argv)
{
	//const char* host = argc > 1 ? argv[1] : "";
	//unsigned int port = 7496;
	//int clientId = 0;
/*

	unsigned attempt = 0;
	printf( "Start of POSIX Socket Client Test %u\n", attempt);

	for (;;) {
		++attempt;
		printf( "Attempt %u of %u\n", attempt, MAX_ATTEMPTS);

		EWrapperImpl client;

		client.connect( client.host, client.port, client.clientId);

		while( client.isConnected()) {
			client.monitor();
		}
		std::cout<<"outside while"<<std::endl;

		if( attempt >= MAX_ATTEMPTS) {
			break;
		}

		printf( "Sleeping %u seconds before next attempt\n", SLEEP_TIME);
		sleep( SLEEP_TIME);
	}

	printf ( "End of POSIX Socket Client Test\n");
*/
	EWrapperImpl ibtrader;
	run_server(ibtrader);
	return 0;
}

