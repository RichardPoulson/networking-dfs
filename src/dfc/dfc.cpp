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
https://stackoverflow.com/questions/7856453/accessing-map-value-by-index

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

HashStruct::HashStruct() {
	str = "";
	ull = 0;
}
HashStruct::~HashStruct() {}

DFC::DFC()
{
  std::cout << "~ DFC Constructor ~" << std::endl;
  for (int i=0; i<4; i++)
    buffer_[i] = (char*) malloc (kBufferSize / 4);
	LoadConfigFile(); // server names + addresses, and username + password
	StartDFCService();
}

DFC::~DFC() {
  std::cout << "~ DFC Destructor ~" << std::endl;
  close(sockfd_);
  for (int i=0; i<4; i++)
    delete [] buffer_[i];
}

bool DFC::CreateBindSocket(std::string address_string) {
  char * pch; // pointer to character
  char cstring[256] = {0};
  strcpy(cstring,address_string.c_str()); // server name and port
  pch = strtok(cstring, ":"); // space is delimiter
  pch = strtok(NULL, " ");  // ignore "Server"
  unsigned short int port = atoi(pch);
  if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    return false;
  }
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
    close(sockfd_);
    return false;
  }
  std::cout << " ~ socket created and connected to port \"" << port << "\"." << std::endl;
  return true;
}

int DFC::GetFileSize(std::string file_name) {
  std::ifstream ifs;
  std::filebuf * file_buf_ptr; // file buffer pointer, used for getting file size
  int num_bytes;
  ifs.open(file_name, std::ifstream::binary);
  if(ifs.is_open()) {
    file_buf_ptr = ifs.rdbuf();
    num_bytes = file_buf_ptr->pubseekoff(0, ifs.end, ifs.in);
    file_buf_ptr->pubseekpos(0, ifs.in); // go back to beginning
    ifs.close();
    return num_bytes;
  }
  else {
    perror("GetFileSize() failed");
    return -1;
  }
}

void DFC::HandleInput(std::string input) {
  struct HashStruct * hash = new struct HashStruct();
  std::ifstream file_ifs;
  std::ofstream file_ofs;
  int i, partition, remainder;
  int num_bytes = 0;
  std::string method, filename;
  char cstring[256] = {0};
  char files[4][256] = {0};
  char * pch;
  strcpy(cstring,input.c_str()); // copy string to cstring
  pch = strtok(cstring, " "); // first get username
  method.assign(pch);
  if (method != "list") {
    pch = strtok(NULL, " ");  // get password
    if(pch != NULL)
      filename.assign(pch);
  }
  // CLEAR ALL BUFFERS
  for(i=0; i<4; i++)
    memset(buffer_[i], '0', kBufferSize / 4);
  strcpy(buffer_[0], input.c_str());
  strcat(buffer_[0], ",");
  strcat(buffer_[0], user_.c_str());
  strcat(buffer_[0], ",");
  strcat(buffer_[0], password_.c_str());
  strcat(buffer_[0], "\0");

  HashMessage(filename.c_str(), hash);
  remainder = hash->ull % kNumDFSServers;
  if (method == "list") {
    map_it = server_map_.begin();
    for(i=1; i<4; i++)
      strcpy(buffer_[i], buffer_[0]);
    std::cout << "list" << std::endl;
    for (int i=0; i<4; i++) {
      if (CreateBindSocket(map_it->second)) { // if connected ok
        send(sockfd_, buffer_[i], strlen(buffer_[i])+1, 0);
        memset(buffer_[i], '0', kBufferSize / 4);
        num_bytes = recv(sockfd_, buffer_[i], kBufferSize / 4, 0);
        if (strcmp(buffer_[i], "ok") != 0) {
          // server didn't okay credentials, print server message
          std::cout << buffer_[i] << std::endl;
          close(sockfd_);
        }
        else {
          send(sockfd_, buffer_[i], num_bytes, 0); // send "ok" back
          memset(buffer_[i], '0', kBufferSize / 4);
          num_bytes = recv(sockfd_, buffer_[i], kBufferSize / 4, 0);
          std::cout << buffer_[i] << std::endl;
          close(sockfd_);
        }
      }
      map_it++;
    }
  }
  else if (method == "get") {

  }
  else if (method == "put") {
    std::cout << filename << "..\n";
    num_bytes = GetFileSize(filename);
    std::cout << num_bytes << std::endl;
    if(num_bytes > 0 ) {
      partition = num_bytes / 4;
      remainder = num_bytes - (3 * partition);
      std::cout << num_bytes << ", " << partition << ", " << remainder << std::endl;
      /*
      file_ofs.open(filename.c_str(), std::ofstream::binary);
      if (file_ofs.is_open()) {
        file_ofs.write(buffer_[0], partition);
        //file_ofs.write(buffer_[1], partition);
        //file_ofs.write(buffer_[2], partition);
        //file_ofs.write(buffer_[3], remainder);
        file_ofs.close();
      }
      */
    }
  }
  else {

  }
  //*/
  close(sockfd_);
  delete hash;




  //UploadFile(input);
  //*/

}

void DFC::HashMessage(std::string message, struct HashStruct * hash_struct) {
	MD5_CTX md5;
	std::string result;
	result.reserve(32);  // C++11 only, otherwise ignore
	unsigned char buffer_md5[16];
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
		std::cout << "User: " << user_ << std::endl << "Pass: " << password_
        << std::endl;
    config_file.close();
		return true;
  }
  else std::cout << "Unable to open file";
	return false;
}

void DFC::SendCommand(std::string command_str) {
	CreateBindSocket("127.0.0.1:10001");
  send(sockfd_, command_str.c_str(), command_str.size(), 0);
}

void DFC::StartDFCService() {
	std::string input;
	bool continue_prompting = true;
	std::cout << std::endl << "Welcome to the distributed file system!" << std::endl;
	while (continue_prompting) {
    input.clear();
		std::cout << "Please enter a command:  ";
    std::getline(std::cin, input);
    if (input == "exit") {
			continue_prompting = false;
		}
    else {
      HandleInput(input);
    }
	}
}

void DFC::UploadFile(std::string command_str) {
  char msg[] = "list,Alice,SimplePassword";
	struct HashStruct * hash = new struct HashStruct();
	memset(hash, 0, sizeof(*hash));
	HashMessage(command_str, hash);
	std::cout << (hash->ull % 4) << std::endl;
	size_t file_size = 0;

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
