//============================================================================
// Name        : driver.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//============================================================================

#include <csignal>

#include "dfs.h"

int main(int argc, char **argv) {
	if (argc != 4) {
	    std::cout << argc << std::endl;
	    std::cout <<
	      "Invalid number of additional arguments, please enter a port number," <<
	      " a folder directory, and a socket timeout value." << std::endl;
	      exit(EXIT_FAILURE);
	  }
	std::cout << "Starting Distributed File Server\"" << atoi(argv[1]) << "\" with a timeout of \"" << atoi(argv[2]) <<
			"\" seconds." << std::endl;
	networking_dfs::DistributedFileServer my_dfs(argv[1], argv[2]);
	return 0;
}
