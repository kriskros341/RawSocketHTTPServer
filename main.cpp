#include "include/Server.cpp"



using std::string;
using std::map;
using std::regex;


responseModel endpointFunction(requestModel request) {
	responseModel response;
	response.proto = "HTTP/1.1";
	response.code = 301;
	response.status = "Moved Permanently";
	response.headers["Location"] = "https://www.youtube.com";
	response.body = "<div>jdjdjdj</div>";
	//::cout << "testing the endpoint" << std::endl;
	return response;
};


/*
MongoDB 


No user auth because it's just pointless without ssl
*/

int main(int argc, char *argv[]) {
	initializeMimeMap();
	//cout << __cplusplus << endl; //14
	Server s;
	s.port = 999;
	s.getEndpointsFromDirectory(".");
	s.on(Method::POST, "/endpoint", endpointFunction);
	//s.on(METHOD, "PATH", functionHandler)
	s.mainLoop();
	return 0;
};

