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

using namespace sharp;

int main(int argc, char** argv)
{
	EWrapperImpl ibtrader;
	run_server(ibtrader);

	return 0;
}

