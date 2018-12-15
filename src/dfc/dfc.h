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
#include <sys/ioctl.h> // set socket to be nonbinding
#include <netinet/in.h> //  sockaddr_in, INADDR_ANY,
#include <arpa/inet.h> //  htonl, htons, inet_ntoa,
#include <dirent.h> //  "Traverse directory", opendir, readdir,
#include <cerrno> //  "C Errors", errno
#include <regex> //  Regular expressions
#include <pthread.h> // process threads,
#include <ctime> //  time_t, tm,
#include <stack> //  stack of process threads

#include <queue>
#include <map> // map of string to regex
#include <openssl/md5.h> // MD5 hash

namespace networking_dfs_dfc {
static const int kBufferSize = 10485760; // 10 MB
static const int kNumDFSServers = 4; // there are 4 DFS
static const int kOn = 1; // used for setsockopt
static const int kOff = 0; // ""

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

ssize_t SendWholeMessage(int sock, char * buf, int buf_size);
// data structure dealing with HTTP request messages

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
	char * buffer_;
	int sockfd_; // file descriptor for socket connecting to DFS
	std::map<std::string, std::string> server_map_;
	std::string user_;
	std::string password_;
	struct sockaddr_in serv_addr_;
	struct addrinfo hints_, *servinfo_, *p_;
  struct timeval timeout_; // timeout of server's listen socket
  fd_set master_set_, working_set_; // file descriptor sets, used with select()
	bool BindSocket(std::string addr_str);
	bool CreateSocket();
	void HandleInput(std::string input);
	void HashMessage(std::string message, struct HashStruct * hash_struct);
	bool LoadConfigFile();
	void SendCommand(std::string command_str);
	void StartDFCService();
	void UploadFile(std::string command_str);
};

} // namespace networking_dfs_dfc

#endif // NETWORKING_DFS_DFC_H_
