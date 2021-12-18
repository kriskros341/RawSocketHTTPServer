#include <stdio.h>
#include <iostream>
#include <string.h>
#include <map>
#include "stdint.h"

// a header with inet functions
#include <sys/types.h>
#include <sys/socket.h>

//close() filedescriptor fn is in
#include <unistd.h>
// better error handling with gai
#include <netdb.h>
#define BACKLOG 10 // how many pending connections queue will hold
#define MYPORT "80" // the port users will be connecting to
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/wait.h>
//pre termination cleanup;
#include <signal.h>
#include "sys/stat.h"
//handling files
#include <fstream>
#include <sstream>

using namespace std;

int getaddrinfo(
		const char *node, // e.g. "www.example.com" or IP
		const char *service, // e.g. "http" or port number
		const struct addrinfo *hints,
		struct addrinfo **res
		);


// get sockaddr, but protocol agnostic
void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		//check if is ipv4
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int yes = 1;

// a fast way to check if a file with given path exists.
bool exists (string &filename) {
  struct stat buffer;   
	return (stat (filename.c_str(), &buffer) == 0);
}

//Populates content string with the file content
int loadFile(string path, string &content) {
	ifstream theFile; //ifstream read, ofstream write
	stringstream buff;
	theFile.open(path.c_str()); // PÓŁ GODZINY ZAPOMNIAŁEM ŻE TRZEBA OTWORZYĆ PLIK WTF
	buff << theFile.rdbuf();
	content = buff.str();
	return 0; //success
}

//Checks path. 
//if it's a folder increments path by index.html and checks for it, 
//if it's not then it populates content string with file's data.
bool isThereSuchFile(string &path, string &content) {
  struct stat buffer;
	if(stat (path.c_str(), &buffer) == 0) {
		if(buffer.st_mode & S_IFDIR) {
			//handle dir
			if(path.back() == '/') {
				path += "index.html";
			} else {
				path += "/index.html";
			}
			return isThereSuchFile(path, content);
		}
		else if(buffer.st_mode & S_IFREG) {
			//handle a file
			loadFile(path, content);
			return true;
		}
		else {
			//handle other
			return false;
		}
	};
	return false;
}

map<string, string> mimetype_map;

void initializeMimeMap() {
	mimetype_map["default"] = "text/html";
	mimetype_map["html"] = "text/html";
	mimetype_map["css"] = "text/css";
	mimetype_map["js"] = "text/js";
	mimetype_map["jpeg"] = "image/jpeg";
	mimetype_map["png"] = "image/png";
}

//Gets the MIME type of specified resource
string getMimeType(string filename) {
	string extension = filename.substr(filename.find('.')+1);
	return strlen(mimetype_map[extension].c_str()) != 0 ? 
		mimetype_map[extension] 
		: 
		mimetype_map["default"];
};

class Server {
	public:
		int serverSocketFileDescriptor, foreignSocketFileDescriptor;
		int port = MYPORT;
		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage their_addr;
		socklen_t sin_size;
		struct sigaction sa;
		char s[INET6_ADDRSTRLEN];
		int rv;
		void bindToLocalAddress() {
			if((rv = getaddrinfo(NULL, this->port, &hints, &servinfo)) != 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); 
			}
			for(p = servinfo; p != NULL; p = p->ai_next) {
				if((serverSocketFileDescriptor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
					perror("server: socket");
					continue;
				}
				//set socket options: should reuse: 1 (yes)
				if(setsockopt(serverSocketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
					perror("setsockopt");
					exit(1);
				}
				if(bind(serverSocketFileDescriptor, p->ai_addr, p->ai_addrlen) == -1) {
					close(serverSocketFileDescriptor);
					perror("server: bind");
					continue;
				}
				break;
			}
			freeaddrinfo(servinfo);
			if (p == NULL) {
				perror("listen");
				exit(1);
			}
			if (listen(serverSocketFileDescriptor, BACKLOG) == -1) {
				perror("listen");
				exit(-1);
			}
		}
		int mainLoop() {
			if(listen(serverSocketFileDescriptor, foreignSocketFileDescriptor) == -1) {
				perror("listen");
				exit(1);
			};
			while(true) {
				sin_size = sizeof their_addr;
				foreignSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr *)&their_addr, &sin_size);
				

				if(!fork()) {
					if (foreignSocketFileDescriptor == -1) {
						perror("accept");
						cout << foreignSocketFileDescriptor << endl;
						continue;
					}
					inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
					printf("server: got connection from %s\n", s);
					

					string request;
					char buff[8096];
					int r;
					if((r = read(foreignSocketFileDescriptor, buff, 8096)) > 0) {
						//TO IMPROVE
						if(r == -1) {
							perror("read");
							break;
						}
						request += buff;
					}
					int methodEndsAt = request.find(' ');
					string method = request.substr(0, methodEndsAt);
					string path = request.substr(methodEndsAt+1, request.substr(methodEndsAt+1).find(' ')); // PATH;
					if(method == "GET") {
						//prepareGetResponse
						string resp;
						string fileContent;
						if(path == "/") {
							path = "index.html";
						}
						if(path.at(0) == '/') {
							path = path.substr(1);
						}
						if(isThereSuchFile(path, fileContent)) {
							string mimetype = getMimeType(path);
							resp = 
								"HTTP/1.1 200 OK\r\n"
								"Server: MyServer!\r\n"
								"Connection: Close\r\n"
								"Content-Type: "+mimetype+"\r\n"
								"\r\n" + fileContent;
						}
						else {
							resp = "HTTP/1.1 404 Not Found\r\n\r\n <div>404 Not Found</div>";
						}
						int respsize = resp.length();
						if(send(foreignSocketFileDescriptor, resp.c_str(), respsize, 0) == -1) {
							perror("send");
						}
					}

				}
				close(foreignSocketFileDescriptor); // done using it
			}
			close(serverSocketFileDescriptor); // no need to listen anymore
			return 0;
		}
		Server() {
			//setup
			hints.ai_family = AF_UNSPEC; //Accept both IPV4 and IPV6
			hints.ai_socktype = SOCK_STREAM; //Accept TCP
			hints.ai_flags = AI_PASSIVE;  //Do not initiate connection.
			memset(&hints, 0, sizeof hints);
			bindToLocalAddress();
			printf("server: waiting for connections to %s...\n", this->port);
		}		
};



int main(int argc, char *argv[]) {
	initializeMimeMap();
	Server s;
	s.port = 80;
	//s.on("GET", path, function)
	s.mainLoop();
	// s.on("POST").add({})
	return 0;
};

