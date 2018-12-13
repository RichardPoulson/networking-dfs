//==============================================================================
// Name        : dfc.cpp
// Author      : Richard Poulson
// Version     : 1.0
// Last edit   : 12/2/2018
//
// Description :
//==============================================================================

#include "dfc.h"

namespace networking_dfs {

DFSClient::DFSClient(char * port_num, std::string folder_dir, int timeout) {
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

DFSClient::~DFSClient() {
	pthread_attr_destroy(&pthread_attr);
	delete(this->shared_);
	std::cout << "~ DistributedFileServer destructor ~" << std::endl;
}

bool DFSClient::CreateBindSocket() {
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

void DFSClient::StartDFSService() {
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
