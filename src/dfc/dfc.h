//==============================================================================
// Name        : dfc.h
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//==============================================================================

#ifndef NETWORKING_DFC_H_
#define NETWORKING_DFC_H_

#include "dfs.h"

#include <stddef.h> // NULL, nullptr_t
#include <stdio.h> // FILE, size_t, fopen, fclose, fread, fwrite,
#include <iostream> // cout
#include <fstream> // ifstream
#include <signal.h> // signal(int void (*func)(int))
#include <unistd.h>
#include <stdlib.h> //  exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string>
#include <string.h> //  strlen, strcpy, strcat, strcmp
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h> //  socklen_t,
#include <netinet/in.h> //  sockaddr_in, INADDR_ANY,
#include <arpa/inet.h> //  htonl, htons, inet_ntoa,
#include <dirent.h> //  "Traverse directory", opendir, readdir,
#include <cerrno> //  "C Errors", errno
#include <regex> //  Regular expressions
#include <pthread.h> // process threads,
#include <ctime> //  time_t, tm,
#include <stack> //  stack of process threads
#include <sys/ioctl.h> // set socket to be nonbinding
#include <queue>
#include <map> // map of string to regex
#include <unordered_set> // unordered set of blacklisted sites

namespace networking_dfs {

class DFSClient {
public:
	DFSClient(char * port_num, std::string folder_dir, int timeout = 60);
	virtual ~DFSClient();
private:
  struct sockaddr_in client_addr_; // address for listening socket
  int client_sd_; // listen/max/new socket descriptors
  struct timeval timeout_; // timeout of server's listen socket
  fd_set master_set_, working_set_; // file descriptor sets, used with select()
	bool CreateBindSocket();
	void StartDFSService();
	void UploadFile();
	void DownloadFile();
};

} // namespace networking_dfs

#endif // NETWORKING_DFC_H_
