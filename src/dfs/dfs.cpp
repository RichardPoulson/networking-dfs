/*==============================================================================
Name        : dfs.cpp
Author      : Richard Poulson
Version     : 1.0
Last edit   : 12/2/2018

Description :

http://www.cplusplus.com/reference/condition_variable/condition_variable/
https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html
https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_72/apis/users_74.htm
https://stackoverflow.com/questions/10356712/mkdir-windows-vs-linux
==============================================================================*/

#include "dfs.h"

#include <iostream>

namespace networking_dfs {
// PThread function
void * PThread(void * arg) {
  //std::cout << " ~ PThread ~" << std::endl;
  struct SharedResources * shared = (struct SharedResources *)arg;
  char * buffer = (char*) malloc (kBufferSize);
  DIR * dir_ptr;
  struct dirent * entry_ptr;
  std::filebuf * filebuf_ptr; // file buffer pointer, used for getting file size
  std::string str = ""; // used to easilty modify file paths
  std::ifstream ifs; // input file stream, used to read from files.
  int num_bytes;

  pthread_mutex_lock(&shared->queue_mx); // LOCK queue_mx
  struct RequestMessage request = shared->request_queue.front();
  shared->request_queue.pop();
  pthread_mutex_unlock(&shared->queue_mx);  // UNLOCK queue_mx

  str.append(request.folder + "/" + request.user + "/");

  pthread_mutex_lock(&shared->file_mx); // LOCK file_mx
  // if returns NULL, make folder for user
  if((dir_ptr = opendir(str.c_str())) == NULL) {
    pthread_mutex_lock(&shared->cout_mx); // LOCK cout_mx
    std::cout << "+ making directory " << str << std::endl;
    pthread_mutex_unlock(&shared->cout_mx); // UNLOCK cout_mx
    #ifdef linux
      mkdir(str.c_str(), 0777);
    #elif _WIN32
      _mkdir(strPath.c_str());
    #else
      mkdir(str.c_str(), 0777);
    #endif
    dir_ptr = opendir(str.c_str());
  }
  pthread_mutex_unlock(&shared->file_mx);  // UNLOCK file_mx

  if (request.method == "list") {
    pthread_mutex_lock(&shared->file_mx); // LOCK file_mx
    memset(buffer, '0', kBufferSize);
    strcpy(buffer, "");
    while((entry_ptr = readdir(dir_ptr))){
      if((strcmp(entry_ptr->d_name, ".") != 0)
          && (strcmp(entry_ptr->d_name, "..") != 0 )) {
        strcat(buffer, entry_ptr->d_name);
        strcat(buffer, "\n");
      }
    }
    strcat(buffer, "\0"); // null terminator
    closedir(dir_ptr);
    pthread_mutex_unlock(&shared->file_mx); // UNLOCK file_mx
    send(request.clients_sd, buffer, strlen(buffer) + 1, 0); // don't forget NULL
  }
  else if (request.method == "get") {
    pthread_mutex_lock(&shared->file_mx); // LOCK file_mx
    ifs.open(request.parameter.c_str(), std::ifstream::binary);
    filebuf_ptr = ifs.rdbuf();
    num_bytes = filebuf_ptr->pubseekoff(0,ifs.end,ifs.in);
    filebuf_ptr->pubseekpos(0,ifs.in); // go back to beginning
    ifs.read(buffer, num_bytes);
    pthread_mutex_unlock(&shared->file_mx); // UNLOCK file_mx
    send(request.clients_sd, buffer, num_bytes, 0);
  }
  else if (request.method == "put") {


  }
  std::this_thread::sleep_for (std::chrono::seconds(5));

  //pthread_mutex_lock(&shared->cout_mx); // LOCK cout_mx
  //std::cout << " ~ PThread finished ~" << std::endl;
  //pthread_mutex_unlock(&shared->cout_mx); // UNLOCK cout_mx

  pthread_mutex_lock(&shared->queue_mx); // LOCK queue_mx
  shared->num_avail_pthreads_ += 1;
  shared->pthread_queue.push(request.pthread_ptr);
  pthread_mutex_unlock(&shared->queue_mx);  // UNLOCK queue_mx
  //*/

  delete [] buffer;
  pthread_cond_signal(&shared->pthreads_avail_cv);
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
  num_avail_pthreads_ = kMaxNumDFSThreads;
	pthread_mutex_init(&this->file_mx, NULL);
	pthread_mutex_init(&this->map_mx, NULL);
	pthread_mutex_init(&this->queue_mx, NULL);
	pthread_mutex_init(&this->cout_mx, NULL);
  pthread_cond_init(&pthreads_avail_cv, NULL);
}

SharedResources::~SharedResources()
{
	pthread_mutex_destroy(&this->file_mx);
	pthread_mutex_destroy(&this->map_mx);
	pthread_mutex_destroy(&this->queue_mx);
	pthread_mutex_destroy(&this->cout_mx);
  pthread_cond_destroy(&pthreads_avail_cv);
	std::cout << " ~ SharedResources Destructor ~" << std::endl;
}
// char * port_num, std::string folder_dir, int timeout = 300
DFS::DFS(char * port_num, std::string folder_dir, int timeout)
{
  shared_ = new struct SharedResources();
  for(int i = 0; i < 8; i++) {
    shared_->pthread_queue.push(&pthread_id_[i]);
  }
	std::cout << " ~ DFS Constructor ~" << std::endl;
  buffer = (char*) malloc (kBufferSize);
	bzero((char *) &server_addr_, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET;
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_.sin_port = htons(atoi(port_num));
	timeout_.tv_sec = timeout; // for listening socket (in seconds)
	timeout_.tv_usec = 0;
  folder_.assign(folder_dir);
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
    std::cout << "dfs.conf opened" << std::endl;
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
  std::stringstream string_stream(request->request_line);
  std::string token = "";
  std::string * str_p;
  char * char_p;
  char cstring[256] = {0};
  std::getline(string_stream, token, ','); //  get command from DFC
  strcpy(cstring,token.c_str()); // switch to cstring
  char_p = strtok(cstring, " "); // if parameter, should be separated by space
  if (char_p != NULL)
    request->method.assign(char_p); // buffer username
  else {
    perror("request->method.assign() failed, char_p == NULL");
    return false;
  }
  str_p = &request->method;
  if ((*str_p != "list") && (*str_p != "get") && (*str_p != "put"))
    return false;
  if (*str_p != "list") {
    char_p = strtok(NULL, " ");  // get password
    if (char_p != NULL)
      request->parameter.assign(char_p); // buffer username
    else {
      perror("request->parameter.assign() failed, char_p == NULL");
      return false;
    }
  }
  token.clear();
  std::getline(string_stream, token, ',');
  request->user.assign(token);
  token.clear();
  std::getline(string_stream, token, ',');
  request->pass.assign(token);
  request->folder.assign(folder_);
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
  pthread_t * pthread_ptr;
	struct sockaddr_in client_address; // client addr
	socklen_t addr_length;
	char * buffer = (char*) malloc (kBufferSize);
	char invalid_msg[] = "Invalid Username/Password. Please try again.";
  char bad_request_msg[] = "Invalid request. Please try another request.";
  char ok_msg[] = "ok";
	std::cout << " ~ Listening for clients ~" << std::endl;

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
              request.request_line.assign(buffer, bytes_received); // reques
              request.clients_sd = i; // client socket descriptor
              request.client_addr = client_address; // sockaddr_in
              request.addr_len = addr_length; // socklen_t
              pthread_mutex_lock(&shared_->cout_mx); // LOCK cout_mx
              std::cout << "new message: \"" << request.request_line <<
                  "\"" << std::endl;
              pthread_mutex_unlock(&shared_->cout_mx); // UNLOCK cout_mxt
              if (ProcessDFSRequest(&request) != true) {
                send(i, bad_request_msg, sizeof(bad_request_msg), 0);
                continue;
              }
              //== CHECK CREDENTIALS
              if (user_pass_map_.count(request.user) == 1) {
                // user exists and passwords match
                if (strcmp(request.pass.c_str(), user_pass_map_[request.user].c_str()) == 0) {
                  pthread_mutex_lock(&shared_->cout_mx); // LOCK cout_mx
                  std::cout << "user authenticated" << std::endl;
                  pthread_mutex_unlock(&shared_->cout_mx); // UNLOCK cout_mx
                  // send acknowledgement
                  send(i, ok_msg, sizeof(ok_msg), 0); // include \0
                  memset(buffer, '0', kBufferSize);
                  // get acknowledgement
                  recv(i, buffer, kBufferSize, 0);
                }
                // user exists, but password doesn't match
                else {
                  send(i, invalid_msg, sizeof(invalid_msg), 0);
                  continue; // move on to next client
                }
              }
              // user doesn't exist
              else {
                send(i, invalid_msg, sizeof(invalid_msg), 0);
                continue; // move on to next client
              }
              pthread_mutex_lock(&shared_->queue_mx);
              if (shared_->num_avail_pthreads_ == 0)
                pthread_cond_wait(&shared_->pthreads_avail_cv, &shared_->queue_mx);
              shared_->num_avail_pthreads_ -= 1;
              pthread_ptr = shared_->pthread_queue.front(); // get available pthread
              shared_->pthread_queue.pop();
              request.pthread_ptr = pthread_ptr; // get available pthread
              shared_->request_queue.push(request);
              if (pthread_create(request.pthread_ptr, NULL, PThread, shared_) != 0) {
                  perror("pthread_create() failed");
                  exit(EXIT_FAILURE);
              }
              // start PThread, then unlock
              pthread_mutex_unlock(&shared_->queue_mx);
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
  // make sure all PThreads exit
  for (i=0; i<kMaxNumDFSThreads; i++)
    pthread_join(pthread_id_[i], &pthread_result_p_);
	free(buffer);
}
} // namespace networking_dfs
