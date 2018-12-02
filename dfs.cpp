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
void * AcceptConnection(void * shared_resources) {
	/*
	// cast shared resources so we can use
	struct SharedResources * shared = (struct SharedResources*)shared_resources;
	char * buffer = (char*) malloc (WEBPROXY_BUFFER_SIZE);
	int bytes_received, bytes_sent;
	bool page_cached, cached_page_old; // Is the requested page cached?  Is the cached page old?
	std::map<std::string, struct CachedPage *>::iterator cached_iter;
	std::map<std::string, struct addrinfo *>::iterator addrinfo_iter;
	struct CachedPage * cached_point;
	struct addrinfo hints, *results, *rp; // used with getaddrinfo()
	int i; // iterator for loop in StartHTTPServices()
	int http_serv_sd;

	RequestMessage request = shared->request_queue.front();
	shared->request_queue.pop();

	std::cout << "(" << request.clients_sd << ") [" << request.host_name << ":" <<
			request.host_port << "]  " << request.request_line << std::endl;
	cached_iter = shared->cached_page_map.find(request.request_line);
	// PAGE NOT CACHED page_cached, cached_page_old
	if (cached_iter == shared->cached_page_map.end()) {
		page_cached = false;
	}

	close(request.clients_sd);
	delete [] buffer;
	std::cout << "Exiting pthread\n";
  pthread_exit(NULL);
  */
}

bool ProcessDFSRequest(char * buf, struct RequestMessage * req) {
	std::cmatch cmatch1, cmatch2; // first match with a line, then with what you're looking for
	/*

	std::regex_search (buf, cmatch1, std::regex("Host: .+"));
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(?![Host: ])[^:]*"));
	req->host_name = cmatch2.str();
	// Had to make custom regex to separate port number from address
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(?![Host: " + req->host_name + ":])[^:]*"));
	req->host_port = cmatch2.str();
	if (req->host_port == "") { req->host_port = "80"; } // if not defined, assume port 80
	std::regex_search (buf, cmatch1, std::regex("^(GET|HEAD|POST|CONNECT).*"));
	req->request_line = cmatch1.str().c_str();
	req->request = buf; // copy entire request
	std::regex_search (cmatch1.str().c_str(), cmatch2, std::regex("(GET|HEAD|POST|CONNECT)"));
	req->method = cmatch2.str();
	if (req->method.compare("GET") != 0) {
		return false;
	}
	*/
	return true;
}

void SendBadRequest(int sock) {
	char bad_request[] = "HTTP/1.1 400 Bad Request\r\n";
	strcat(bad_request, "Connection: close\r\n\r\n\r\n");
	if (SendWholeMessage(sock, bad_request, sizeof bad_request) < 0) {
		perror("send() failed");
	}
}
//*/

ssize_t SendWholeMessage(int sock, char * buf, int buf_size) {
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

RequestMessage::RequestMessage() {
	bzero((char *) &client_addr, sizeof(client_addr));
}
RequestMessage::~RequestMessage() {}

SharedResources::SharedResources() {
	pthread_mutex_init(&this->file_mx, NULL);
	pthread_mutex_init(&this->map_mx, NULL);
	pthread_mutex_init(&this->queue_mx, NULL);
	pthread_mutex_init(&this->cout_mx, NULL);
	pthread_mutex_init(&this->continue_mx, NULL);
	this->regex_map["request line"] = std::regex(
			"^(GET|HEAD|POST|CONNECT).*");
	this->regex_map["method"] = std::regex("(GET|HEAD|POST|CONNECT)"); // Method
	this->regex_map["http"] = std::regex("HTTP\\/\\d\\.\\d"); // HTTP Version
	this->regex_map["connection line"] = std::regex("Connection: (keep-alive|close)\r\n"); // Connection: keep-alive
	this->regex_map["connection"] = std::regex("(keep-alive|close)"); // Connection: keep-alive
	this->regex_map["headers"] = std::regex("HTTP.+\r\n(.+\r\n)+\r\n"); // Connection: keep-alive
	this->regex_map["host line"] = std::regex("Host: .+"); // Connection: keep-alive
	this->regex_map["host name"] = std::regex("(?![Host: ])[^:]*"); // Connection: keep-alive
	this->regex_map["host port"] = std::regex("(?![:]).*"); // Connection: keep-alive
}

SharedResources::~SharedResources() {
	pthread_mutex_destroy(&this->file_mx);
	pthread_mutex_destroy(&this->map_mx);
	pthread_mutex_destroy(&this->queue_mx);
	pthread_mutex_destroy(&this->cout_mx);
	pthread_mutex_destroy(&this->continue_mx);
	std::cout << "~ SharedResources destructor ~" << std::endl;
}
//
DistributedFileServer::DistributedFileServer(char * port_num, std::string folder_dir, int timeout) {
  //  Define the server's Internet address
	bzero((char *) &server_addr_, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_.sin_port = htons(atoi(port_num));
	timeout_.tv_sec = timeout; // for listening socket (in seconds)
	timeout_.tv_usec = 0;
	shared_ = new struct SharedResources();
  /*
	// Initialize and set thread joinable
  pthread_attr_init(&pthread_attr);
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);
  */
	CreateBindSocket();
	StartDFSService();
}

DistributedFileServer::~DistributedFileServer() {
	pthread_attr_destroy(&pthread_attr);
	delete(this->shared_);
	std::cout << "~ DistributedFileServer destructor ~" << std::endl;
}

bool DistributedFileServer::CreateBindSocket() {
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
  /*
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

void DistributedFileServer::StartDFSService() {
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
	char * error_message = "ERROR 403 Forbidden\r\n\r\n\r\n";

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
								SendBadRequest(i);
								std::cout << "! client " << i << ": bad request: \"" <<
										request.request_line << "\"" << std::endl;
								continue;
							}
              std::cout << request.request_line << std::endl;
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
