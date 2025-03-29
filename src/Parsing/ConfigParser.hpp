#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <regex>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <sstream>
#include "../Server/Server.hpp"
#include "./ConfigTokenizer.hpp"

struct ServerValidationState {
	bool has_root = false;
	bool has_index = false;
	bool has_listen = false;
};

class ConfigParser
{
private:
	int open_braces;
	std::vector<Server> servers;
	ConfigTokenizer Tokenizer;
	ServerValidationState required_directives;

public:
	ConfigParser(const std::string &filename);

	std::string loadFileAsString(std::ifstream &file);
	void parseServer(std::vector<Token> tokens);
	void parseLocation(std::vector<Token>::iterator& it, std::vector<Token>::iterator end);

	bool isValidPath(std::string& path);
	bool isValidIndex(std::string& index);
	bool isValidClientBodySize(std::string& client_body_size);
	bool isValidErrorCode(std::string& error_code);

	// u_long convertHost(const std::string &host);

	const std::vector<Server> &getServers() const;
	void error_check(const std::string &msg) const;
};

#endif