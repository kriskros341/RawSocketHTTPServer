#include "Server.h"
#include "iostream"


struct responseModel {
	string proto;
	//for now will do
	int code;
	string status;

	map<string, string> headers;
	string body;

	string parse();
};


string responseModel::parse() {
	string result;
	result = proto + " " + ::std::to_string(code) + " " + status + "\r\n";
	for(auto x: headers) {
		result += x.first + ": " + x.second + "\r\n";
	}
	result += "\r\n" + body;
	return result;
};

responseModel NOT_IMPLEMENTED = {"HTTP/1.1", 501, "Not Implemented", {}, "Not Implemented"};

int getaddrinfo(
	const char *node, // e.g. "www.example.com" or IP
	const char *service, // e.g. "http" or port number
	const struct addrinfo *hints,
	struct addrinfo **res
);


struct requestModel {
	Method method;
	string proto;
	string path;
	string path_params;
	map<string, string> headers;
	string body;
};


enum class Method {
	GET = 1,
	POST = 2,
	PUT = 3,
	DELETE = 4,
	HEAD = 5,
};


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


// get sockaddr, but protocol agnostic
void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		//handle ipv4
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	//handle ipv6
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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


void initializeMimeMap() {
	mimetype_map["default"] = "text/plain";
	mimetype_map["html"] = "text/html";
	mimetype_map["css"] = "text/css";
	mimetype_map["js"] = "text/js";
	mimetype_map["jpeg"] = "image/jpeg";
	mimetype_map["png"] = "image/png";
}


int parseRequest(string request, requestModel &result) {
	map<string, string> headers;
	::std::regex firstLineR = ::std::regex(R"(^(.*)\S)");
	::std::regex headersR = ::std::regex(R"([A-Z].*: (.*)\S)");
	::std::regex bodyR = ::std::regex(R"((.&)$)");
	std::smatch firstLineM;
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

	string proto = firstLine.substr(methodEndsAt + 1 + pathEndsAt + 1); 
	// cumulated offset
	auto headers_begin = std::sregex_iterator(request.begin(), request.end(), headersR);
	auto headers_end = std::sregex_iterator();
	int length = distance(headers_begin, headers_end);
	string lastHeader;
	for(std::sregex_iterator i = headers_begin; i != headers_end; i++) {
		lastHeader = (*i).str();
		int colonIndex = lastHeader.find(":");
		headers[lastHeader.substr(0, colonIndex)] = lastHeader.substr(colonIndex+1);
	}
	string body;
	std::smatch bodyM;
	regex_search(request, bodyM, bodyR);
	if(!(bodyM.str() == lastHeader)) { //I'm that lazy
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


// Populates content string with the file content
int loadFile(string path, string &content) {
	std::ifstream theFile; //ifstream read, ofstream write
	std::stringstream buff;
	theFile.open(path.c_str()); 
	// PÓŁ GODZINY NIE DZIALALO ZAPOMNIAŁEM ŻE TRZEBA OTWORZYĆ PLIK WTF
	if(theFile.good()) {
		buff << theFile.rdbuf();
		content = buff.str();
		return 0; //success
	}
	return -1; //failure
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


void SocketServer::bindToLocalAddress() {
	int addrinfoStatus;
	//Tłumaczenie tla peasantów od pythona i typescripta jak ja
	//Node == IP
	//Null znaczy że tylko localhost może się połączyć. NAWET 127.0.0.1 i [::1] nie zadziałają
	//Service == PORT
	//hints == OPTIONS
	//servinfo == ??
	if((addrinfoStatus = getaddrinfo("0.0.0.0", std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
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


void SocketServer::setup() {
	hints.ai_family = AF_UNSPEC; //Accept both IPV4 and IPV6
	hints.ai_socktype = SOCK_STREAM; //Accept TCP
	hints.ai_flags = AI_PASSIVE;  //Do not initiate connection.
	memset(&hints, 0, sizeof hints);
	bindToLocalAddress();
	printf("server: waiting for connections to %d...\n", this->port);
};


int SocketServer::mainLoop() {
	if(listen(serverSocketFileDescriptor, foreignSocketFileDescriptor) == -1) {
		perror("listen");
		exit(1);
	};
	while(true) {
		sin_size = sizeof their_addr;
		foreignSocketFileDescriptor = accept(serverSocketFileDescriptor, (struct sockaddr *)&their_addr, &sin_size);
		if(!fork()) {
			std::string incomingData;
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
				incomingData += buff;
			}
			handleIncomingData(incomingData);
		}
		close(foreignSocketFileDescriptor); // done using it
	}
	close(serverSocketFileDescriptor); // no need to listen anymore
	return 0;
};

template <typename Context>
void Server<Context>::on(Method method, string path, handlerFunction<Context> handler){
	string m = parseMethod(method);
	std::cout << "initializing " << m << " endpoint " << path.substr(1) << std::endl;
	endpoints[method][path.substr(1)] = handler;
};


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

template <typename Context>
void Server<Context>::getEndpointsFromDirectory(string path) {
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
					// ignorujemy . i .. obecne w każdym folderze
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

template <typename Context>
void Server<Context>::createGetFileEndpoint(string path) {
	translatePath(path);
	std::cout << "initializing GET endpoint " << path << std::endl;
	endpoints[Method::GET][path] = GETFileHandler; //it
}


template <typename Context>
responseModel Server<Context>::GETFileHandler(requestModel request, Context context) {
	responseModel response;
	string fileContent;
	translatePath(request.path);
	string mimetype = getMimeType(request.path);
	response.proto = request.proto;
	if(loadFile(request.path, fileContent) == 0) {
		response.code = 200;
		response.status = "OK";
		response.headers["Server"] = "MyServer!";
		response.headers["Connection"] = "Keep-Alive";
		response.headers["Content-Type"] = mimetype;
		response.body = fileContent;
	}
	else {
		response.code = 404;
		response.status = "NOT FOUND";
	}
	return response;
}


template <typename Context>
void Server<Context>::handleIncomingData(std::string incomingData) {
	string resp;
	struct requestModel request;
	if(parseRequest(incomingData, request) == -1) {
		//REDUCE THE AMOUNT OF ENDPOINTS PLZ
		resp = HTTP_BADREQUEST;
		
		if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
			perror("send");
			return;
		};
	};
	if(endpoints[request.method][request.path] != 0) {
		resp = endpoints[request.method][request.path](request, context).parse();
	} else {
		resp = HTTP_NOTFOUND;
	}
	if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
		perror("send");
	};
}
	
