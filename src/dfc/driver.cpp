//============================================================================
// Name        : driver.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//============================================================================

#include <csignal>

#include "dfc.h"

int main(int argc, char **argv) {
	if (argc != 3) {
	    std::cout << argc << std::endl;
	    std::cout <<
	      "Invalid number of additional arguments, please enter two integers" <<
	      " <port number> <timeout> as arguments." << std::endl;
	      exit(EXIT_FAILURE);
	  }
	std::cout << "Starting WebProxy on port \"" << atoi(argv[1]) << "\" with a timeout of \"" << atoi(argv[2]) <<
			"\" seconds." << std::endl;
	DistributedFileServer my_dfs(argv[1], atoi(argv[2]));
	return 0;
}
