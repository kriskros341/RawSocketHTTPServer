#include "include/Server.cpp"
#include "pqxx/pqxx"
#include <functional>




using std::string;
using std::map;
using std::regex;


class database {
	public:
		pqxx::connection* conn;
		void setConnection() {
			conn = new pqxx::connection(
					"user=postgres "
					"host=192.168.1.21 "
					"password=p1ssw2rd "
					"port=5432 "
					"dbname=tasks"
					);
		};
		void disconnect() {
			conn->disconnect();
		};
		pqxx::result query(string query) {
			pqxx::work trans(*conn, "trans");
			pqxx::result res = trans.exec(query);
			trans.commit();
			return res;
		}
};


class ServerWithDB: public Server<database> {
	public:
		ServerWithDB(database &db) : 
		Server<database>(db) {
		
		};
};


responseModel endpointWithDB(database d, requestModel r) {
	responseModel response;
	response.proto = "HTTP/1.1";
	response.code = 301;
	response.status = "Moved Permanently";
	response.headers["Location"] = "https://www.youtube.com";
	response.body = "<div>jdjdjdj</div>";
	//::cout << "testing the endpoint" << std::endl;
	return response;
};


responseModel endpointFunction(requestModel request, database context) {
	context.query("SELECT * FROM tasks");
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
	database *db = new database();
	ServerWithDB s(*db);
	
	s.getEndpointsFromDirectory(".");
	s.on(Method::GET, "/dbtest", endpointFunction);
	//s.on(METHOD, "PATH", functionHandler)
	s.mainLoop();
	return 0;
};

/*
 * pqxx::result g = s.context.query("SELECT * FROM todo");
for (auto const &row: g) {
	for (auto const &field: row) {
		std::cout << field << std::endl;
	}
}
*/
