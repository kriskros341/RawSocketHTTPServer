#pragma once
#include <dirent.h>

#include <sys/stat.h>


#include <functional>
#include <string.h>
#include <map>
#include <vector>

#include <fstream>
#include <sstream>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BACKLOG 10 // how many pending connections queue will hold
#define DEFAULT_PORT 80 // the port users will be connecting to
#define IGNORE_CHAR '.'
#define HTTP_BADREQUEST "HTTP/1.1 400 Bad Request\r\n\r\n <div>400 Bad Request</div>"
#define HTTP_NOTFOUND "HTTP/1.1 404 Not Found\r\n\r\n <div>404 Not Found</div>"


using std::string;
using std::map;
int yes = 1; //It's needed. Trust me.

enum class Method;
struct requestModel;
struct responseModel;
map<string, string> mimetype_map;

/* 
 * Creates appropriate sockaddr info struct for ipv4 and ipv6
 */
void *get_in_addr(struct sockaddr *sa);
/*
 * Returns string version of given method used in responses
 */
string parseMethod(Method method); 

/*
 * Returns enum version of given method used in functions
 */
Method parseMethod(string method);

/*
 * Returns enum version of given method used in functions
 */
void initializeMimeMap();

/*
 * A function that returns string with appropriate mime type for given file
 */ 
string getMimeType(string filename);

/*
 * A function that returns whether a specified file exists or not
 */ 
bool exists (string filename);

/*
 * A function that unifies paths between requests and filesystem
 */ 
void translatePath(string &path);

/*
 * A function that parses request string into a request struct
 * returns 0 on success and -1 on failure
 */ 
int parseRequest(string request, requestModel &result);

/*
 * A function that loads file content into a string
 * returns 0 on success and -1 on failure
 */ 
int loadFile(string path, string &content);

/*
 * A function that maps files in specified directory
 */ 
bool isThereSuchFile(string &path, string &content);


class SocketServer {
	//  Pure socket setup. 0% Implementation.
	private:
		//  structures for holding info about server for binding
		addrinfo hints, *servinfo, *p;
		//  structure for holding info about remote client
		sockaddr_storage their_addr;
		char s[INET6_ADDRSTRLEN];

		void bindToLocalAddress();
	
	protected: 
		// file descriptor for unix style socket.
		int serverSocketFileDescriptor, foreignSocketFileDescriptor;
		socklen_t sin_size;
	
	public:
		//  Service on which server will listen
		int port = DEFAULT_PORT;
		//  Override it for actual implementation
		virtual void handleIncomingData(std::string incomingData) {};
		//  even more setup
		void setup();
		//  data sending, receiving and subprocess creation
		int mainLoop();
		SocketServer() {};
};



template <typename Context>
class HandlerClass {
	public:
		virtual responseModel get(requestModel r, Context c);
		virtual responseModel post(requestModel r, Context c);
		virtual responseModel put(requestModel r, Context c);
		virtual responseModel del(requestModel r, Context c);
};


//  a type that every endpoint function must imitate
template <typename Context>
using handlerFunction = 
	std::function<
		responseModel(requestModel, Context)
	>;


//  a type that every endpoint object must imitate
template <typename Context>
using string_handlerFunction = std::function<string(requestModel, Context)>;


typedef string pathString;

//  used for storing data about paths.
//  Global, because developer might need
//  it when defining custom path translations
std::map<pathString, pathString> pathDictionary;


/*
 * A structure that decorates endpoint function
 * */
template <typename Context>
class MiddlewareFunctor {
	public:
		virtual requestModel wrapRequest(requestModel req);
		virtual responseModel wrapResponse(responseModel res);
		responseModel operator() (
				handlerFunction<Context> fn, 
				requestModel &r, 
				Context &c
			);
};


template <typename Context>
class Server: public SocketServer {
	private:
		//  A map of Handler functions and associated paths/methods
		std::map<
			Method, 
			std::map<
				pathString, 
				handlerFunction<Context> 
			>
		> endpoints;
		
		//dealing with CORS
		std::map<pathString, std::vector<Method>> preflight;
		responseModel preflightResponse(requestModel r);
		
		//  Internal use only. Does not add record to pathDictionary
		//  Other methods are call this one.
		void createEndpoint(Method method, pathString path, handlerFunction<Context>);

		//  version where we only define body of response
		void createEndpoint(Method method, pathString path, std::function<string(requestModel, Context)>);
		
		//  Used internally in functions with the same name.
		void serveDirectory(pathString pathToDirectory, int skip);
		void createGetFileEndpoint(pathString path, int skip);

		//  A handler function for dealing with singular files.
		static responseModel GETFileHandler(requestModel request, Context context);
		
		//  Processsing requests
		void handleIncomingData(string incomingData) override;
	public:
		//  Passed to every endpoint function.
		Context context;
		//  Used for defining file endpoints.
		void createGetFileEndpoint(pathString path);
		//  Function that maps directory into endpoints.
		void serveDirectory(pathString pathToDirectory);
		//  Function that creates an endpoint with custom handler.
		void on(Method method, pathString path, handlerFunction<Context>);
		//  For when you only need to define body
		void on(Method method, pathString path, string_handlerFunction<Context>);
		
		//wrap handlerFunction in middleware
		handlerFunction<Context> onionize(handlerFunction<Context> fn);
		
		// Two ways of achieving the same thing for
		// demonstration purpose
		template <typename Type>
		void on(pathString, Type);
		//  You can't store raw pointers in vectors
		std::vector<
				MiddlewareFunctor<Context>*
			> middleware;
		
		Server<Context>(Context &External) {
			setup();
			context = External;
		}
};

