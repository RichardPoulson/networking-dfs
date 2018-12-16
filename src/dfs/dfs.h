//==============================================================================
// Name        : dfs.h
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//==============================================================================

#ifndef NETWORKING_DFS_H_
#define NETWORKING_DFS_H_

#include <string>

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
static const int kBufferSize = 10485760; // 10 MB
static const int kMaxNumDFSThreads = 8;
static const int kOn = 1; // used for setsockopt
static const int kOff = 0; // ""

// PThread function
void * PThread(void * arg);
// send HTTP/1.1 400 Bad Request\r\n\r\n\r\n
void SendBadRequest(int sock);
// keeps sending until all bytes sent
ssize_t SendWholeMessage(int sock, char * buf, int buf_size);
// data structure dealing with HTTP request messages
struct RequestMessage
{
	int clients_sd; // socket descriptor for client socket
  struct sockaddr_in client_addr; // client addr
	socklen_t addr_len;
	std::string request_line; // entire request from message
  std::string method; // GET,HEAD,POST
  std::string parameter; // e.g. test.txt
	std::string user;
	std::string pass;
  RequestMessage();
  virtual ~RequestMessage();
};
// struct designed to be used between the WebProxy object and pthreads
struct SharedResources {
  struct timeval socket_timeout; // timeout value for socket
  pthread_mutex_t file_mx, map_mx, queue_mx, cout_mx, continue_mx;
  std::queue<struct RequestMessage> request_queue;//client_queue;
  SharedResources();
  virtual ~SharedResources();
};

// WebProxy constructor takes two arguments (see below for default values).
class DFS {
public:
	DFS(char * port_num, std::string folder_dir, int timeout = 20);
	virtual ~DFS();
protected:
	char * buffer;
	std::string folder_; // folder that DFS works from
  struct sockaddr_in server_addr_; // address for listening socket
  int listen_sd_; // listen/max/new socket descriptors
  struct timeval timeout_; // timeout of server's listen socket
  fd_set master_set_, working_set_; // file descriptor sets, used with select()
	std::map<std::string, std::string> user_pass_map_;

	pthread_t pthread_id_;
	struct thread_info * thread_info_p_;
	pthread_attr_t pthread_attr_;
	int pthread_stack_size_;
	void * pthread_result_p_;

	struct SharedResources * shared_;
	bool CreateBindSocket();
	bool LoadConfigFile();
	bool ProcessDFSRequest(struct RequestMessage *);
	void StartDFSService();
};

} // namespace networking_dfs

#endif // NETWORKING_DFS_H_
