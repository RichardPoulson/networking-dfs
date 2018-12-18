//============================================================================
// Name        : driver.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description : driver program that creates a distributed file system client
//============================================================================

#include <csignal>

#include "dfc.h"

int main(int argc, char **argv) {
	std::cout << std::endl << "Starting Distributed File Client" << std::endl;
	networking_dfs_dfc::DFC * my_dfc = new networking_dfs_dfc::DFC();
	delete(my_dfc);
	return 0;
}
