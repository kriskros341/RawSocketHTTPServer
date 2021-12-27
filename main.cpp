#include "include/Server.cpp"
#include "pqxx/pqxx"
#include <functional>
#include "include/json.hpp"


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
			setConnection();
			pqxx::work trans(*conn);
			pqxx::result res = trans.exec(query);
			trans.commit();
			conn->disconnect();
			return res;
		}
		database() {
			setConnection();
		}
};


class ServerWithDB: public Server<database> {
	public:
		ServerWithDB(database &db) : 
		Server<database>(db) {
			std::cout << "database " << context.conn->dbname() << " connected" << std::endl;
		};
};


responseModel endpointFunction(requestModel request, database db) {
	responseModel response;
	nlohmann::json jsonBody;
	pqxx::result res = db.query("SELECT * FROM todo");
	if(res.empty()) {
		return {"HTTP/1.1", 404, "Not Found", {}, "<div>Not Found</div>"};
	}
	int i{};
	for(pqxx::row x: res) {
		jsonBody[i]["id"] = x["id"].as<int>();
		jsonBody[i]["name"] = x["name"].as<string>();
		jsonBody[i]["isdone"] = x["isdone"].as<bool>();
		i++;
	};
	response.proto = "HTTP/1.1";
	response.code = 200;
	response.status = "OK";
	response.body = jsonBody.dump();
	//::cout << "testing the endpoint" << std::endl;
	return response;
};


class Tasks {
	public:
		static responseModel get(requestModel, database d) {
			responseModel g;
			g.proto = "HTTP/1.1";
			g.code = 200;
			g.status = "OK";
			return g;
	}
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
	Tasks t;
	s.on(Method::GET, "/dbtest", endpointFunction);
	s.on(Method::GET, "/dbtest2", t.get);
	//s.on("/dbtest2", t);

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
