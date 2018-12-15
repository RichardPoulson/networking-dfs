//==============================================================================
// Name        : dfs.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//==============================================================================

#include "dfs.h"

#include <iostream>

namespace networking_dfs {
// PThread function
void * PThread(void * arg) {
  struct SharedResources * shared = (struct SharedResources *)arg;
  pthread_exit(NULL); // exit, don't return anything
}

void SendBadRequest(int sock)
{
	char bad_request[] = "HTTP/1.1 400 Bad Request\r\n";
	strcat(bad_request, "Connection: close\r\n\r\n\r\n");
	if (SendWholeMessage(sock, bad_request, sizeof bad_request) < 0) {
		perror("send() failed");
	}
}
//*/

ssize_t SendWholeMessage(int sock, char * buf, int buf_size)
{
	// http://beej.us/guide/bgnet/html/single/bgnet.html#sendall
	ssize_t total_sent = 0;        // how many bytes we've sent
  ssize_t bytes_left = buf_size; // how many we have left to send
  ssize_t bytes_sent;

  while(total_sent < buf_size) {
		bytes_sent = send(sock, buf + total_sent, bytes_left, 0);
    if (bytes_sent == -1) {
			perror("send() failed");
			break;
		}
    total_sent += bytes_sent;
    bytes_left -= bytes_sent;
  }
	return total_sent;
}

RequestMessage::RequestMessage()
{
	bzero((char *) &client_addr, sizeof(client_addr));
	request_line = ""; // entire request from message
  method = ""; // GET,HEAD,POST
  parameter = ""; // e.g. test.txt
	user = "";
	pass = "";
}
RequestMessage::~RequestMessage() {}

SharedResources::SharedResources()
{
	pthread_mutex_init(&this->file_mx, NULL);
	pthread_mutex_init(&this->map_mx, NULL);
	pthread_mutex_init(&this->queue_mx, NULL);
	pthread_mutex_init(&this->cout_mx, NULL);
	pthread_mutex_init(&this->continue_mx, NULL);
}

SharedResources::~SharedResources()
{
	pthread_mutex_destroy(&this->file_mx);
	pthread_mutex_destroy(&this->map_mx);
	pthread_mutex_destroy(&this->queue_mx);
	pthread_mutex_destroy(&this->cout_mx);
	pthread_mutex_destroy(&this->continue_mx);
	std::cout << " ~ SharedResources Destructor ~" << std::endl;
}
//
DFS::DFS(char * port_num, std::string folder_dir, int timeout)
{
	std::cout << " ~ DFS Constructor ~" << std::endl;
  buffer = (char*) malloc (kBufferSize);
	bzero((char *) &server_addr_, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_.sin_port = htons(atoi(port_num));
	timeout_.tv_sec = timeout; // for listening socket (in seconds)
	timeout_.tv_usec = 0;
	LoadConfigFile();
	CreateBindSocket();
	StartDFSService();
}

DFS::~DFS() {
	std::cout << " ~ DFS Destructor ~" << std::endl;
  delete [] buffer;
}

bool DFS::CreateBindSocket() {
	std::cout << " ~ CreateBindSocket ~" << std::endl;
	//  0= pick any protocol that socket type supports, can also use IPPROTO_TCP
	if ((this->listen_sd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR opening socket");
		return false;
	}
	//  Allows bind to reuse socket address (if supported)
	if (setsockopt(this->listen_sd_, SOL_SOCKET, SO_REUSEADDR,
	    (const void *)&kOn, sizeof(int)) < 0) {
		perror("setsockopt() failed");
		close(listen_sd_);
		return false;
	}
  /*==  set port to be non-blocking
	// set listen socket to be non-binding
	if (ioctl(listen_sd_, FIONBIO, (char *)&on_) < 0) {
		perror("ioctl() failed");
		close(listen_sd_);
		return false;
	}
  */
	//  Define the server's Internet address
	if (bind(listen_sd_, (struct sockaddr *)&server_addr_, sizeof(server_addr_)) < 0) {
	    perror("ERROR on binding");
	    close(listen_sd_);
	    return false;
	}
	// set the listen back log
	if (listen(listen_sd_, 32) < 0) {
		perror("listen() failed");
		close(listen_sd_);
		return false;
	}
	// initialize master file-descriptor set
	FD_ZERO(&master_set_);
	FD_SET(listen_sd_, &master_set_);
	return true;
}

bool DFS::LoadConfigFile() {
	std::string line;
	std::string user;
	std::string pass;
	char cstring[256];
	char * pch;
  std::ifstream config_file("dfs.conf");
  if(config_file.is_open()) {
		while(getline(config_file, line)) {
			bzero(cstring, sizeof cstring);
			strcpy(cstring,line.c_str()); // copy string to cstring
			pch = strtok(cstring, " "); // first get username
      while (pch != NULL) // go through the line
      {
				user.assign(pch); // buffer username
				pch = strtok(NULL, " ");  // get password
				pass.assign(pch); // buffer password
				user_pass_map_[user] = pass; // map folder
				std::cout << user << ": " << user_pass_map_[user] << std::endl;
				pch = strtok(NULL, " ");  // should return NULL, breaking loop
      }
		}
		config_file.close();
		return true;
  }
  else {
		std::cout << "Unable to open file";
	  return false;
	}
}

bool DFS::ProcessDFSRequest(struct RequestMessage * request) {
	std::string line;
	std::string user;
	std::string pass;
	char cstring[256] = "";
	char * pch;
	strcpy(cstring,request->request_line.c_str()); // copy string to cstring
	pch = strtok(cstring, " "); // get method
	request->method.assign(pch); // list, get, put
	if (request->method != "list") { // if method not list, get parameter
		pch = strtok(NULL, " ");
		request->parameter.assign(pch);
	}
	pch = strtok(NULL, " ");  // get user
	request->user.assign(pch);
	pch = strtok(NULL, " ");  // get password
	request->pass.assign(pch);
	std::cout << request->request_line << std::endl;
	std::cout << request->method << ", " << request->parameter << ", " <<
	        request->user << ", " << request->pass << std::endl;
	return true;
}

void DFS::StartDFSService() {
	std::cout << " ~ StartDFSService ~" << std::endl;

  int max_sd_ = listen_sd_; // listen socket has highest descriptor
	int new_sd_; // socket descriptor for new client
	bool continue_servicing = true; // if set to false, exits while loop below
	struct RequestMessage request; // request from client
	int i; // iterator for loop in StartHTTPServices()
	int num_sockets_ready; // keeps track of how many socket descriptors are readable
	ssize_t bytes_received;
	struct sockaddr_in client_address; // client addr
	socklen_t addr_length;
	char * buffer = (char*) malloc (kBufferSize);
	std::string error_message = "ERROR 403 Forbidden\r\n\r\n\r\n";
	std::cout << "About to enter service loop." << std::endl;

	while (continue_servicing) {
		// copy master set to working set
		memcpy(&working_set_, &master_set_, sizeof(master_set_));
		// how many socket descriptors are readable?
		num_sockets_ready = select(max_sd_+1, &working_set_, NULL, NULL, &timeout_);
		if (num_sockets_ready < 0) { // ERROR
			perror("select() failed");
			continue_servicing = false;
		}
		else if (num_sockets_ready == 0) { // TIMEOUT
			continue_servicing = false;
		}
		else { // 1 or more socket descriptors are readable
			for (i=0; i <= max_sd_; ++i) {
				if (FD_ISSET(i, &working_set_)) { // read data from them
					if (i == listen_sd_) { // SERVER SOCKET
						if ((new_sd_ = accept(listen_sd_,
									(struct sockaddr *)&client_address, &addr_length)) < 0) {
							perror("accept() failed.");
						}
						FD_SET(new_sd_, &master_set_);
						if (new_sd_ > max_sd_) {
							max_sd_ = new_sd_;
						}
					}
					else { // socket other than listening socket wants to send data
						if ((bytes_received = recv(i, buffer, kBufferSize, 0)) < 0) {
							if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) { // ERROR
								perror("recv() failed");
								continue_servicing = false;
							}
						}
						else if (bytes_received == 0) { // Client closed connection
							close(i);
							FD_CLR(i, &master_set_);
							if (i == max_sd_) {
								if (FD_ISSET(max_sd_, &master_set_) == 0) {
									max_sd_ = max_sd_ - 1;
								}
							}
						}
						else { // bytes_received  > 0
              request.request_line.copy(buffer, bytes_received); // request
              request.clients_sd = i; // client socket descriptor
              request.client_addr = client_address; // sockaddr_in
              request.addr_len = addr_length; // socklen_t
              if (ProcessDFSRequest(&request) != true) {
								std::cout << "bad request" << std::endl;
              }
              else std::cout << "good request" << std::endl;

              //std::cout << buffer << std::endl;
              //send(i, buffer, bytes_received, 0);
							//*/
						}
					} // Enf of if i != listen_sd_
				} // End of if (FD_ISSET(i, &read_fds_))
			} // End of loop through selectable descriptors
		} // End of Else (socket is not listen_sd
	} // End while(continue_servicing)
	// clean up open sockets
	for (i=0; i <= max_sd_; ++i) {
		if (FD_ISSET(i, &master_set_)) {
			close(i);
			if (i == listen_sd_) {
				std::cout << "- closing LISTENING socket (" << i << ")" << std::endl;
			}
			else {
				std::cout << "- closing client socket (" << i << ")" << std::endl;
			}
		}
	}
	free(buffer);
}
} // namespace networking_dfs
