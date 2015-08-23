#include "sharp.h"

using namespace sharp;

int main(int argc, char** argv)
{
    init_logging();

	EWrapperImpl ibtrader;
	run_server(ibtrader);

	return 0;
}

