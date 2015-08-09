#include "sharp.h"

using namespace sharp;

int main(int argc, char** argv)
{
	EWrapperImpl ibtrader;
	run_server(ibtrader);

	return 0;
}

