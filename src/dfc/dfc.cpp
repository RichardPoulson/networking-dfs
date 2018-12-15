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
https://stackoverflow.com/questions/49764841/c-stdmap-get-the-key-at-a-specific-offset
==============================================================================*/

#include "dfc.h"

#include <iostream>

namespace networking_dfs_dfc {

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
  buffer_ = (char*) malloc (kBufferSize); // allocate memory for buffer
	LoadConfigFile(); // server names + addresses, and username + password
  CreateSocket(); // create socket used for communicating with DFS
	StartDFCService();
}

DFC::~DFC() {
  std::cout << "~ DFC Destructor ~" << std::endl;
  close(sockfd_);
  delete [] buffer_;
}

bool DFC::BindSocket(std::string address_string) {
  char * pch; // pointer to character
  char cstring[256] = {0};
  strcpy(cstring,address_string.c_str()); // server name and port
  pch = strtok(cstring, ":"); // space is delimiter
  pch = strtok(NULL, " ");  // ignore "Server"
  unsigned short int port = atoi(pch);
  // set
  memset(&serv_addr_, '0', sizeof(serv_addr_));
  serv_addr_.sin_family = AF_INET; // IPv4
  serv_addr_.sin_port = htons(port);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr_.sin_addr)<=0) {
    perror("inet_pton() failed");
    close(sockfd_);
    return false;
  }
  if (connect(sockfd_, (struct sockaddr *)&serv_addr_, sizeof(serv_addr_)) < 0) {
    perror("connect() failed");
    close(sockfd_);
    return false;
  }
  std::cout << " ~ socket connected to port \"" << port << "\"." << std::endl;
  return true;
}

bool DFC::CreateSocket() {
  if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    return false;
  }
  std::cout << " ~ socket created ~" << std::endl;
  return true;
}

void DFC::HandleInput(std::string input) {
  struct HashStruct * hash = new struct HashStruct();
  HashMessage(input, hash);
  int remainder = hash->ull % kNumDFSServers;
  BindSocket("127.0.0.1:10001");

	//UploadFile(input);
}

void DFC::HashMessage(std::string message, struct HashStruct * hash_struct) {
	MD5_CTX md5;
	int i;
	std::string result;
	result.reserve(32);  // C++11 only, otherwise ignore
	unsigned char buffer_md5[16];
	unsigned long long int ull = 0;
	const char* test;
	MD5_Init(&md5);
	test = message.c_str();
	MD5_Update(&md5, (const unsigned char *)test, message.length());
	bzero(buffer_md5, sizeof buffer_md5);
	MD5_Final(buffer_md5, &md5);
	for (std::size_t i = 0; i != 16; ++i)
	{
		result += "0123456789ABCDEF"[buffer_md5[i] / 16];
		result += "0123456789ABCDEF"[buffer_md5[i] % 16];
	}
	hash_struct->str.assign(result);
	hash_struct->ull = std::strtoull(result.substr(0,8).c_str(),NULL,16);
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

void DFC::SendCommand(std::string command_str) {
	BindSocket("127.0.0.1:10001");
  send(sockfd_, command_str.c_str(), command_str.size(), 0);
}

void DFC::StartDFCService() {
	std::string input = "";
	bool continue_prompting = true;
	std::cout << std::endl << "Welcome to the distributed file system!" << std::endl;
	while (continue_prompting) {
		input.clear();
		std::cout << "Please enter a command:  ";
		std::cin >> input;
		HandleInput(input);

		if (input == "exit") {
			continue_prompting = false;
		}
	}
}

void DFC::UploadFile(std::string command_str) {
  char msg[] = "list Alice SimplePassword";
	struct HashStruct * hash = new struct HashStruct();
	memset(hash, 0, sizeof(*hash));
	HashMessage(command_str, hash);
	std::cout << (hash->ull % 4) << std::endl;
	size_t file_size = 0;
	char part1[kBufferSize / 4] = "";
	char part2[kBufferSize / 4] = "";
	char part3[kBufferSize / 4] = "";
	char part4[kBufferSize / 4] = "";
  /*
	FILE * fp = fopen(file.c_str(), "rb"); // open the file in read mode (binary)
	if (fp != NULL) {
		fseek(fp, 0L, SEEK_END); // seek to end in order to get size
		file_size = ftell(fp); // get size of file in bytes
		rewind(fp); // seek back to beginning of file
		fclose(fp);
	}
	else {
		perror("fopen() failed");
		return;
	}
  */
	std::cout << file_size << std::endl;
	BindSocket("127.0.0.1:10001");
  send(sockfd_, msg, sizeof msg, 0);

  /*

  memset(buffer_, '0', kBufferSize);
  size_t received = recv(sockfd_, buffer_, kBufferSize, 0);
  std::cout << buffer_ << std::endl;


	DIR * directory; // used for "ls" command
	struct dirent *dir; // used for "ls" command
	directory = opendir(".");
  if (directory) {
    std::cout << "  Files in your LOCAL directory:" << std::endl;
    while ((dir = readdir(directory)) != NULL) {
      std::cout << "    " << dir->d_name << std::endl;
    }
    std::cout << std::endl;
  }
  */
}

} // namespace networking_dfs_dfc
