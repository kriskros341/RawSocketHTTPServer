#include "include/Server.cpp"
#include "pqxx/pqxx"
#include <functional>
#include "include/json.hpp"
#include <iostream>


using std::string;
using std::map;
using std::regex;
using nlohmann::json;


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


responseModel getTasks(requestModel request, database db) {
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
	response.headers["Access-Control-Allow-Origin"] = '*';
	response.body = jsonBody.dump();
	//::cout << "testing the endpoint" << std::endl;
	return response;
};

bool isAlright(string s) {
	if(s.length() == 0) {
		return false;
	}
	for(char x: s) {
		if(not isdigit(x)) {
			return false;
		}
	}
	return true;
}

responseModel deleteTask(requestModel request, database db) {
	responseModel response;
	nlohmann::json jsonBody;
	string id = request.path_params.at("id");
	if(isAlright(id)) {
		pqxx::result res = db.query("DELETE FROM todo WHERE id="+id);
	}
	response.proto = "HTTP/1.1";
	response.code = 200;
	response.status = "OK";
	response.headers["Access-Control-Allow-Origin"] = '*';
	return response;
}

responseModel createTask(requestModel request, database db) {
	map<string, string> body = parseParams(request.body);
	responseModel response;
	nlohmann::json jsonBody;
	response.proto = "HTTP/1.1";
	if(isAlright(body["id"]) && body["name"].length() != 0 && body["isdone"] == "false" || body["isdone"] == "true") {
		pqxx::result res = db.query("INSERT INTO todo VALUES("+body["id"]+", \'"+body["name"]+"\', "+body["isdone"]+")");
		response.code = 200;
		response.status = "OK";
		response.headers["Access-Control-Allow-Origin"] = '*';
		return response;
	}
	response.code = 422;
	response.status = "Unprocessable Entity";
	response.headers["Access-Control-Allow-Origin"] = '*';
	//for(std::pair<string, string> x: body) {
	//	std::cout << x.first << "     " << x.second << std::endl;
	//}
	//::cout << "testing the endpoint" << std::endl;
	return response;
}

responseModel updateTask(requestModel request, database db) {
	map<string, string> body = parseParams(request.body);
	responseModel response;
	nlohmann::json jsonBody;
	response.proto = "HTTP/1.1";
	string id = request.path_params["id"];
	std::cout << parseMethod(request.method) << std::endl;
	if(isAlright(id)) {
		string setString;
		int iter{};
		for(std::pair<string, string> x: body) {
			if(iter != 0) {
				setString += ", ";
			}
			setString += x.first + " = '" + x.second + "'";
			iter++;
		}
		string q = "UPDATE todo SET " + setString + " WHERE id = " + request.path_params["id"];
		std::cout << q << std::endl;
		pqxx::result res = db.query("UPDATE todo SET " + setString + " WHERE id = " + request.path_params["id"]+";");
		response.code = 200;
		response.status = "OK";
		response.headers["Access-Control-Allow-Origin"] = '*';
		return response;
	}
	response.code = 422;
	response.status = "Unprocessable Entity";
	response.headers["Access-Control-Allow-Origin"] = '*';
	//for(std::pair<string, string> x: body) {
	//	std::cout << x.first << "     " << x.second << std::endl;
	//}
	//::cout << "testing the endpoint" << std::endl;
	return response;
}


int main(int argc, char *argv[]) {
	initializeMimeMap();
	//cout << __cplusplus << endl; //14
	database *db = new database();
	ServerWithDB s(*db);
	s.serveDirectory("build");
	s.on(Method::GET, "/gettasks", getTasks);
	s.on(Method::DELETE, "/deltask", deleteTask);
	s.on(Method::POST, "/createtask", createTask);
	s.on(Method::PUT, "/updatetask", updateTask);
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
