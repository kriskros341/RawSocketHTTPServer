#include "Server.h"
#include "iostream"
#include "regex"
#include <thread>


struct responseModel {
	string proto;
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
responseModel NOT_FOUND = {"HTTP/1.1", 404, "Not Found", {}, "<div>Not Found</div>"};

struct requestModel {
	Method method;
	string proto;
	string path;
	map<string, string> path_params;
	map<string, string> headers;
	string body;
};

enum class Method {
	GET = 1,
	POST = 2,
	PUT = 3,
	DELETE = 4,
	HEAD = 5,
	OPTIONS = 6,
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
		case 6:
			m = "OPTIONS";
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
	} else if(method == "OPTIONS") {
		return Method::OPTIONS;
	} else {
		return Method::HEAD;
	}
}


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
	string extension = filename.substr(filename.find_last_of('.')+1);
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
	mimetype_map["js"] = "text/javascript";
	mimetype_map["map"] = "text/javascript";
	mimetype_map["jpeg"] = "image/jpeg";
	mimetype_map["png"] = "image/png";
}


map<string, string> parseParams(string fullPath) {
	map<string, string> result;
	::std::regex paramsR = ::std::regex(R"((([\w\d\=\(\)\+\~\-\.\!\*\'\%\`\ ])+))");
	string path_params = fullPath.substr(fullPath.find("?")+1);
	auto params_begin = std::sregex_iterator(path_params.begin(), path_params.end(), paramsR);
	auto params_end = std::sregex_iterator();
	for(std::sregex_iterator i = params_begin; i != params_end; i++) {
		string parameter = (*i).str();
		string key, val;
		int keyEndsAt;
		if(((keyEndsAt = parameter.find('=')) != -1)) {
			key = parameter.substr(0, keyEndsAt);
			val = parameter.substr(keyEndsAt+1);	
		} else {
			key = parameter;
			val = "true";
		};
		result[key] = val;
	}
	return result;
}

map<string, string> handleParams(string fullPath) {
	map<string, string> result;
	if(fullPath.find("?") != -1) {
		result = parseParams(fullPath);
	}
	return result;
}

// A mess...
int parseRequest(string request, requestModel &result) {
	map<string, string> headers;
	::std::regex firstLineR = ::std::regex(R"(^(.*)\S)");
	::std::regex headersR = ::std::regex(R"([A-Z].*: (.*)\S)");
	::std::regex bodyR = ::std::regex(R"(.*$)");
	
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
	
	map<string, string> params = handleParams(fullPath);
	
	string proto = firstLine.substr(methodEndsAt + 1 + pathEndsAt + 1); 
	// cumulated offset
	
	auto headers_begin = std::sregex_iterator(request.begin(), request.end(), headersR);
	auto headers_end = std::sregex_iterator();
	string lastHeader;
	for(std::sregex_iterator i = headers_begin; i != headers_end; i++) {
		lastHeader = (*i).str();
		int colonIndex = lastHeader.find(":");
		headers[lastHeader.substr(0, colonIndex)] = lastHeader.substr(colonIndex+1);
	}
	string body;
	std::smatch bodyM;
	regex_search(request, bodyM, bodyR);
	string b = bodyM.str();
	if(!(b == lastHeader)) { //I'm that lazy
		body = b;
	}
	result.method = parseMethod(method);
	result.path = path;
	result.path_params = params;
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
	
	//Tłumaczenie tla peasantów od pythona i typescripta jak ja
	//Node == IP
	//Null znaczy że tylko localhost może się połączyć. NAWET 127.0.0.1 i [::1] nie zadziałają
	//Service == PORT
	//hints == OPTIONS
	//servinfo == 
	int addrinfoStatus;
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

/*
template <typename T>
struct Node {
	T val;
	Node* next;
};

template <typename T>
struct Queue {
	T val;
	Queue* head;
};
*/

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
		//if(!fork()) {
		std::thread worker( [&]() {
			std::string incomingData;
			char buff[8096];
			int bytesRead;
			if(foreignSocketFileDescriptor == -1) {
				perror("accept");
				return;
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
		});
		worker.join();
		close(foreignSocketFileDescriptor); // done using it
	}
	close(serverSocketFileDescriptor); // no need to listen anymore
	return 0;
};

template <typename Context>
responseModel Server<Context>::preflightResponse(requestModel req) {
	responseModel res;
	string methods;
	res.proto = "HTTP/1.1";
	res.code = 204;
	res.status = "No Content";
	res.headers["Access-Control-Allow-Origin"] = '*';
	for(auto m: preflight[req.path]) {
		methods += parseMethod(m);
		if(m != preflight[req.path].back()) {
			methods += ", ";
		}
	}
	res.headers["Access-Control-Allow-Methods"] = methods;
	return res;
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
void Server<Context>::serveDirectory(string path) {
	DIR *directory;
	struct dirent *entry;
	struct stat statBuffer;
	int initial = path.length();
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
					serveDirectory(path+"/"+entry->d_name, initial);
				}
			}
			closedir(directory);
		} else if(statBuffer.st_mode & S_IFREG) {
			// Given path leads to a file
			if(getFilenameFromPath(path)[0] != IGNORE_CHAR) {
				createGetFileEndpoint(path, initial);
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
void Server<Context>::serveDirectory(string path, int skip) {
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
					serveDirectory(path+"/"+entry->d_name, skip);
				}
			}
			closedir(directory);
		} else if(statBuffer.st_mode & S_IFREG) {
			// Given path leads to a file
			if(getFilenameFromPath(path)[0] != IGNORE_CHAR) {
				createGetFileEndpoint(path, skip);
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
	createGetFileEndpoint(path, 0);
}

template <typename Context>
void Server<Context>::createGetFileEndpoint(string path, int skip) {
	string networkPath = path.substr(skip);
	translatePath(path);
	translatePath(networkPath);
	pathDictionary[networkPath] = path;
	createEndpoint(Method::GET, path, GETFileHandler);
}

template <typename Context>
responseModel Server<Context>::GETFileHandler(requestModel request, Context context) {
	responseModel response;
	string fileContent;
	translatePath(request.path);
	// Check if key exists.
	//if(pathDictionary.find(request.path) != pathDictionary.end() ) {
		request.path = pathDictionary[request.path];
	//}
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
responseModel HandlerClass<Context>::get(requestModel r, Context c) {
	return NOT_IMPLEMENTED;
};
template <typename Context>
responseModel HandlerClass<Context>::post(requestModel r, Context c) {
	return NOT_IMPLEMENTED;
};
template <typename Context>
responseModel HandlerClass<Context>::put(requestModel r, Context c) {
	return NOT_IMPLEMENTED;
};
template <typename Context>
responseModel HandlerClass<Context>::del(requestModel r, Context c) {
	return NOT_IMPLEMENTED;
};


template <typename Context>
void Server<Context>::handleIncomingData(std::string incomingData) {
	string resp;
	struct requestModel request;
	// added after swithcing to thread from fork.
	try {
		if(parseRequest(incomingData, request) == -1) {
			resp = HTTP_BADREQUEST;
			if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
				perror("send");
				std::cout << "bad request send error" << std::endl;
				return;
			};
		};
		std::cout << "translating from " << request.path << " to " << pathDictionary[request.path] << std::endl;
		// Sometimes requests come too often for threads. This happens when I kepp
		// F5 pressed for like 6 seconds. Socket read returns empty string. In that
		// case fork() works just fine, but thread breaks quits without exception...
		// checking if there actually is data before handling request mitigates this issue
		if(request.path == "") {
			return;
		}
		if(request.method == Method::OPTIONS) {
			resp = preflightResponse(request).parse();
		} else if(endpoints[request.method][pathDictionary[request.path]] != 0) {
			resp = endpoints[request.method][pathDictionary[request.path]](request, context).parse();
		} else {
			resp = HTTP_NOTFOUND;
		}
		if(send(foreignSocketFileDescriptor, resp.c_str(), resp.size(), 0) == -1) {
			perror("send");
		};
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}


template <typename Context>
handlerFunction<Context> Server<Context>::onionize(
		handlerFunction<Context> fn
	) {
	for(MiddlewareFunctor<Context> functor: middleware) {
		
		handlerFunction<Context> new_fn = 
			[=](requestModel r, Context c) {
				return functor(fn, r, c);
			};
		fn = new_fn;

	}
	return fn;
}

//  misc  //
//  MiddlewareFunctor  //

template <typename Context>
requestModel MiddlewareFunctor<Context>::wrapRequest(requestModel req) {
	return req;
};

template <typename Context>
responseModel MiddlewareFunctor<Context>::wrapResponse(responseModel res) {
	return res;
};

template <typename Context>
responseModel MiddlewareFunctor<Context>::operator() (
		handlerFunction<Context> fn, 
		requestModel &r, 
		Context &c
	) 
{
	std::cout << "Applying middleware" << std::endl;
	r = wrapRequest(r);
	responseModel response = fn(r, c);
	response = wrapResponse(response);
	return response;
};

//  MiddlewareFunctor  //
//  on  //

template <typename Context>
void Server<Context>::on(
		Method method, 
		string path, 
		handlerFunction<Context> handler
	)
{
	handlerFunction<Context> fn = [=](requestModel r, Context c) {
		responseModel response = handler(r, c);
		return response;
	};
	translatePath(path);
	pathDictionary[path] = path;
	createEndpoint(method, path, fn);
};


template <typename Context>
void Server<Context>::on(
		Method method, 
		string path, 
		string_handlerFunction<Context> handler
	)
{
	handlerFunction<Context> fn = [&](requestModel r, Context c) {
		responseModel response;
		response.proto = "HTTP/1.1";
		response.code = 200;
		response.status = "OK";
		response.body = handler(r, c);
		return response;
	};
	translatePath(path);
	pathDictionary[path] = path;
	createEndpoint(method, path, fn);
};

//  gosh
template<typename Context>
template<typename Type>
void Server<Context>::on(pathString path, Type h) {
	if(std::is_base_of<HandlerClass<Context>, Type>()) {
		using namespace std::placeholders;

		handlerFunction<Context> getHandler = 
			[&](requestModel r, Context c){
					return h.get(r, c);
				};
		on(Method::GET, path, getHandler);
		
		handlerFunction<Context> postHandler = 
			std::bind(&Type::post, &h, _1, _2);
		on(Method::POST, path, postHandler);
		
		handlerFunction<Context> putHandler = 
			std::bind(&Type::put, &h, _1, _2);
		on(Method::PUT, path, putHandler);
		
		handlerFunction<Context> delHandler = 
			std::bind(&Type::del, &h, _1, _2);
		on(Method::DELETE, path, delHandler);
	} else {
		std::cout << "Given type does not extend ClassHandler<>!" << std::endl;
	}
};

//  on  //
//  createEndpoint  //

template <typename Context>
class CORSMiddleware: public MiddlewareFunctor<Context> {
	public: 
		std::vector<string> origins;
		responseModel wrapResponse(responseModel response) override {

			std::cout << "Applying CORS middleware" << std::endl;
			response.headers["Access-Control-Allow-Origin"] = '*';
			return response;
		}
};

template <typename Context>
handlerFunction<Context> test(handlerFunction<Context> &fn) {
	auto cr = CORSMiddleware<Context>();
	return [&](requestModel r, Context c) mutable {
		return fn(r, c);
	};
}




template <typename Context>
void Server<Context>::createEndpoint(
		Method method, 
		string path, 
		handlerFunction<Context> handler
	)
{
	string m = parseMethod(method);
	std::cout << "initializing " << m << " endpoint for " << path << std::endl;
	const handlerFunction<Context> fn = [=](requestModel r, Context c) {
		// [0]->(...) doesnt work ;.;
		return middleware[0]->operator()(handler, r, c);
	};
	endpoints[method][path] = fn;
 	preflight[path].push_back(method);
}

//  createEndpoint  //


