/*==============================================================================
Name        : dfc.cpp
Author      : Richard Poulson
Version     : 1.0
Last edit   : 12/2/2018

Description :

https://stackoverflow.com/questions/8257432/how-to-return-the-md5-hash-in-a-string-in-this-code-c
https://stackoverflow.com/questions/1070497/c-convert-hex-string-to-signed-integer
http://www.cplusplus.com/reference/string/stoul/
https://en.wikipedia.org/wiki/MD5
http://www.cplusplus.com/reference/string/string/
==============================================================================*/

#include "dfc.h"

#include <iostream>

namespace networking_dfs_dfc {

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

HashStruct::HashStruct() {
	str = "";
	ull = 0;
}
HashStruct::~HashStruct() {}

DFC::DFC()
{
	std::cout << "~ DFC Constructor ~" << std::endl;
  bzero((char *) &server_addr_, sizeof(server_addr_));
	server_addr_.sin_family = AF_INET; // Address Family
	// socket bound to all local interfaces
	server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
	LoadConfigFile(); // server names + addresses, and username + password
	StartDFCService();
}

DFC::~DFC() {
	std::cout << "~ DFC Destructor ~" << std::endl;
}

bool DFC::CreateBindSocket() {
	std::cout << "Entered Createbindsocket()" << std::endl;
	/*
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
  //==  set port to be non-blocking
	// set listen socket to be non-binding
	if (ioctl(listen_sd_, FIONBIO, (char *)&on_) < 0) {
		perror("ioctl() failed");
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
	  */
	return true;
}

bool DFC::LoadConfigFile() {
	std::string line;
	std::string name;
	std::string address;
	char cstring[256];
	char * pch;
  std::ifstream config_file("dfc.conf");
  if(config_file.is_open()) {
		// map 4 server names/folders to network addresses
		for (int i=0; i<4; i++) {
			getline(config_file, line);
			bzero(cstring, sizeof cstring);
			strcpy(cstring,line.c_str()); // copy string to cstring
			pch = strtok(cstring, " "); // space is delimiter
      while (pch != NULL) // go through the line
      {
				pch = strtok(NULL, " ");  // ignore "Server"
				name.assign(pch);
	      pch = strtok(NULL, " ");  // get folder for server
				address.assign(pch);
				pch = strtok(NULL, " ");  // get address for server
				server_map_[name] = address; // map folder
				std::cout << name << ": " << server_map_[name] << std::endl;
      }
		}
		getline(config_file, line);
		bzero(cstring, sizeof cstring);
		strcpy(cstring,line.c_str()); // copy string to cstring
		pch = strtok(cstring, " "); // get "Username:" and discard
    pch = strtok(NULL, " ");  // get username
		user_.assign(pch); // assign username
		getline(config_file, line);
		bzero(cstring, sizeof cstring);
		strcpy(cstring,line.c_str()); // copy string to cstring
		pch = strtok(cstring, " "); // get "Password:" and discard
    pch = strtok(NULL, " ");  // get password
		password_.assign(pch);
		std::cout << "User: " << user_ << " , Pass: " << password_ << std::endl;
    config_file.close();
		return true;
  }
  else std::cout << "Unable to open file";
	return false;
}

void DFC::StartDFCService() {
	MD5_CTX md5;
	int i;
	std::string result;
	result.reserve(32);  // C++11 only, otherwise ignore
	unsigned char buffer_md5[16];
	unsigned long long int ull = 0;
	std::string input = "";
	const char* test;
	bool continue_prompting = true;
	std::cout << std::endl << "Welcome to the distributed file system!" << std::endl;
	while (continue_prompting) {
		MD5_Init(&md5);
		input.clear();
		result.clear();
		std::cout << "Please enter a command:  ";
		std::cin >> input;
		test = input.c_str();
		MD5_Update(&md5, (const unsigned char *) test, input.length());

		bzero(buffer_md5, sizeof buffer_md5);
		MD5_Final(buffer_md5, &md5);
		for (std::size_t i = 0; i != 16; ++i)
		{
			result += "0123456789ABCDEF"[buffer_md5[i] / 16];
			result += "0123456789ABCDEF"[buffer_md5[i] % 16];
		}
		std::cout << result << std::endl;
		ull = std::strtoull(result.c_str(),NULL,0);
		std::cout << "You entered: " << ull << '\n';
		//long unsigned int x = std::stoul(&result, nullptr, 16);

		/*
		result.reserve(32);  // C++11 only, otherwise ignore
		for (std::size_t i = 0; i != 16; ++i)
		{
			result += "0123456789ABCDEF"[buffer_md5[i] / 16];
			result += "0123456789ABCDEF"[buffer_md5[i] % 16];
		}
		std::cout << result << std::endl;
		*/
		if (input == "exit") {
			continue_prompting = false;
		}
	}

}
} // namespace networking_dfs_dfc
