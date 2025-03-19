#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <stdbool.h>
#include <unordered_map>
#include "../Networks/Socket.hpp"

struct Location {
	std::string path;
	std::string root;
	std::string index;
	bool directory_listing;
	std::vector<std::string> methods; //GET POST
	std::pair<int, std::string> HTTP_redirection; //301 (code: int) http://new_webstie (redirection: string)
};

class Server {
private:
	Socket server_socket;
	int port = 8080;
	u_long host = INADDR_ANY;
	int client_max_body = 10485760;
	std::vector<std::string> server_names {};
	std::unordered_map<std::string, Location> locations {};

public:
	Server();
	~Server() = default;

	void setPort(int port);
	void setHost(u_long host);
	void setClientMaxBody(int client_max_body);
	void setServerNames(std::vector<std::string>& server_names);
	void setLocations();

	int getPort() const;
	u_long getHost() const;
	int getClientMaxBody() const;
	std::vector<std::string> getServerNames() const;
	std::unordered_map<std::string, Location> getLocations() const;

};

#endif
