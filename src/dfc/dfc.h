//==============================================================================
// Name        : dfc.h
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//==============================================================================

#ifndef NETWORKING_DFS_DFC_H_
#define NETWORKING_DFS_DFC_H_

// crypto++
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <arpa/inet.h> //  htonl, htons, inet_ntoa,
#include <netinet/in.h> //  sockaddr_in, INADDR_ANY,
#include <openssl/md5.h> // MD5 hash
#include <sys/ioctl.h> // set socket to be nonbinding
#include <sys/socket.h> //  socklen_t,
#include <sys/types.h>
#include <cerrno> //  "C Errors", errno
#include <ctime> //  time_t, tm,
#include <dirent.h> //  "Traverse directory", opendir, readdir,
#include <fstream> // ifstream
#include <iostream> // cout
#include <map>
#include <netdb.h>
#include <pthread.h> // process threads,
#include <queue>
#include <signal.h> // signal(int void (*func)(int))
#include <stack> //  stack of process threads
#include <stddef.h> // NULL, nullptr_t
#include <stdio.h> // FILE, size_t, fopen, fclose, fread, fwrite,
#include <stdlib.h> //  exit, EXIT_FAILURE, EXIT_SUCCESS
#include <string>
#include <string.h> //  strlen, strcpy, strcat, strcmp
#include <unistd.h>

//#include <regex> //  Regular expressions

namespace networking_dfs_dfc {
static const int kBufferSize = 10485760; // 10 MB
static const int kNumDFSServers = 4; // there are 4 DFS
static const int kOn = 1; // used for setsockopt
static const int kOff = 0; // ""

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

// data structure dealing with HTTP request messages
struct HashStruct
{
	std::string str; // string representation of value in hex form
  unsigned long long int ull; // value of the hash
  HashStruct();
  virtual ~HashStruct();
};

// WebProxy constructor takes two arguments (see below for default values).
class DFC {
public:
	DFC();
	virtual ~DFC();
protected:
	char * buffer_[4];
	int sockfd_; // file descriptor for socket connecting to DFS
	std::map<std::string, std::string> server_map_;
	std::map<std::string, std::string>::const_iterator map_it;
	std::string user_;
	std::string password_;
	struct sockaddr_in serv_addr_;
	struct addrinfo hints_, *servinfo_, *p_;
  struct timeval timeout_; // timeout of server's listen socket
  fd_set master_set_, working_set_; // file descriptor sets, used with select()
	bool CreateBindSocket(std::string addr_str);
	int GetFileSize(std::string file_name);
	void HandleInput(std::string input);
	void HashMessage(std::string message, struct HashStruct * hash_struct);
	bool LoadConfigFile();
	void SendCommand(std::string command_str);
	void StartDFCService();
	void UploadFile(std::string command_str);
};

} // namespace networking_dfs_dfc

#endif // NETWORKING_DFS_DFC_H_
