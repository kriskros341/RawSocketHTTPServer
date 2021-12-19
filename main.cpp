#include <stdio.h>
#include <iostream>
#include <string.h>
#include <map>
#include <vector>
#include "stdint.h"
#include <dirent.h>
// a header with inet functions
#include <sys/types.h>
#include <sys/socket.h>

#include <regex>
//close() filedescriptor fn is in
#include <unistd.h>
// better error handling with gai
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/wait.h>
//pre termination cleanup;
#include <signal.h>
#include "sys/stat.h"
//handling files
#include <fstream>
#include <sstream>
#define IGNORE_CHAR '.'
#define HTTP_BADREQUEST "HTTP/1.1 400 Bad Request\r\n\r\n <div>400 Bad Request</div>"
#define HTTP_NOTFOUND "HTTP/1.1 404 Not Found\r\n\r\n <div>404 Not Found</div>"
#define BACKLOG 10 // how many pending connections queue will hold
#define MYPORT "80" // the port users will be connecting to
#define DEFAULTSRC "/"
using namespace std;


enum class Method {
	GET = 1,
	POST = 2,
	PUT = 3,
	DELETE = 4,
	HEAD = 5,
};

struct requestModel {
	Method method;
	string proto;
	string path;
	string path_params;
	map<string, string> headers;
	string body;
};

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

Method parseMethod(string method) {
	if(method == "GET") {
		return Method::GET;
	} else if(method == "POST") {
		return Method::POST;
	} else if(method == "PUT") {
		return Method::PUT;
	} else if(method == "DELETE") {
		return Method::DELETE;
	} else if(method == "HEAD") {
		return Method::HEAD;
	} else {
		return Method::HEAD;
	}
}

string parseMethod(Method method) {
	string m;
	switch((int)method) {
		case 1:
			m = "GET";
			break;
		case 2:
			m = "POST";
			break;
		case 3:
			m = "PUT";
			break;
		case 4:
			m = "DELETE";
			break;
		case 5:
			m = "HEAD";
			break;
		default:
			m = "HEAD";
	};
	return m;
};

// a fast way to check if a file with given path exists.

//Populates content string with the file content
int loadFile(string path, string &content) {
	ifstream theFile; //ifstream read, ofstream write
	stringstream buff;
	theFile.open(path.c_str()); // PÓŁ GODZINY NIE DZIALALO ZAPOMNIAŁEM ŻE TRZEBA OTWORZYĆ PLIK WTF
	if(theFile.good()) {
		buff << theFile.rdbuf();
		content = buff.str();
		return 0; //success
	}
	return -1;
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
	mimetype_map["default"] = "text/plain";
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


bool exists (string filename) {
  struct stat buffer;   
	if(stat (filename.c_str(), &buffer) == 0) {
		if(buffer.st_mode & S_IFREG) {
			return true;
		};
	}
	return false;
}


void translatePath(string &path) {
	if(path.substr(0, 2) == "./") {
		//I want to be able to use it both ways, let's see if it's that bad of an idea
		path = path.substr(2);
		return;
	}
	if(path == "/") {
		cout << "e" << endl;
		if(exists("index.html")) {
			path = "index.html";
			return;
		}
	}
	if(path.at(0) == '/') {
		path = path.substr(1);
	}
	if(path.find('.') == -1) {
		if(path.back() == '/') {
			if(exists(path+"index.html")) {
				path += "index.html";
			}
		}
		if(exists(path+"/index.html")) {
			path += "/index.html";
		}
	}
};






//https://regexr.com/
int parseRequest(string request, requestModel &result) {
	map<string, string> headers;
	regex firstLineR = regex(R"(^(.*)\S)");
	regex headersR = regex(R"([A-Z].*: (.*)\S)");
	regex bodyR = regex(R"((.&)$)");
	smatch firstLineM;
	if(!regex_search(request, firstLineM, firstLineR)) {
		return -1;
	}
	string firstLine = firstLineM.str();
	int methodEndsAt = firstLine.find(' ');
	int pathEndsAt = firstLine.substr(methodEndsAt+1).find(' ');
	string method = firstLine.substr(0, methodEndsAt);
	
	string fullPath = firstLine.substr(methodEndsAt+1, pathEndsAt);
	int qindex = fullPath.find("?");
	string path = fullPath.substr(0, fullPath.find("?"));
	translatePath(path);
	string path_params = fullPath.substr(fullPath.find("?")+1);

	string proto = firstLine.substr(methodEndsAt + 1 + pathEndsAt + 1); // cumulated offset
	
	
	auto headers_begin = sregex_iterator(request.begin(), request.end(), headersR);
	auto headers_end = std::sregex_iterator();
	int length = distance(headers_begin, headers_end);
	//cout << "items: " << length << endl;
	string lastHeader;
	for(sregex_iterator i = headers_end; i != headers_end; i++) {
		string h = (*i).str();
		int colonIndex = h.find(":");
		lastHeader = h;
		headers[h.substr(0, colonIndex)] = h.substr(colonIndex+1);
		//cout << h.substr(0, colonIndex) << "---" << h.substr(colonIndex+2) << endl;
	}
	string body;
	smatch bodyM;
	regex_search(request, bodyM, bodyR);
	if(!(bodyM.str() == lastHeader)) {
		body = bodyM.str();
	}

	result.method = parseMethod(method);
	result.path = path;
	result.path_params = path_params;
	result.headers = headers;
	result.proto = proto;
	result.body = body;
	return 0;
}


class Server {
	public:
		int serverSocketFileDescriptor, foreignSocketFileDescriptor;
		string port = MYPORT;
		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage their_addr;
		socklen_t sin_size;
		struct sigaction sa;
		char s[INET6_ADDRSTRLEN];
		string directory = DEFAULTSRC;
		map<Method, map<string, string (*)(requestModel request)>> endpoints;


		static string GETFileHandler(requestModel request);
		void createGetFileEndpoint(string path);
		void getEndpointsFromDirectory(string pathToDirectory);
		void bindToLocalAddress();
		void on(Method method, string path, string (*)(requestModel request));
		int mainLoop();
		Server() {
			//setup
			hints.ai_family = AF_UNSPEC; //Accept both IPV4 and IPV6
			hints.ai_socktype = SOCK_STREAM; //Accept TCP
			hints.ai_flags = AI_PASSIVE;  //Do not initiate connection.
			memset(&hints, 0, sizeof hints);
			bindToLocalAddress();
			printf("server: waiting for connections to %s...\n", this->port.c_str());
		}		
};



int Server::mainLoop() {
	if(listen(serverSocketFileDescriptor, foreignSocketFileDescriptor) == -1) {
		perror("listen");
		exit(1);
	};
	while(true) {
		sin_size = sizeof their_addr;
		foreignSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr *)&their_addr, &sin_size);
		if(!fork()) {
			string resp;
			string request_string;
			char buff[8096];
			int bytesRead;
			if(foreignSocketFileDescriptor == -1) {
				perror("accept");
				continue;
			}
			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);
			if((bytesRead = read(foreignSocketFileDescriptor, buff, 8096)) > 0) {
				//TO IMPROVE
				if(bytesRead == -1) {
					perror("read");
				}
				request_string += buff;
			}
			struct requestModel request;
			if(parseRequest(request_string, request) == -1) {
				//REDUCE THE AMOUNT OF ENDPOINTS PLZ
				resp = HTTP_BADREQUEST;
				if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
					perror("send");
				};
				continue;
			};
			
			if(endpoints[request.method][request.path] != 0) {
				resp = endpoints[request.method][request.path](request);
			} else {
				resp = HTTP_NOTFOUND;
			}
			if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
				perror("send");
			};
		}
		close(foreignSocketFileDescriptor); // done using it
	}
	close(serverSocketFileDescriptor); // no need to listen anymore
	return 0;
}


string Server::GETFileHandler(requestModel request) {
	string response;
	string fileContent;
	translatePath(request.path);
	string mimetype = getMimeType(request.path);
	if(loadFile(request.path, fileContent) == 0) {
		response = 
			"HTTP/1.1 200 OK\r\n"
			"Server: MyServer!\r\n"
			"Connection: Kepp-Alive\r\n"
			"Content-Type: "+mimetype+"\r\n"
			"\r\n" + fileContent;
		return response;
	};
	return HTTP_NOTFOUND;
}

/*
Old version that checks endpoints itself
string Server::GETFileHandler(string request) {
	string resp;
	string fileContent;
	int methodEndsAt = request.find(' ');
	string method = request.substr(0, methodEndsAt);
	string path = request.substr(methodEndsAt+1, request.substr(methodEndsAt+1).find(' ')); // PATH;
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
		resp = HTTP_NOTFOUND;
	}
	return resp;
}
*/

void Server::bindToLocalAddress() {
	int addrinfoStatus;
	if((addrinfoStatus = getaddrinfo(NULL, this->port.c_str(), &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addrinfoStatus)); 
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

void handleFile() {
}

string getFilenameFromPath(string path) {
	int len = strlen(path.c_str());
	string result;
	string result2;
	for(int i = path.size(); i > 0; i--) {
		if(path[i] == '/') {
			break;
		}
		result += path[i];
	}
	int l = result.size();
	for(int i{}; i != l; i++) {
		result2 += result[l - i - 1];
	}
	return result2;
}



void Server::createGetFileEndpoint(string path) {
	translatePath(path);
	cout << "initializing GET endpoint " << path << endl;
	endpoints[Method::GET][path] = GETFileHandler;
	//cout << endpoints[Method::GET][path](path) << endl;
	/*
		endpoints[path](path)
	*/
}


void Server::getEndpointsFromDirectory(string path) {
	DIR *directory;
	struct dirent *entry;
	struct stat statBuffer;
	if(stat (path.c_str(), &statBuffer) == 0) {
		if((directory = opendir(path.c_str()))) {
			while((entry = readdir(directory))) {
				if(
						strcmp(entry->d_name, "..") != 0 && 
						strcmp(entry->d_name, ".") != 0 &&
						entry->d_name[0] != IGNORE_CHAR
					) {
					// strcmp cuz compiler complains about === 
					// Given path leads to a directory. let the fun begin ^^
					getEndpointsFromDirectory(path+"/"+entry->d_name);
				}
			}
			closedir(directory);
		} else if(statBuffer.st_mode & S_IFREG) {
			// Given path leads to a file
			if(getFilenameFromPath(path)[0] != IGNORE_CHAR) {
				createGetFileEndpoint(path);
			}
			return;
		}
		else {
			//Given path doesn't lead to neither file, nor directory
			return;
		}
	}	
	//Such item doesn't exist
}

typedef string (*Handler)(requestModel request);




void Server::on(Method method, string path, Handler handler){
	string m = parseMethod(method);
	cout << "initializing" << m << " endpoint " << path.substr(1) << endl;
	endpoints[method][path.substr(1)] = handler;
};

string testEndpoint(requestModel request) {
	cout << "testing the endpoint" << endl;
	return "a";
};

int main(int argc, char *argv[]) {
	initializeMimeMap();
	//cout << __cplusplus << endl; //14
	Server s;
	s.getEndpointsFromDirectory(".");
	s.on(Method::POST, "/endpoint", testEndpoint);
	//s.on("GET", "PATH", function)
	//s.includeDirectory("Path")
	s.mainLoop();
	// s.on("POST").add({})
	return 0;
};

