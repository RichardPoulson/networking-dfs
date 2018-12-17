//============================================================================
// Name        : driver.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description : driver program that creates a DistributedFileServer
//============================================================================

#include <csignal>

#include "dfs.h"

int main(int argc, char **argv) {
	if (argc < 3) {
	    std::cout << argc << std::endl;
	    std::cout <<
	    "Invalid number of additional arguments, please enter a port number," <<
	    " a folder directory, and a socket timeout value (optional)." << std::endl;
	    exit(EXIT_FAILURE);
	  }
	std::cout << std::endl << "Starting Distributed File Server \"" << atoi(argv[1]) <<
	    "\", will exit if idle for " << atoi(argv[3]) <<
			" seconds." << std::endl;
	networking_dfs::DFS * my_dfs = new networking_dfs::DFS(argv[1], argv[2], atoi(argv[3]));
	delete(my_dfs);
	return 0;
}
