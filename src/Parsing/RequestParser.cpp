/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 15:06:13 by maraasve          #+#    #+#             */
/*   Updated: 2025/05/21 19:14:31 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestParser.hpp"

RequestParser::RequestParser()
{
}

RequestParser::~RequestParser()
{
}

bool RequestParser::parseHeader(std::string &requestStr)
{
	size_t header_end = requestStr.find("\r\n\r\n");
	if (header_end == std::string::npos)
	{
		return false;
	}
	std::string header = requestStr.substr(0, header_end);
	std::istringstream stream(header);
	parseRequestLine(stream);
	parseHeaders(stream);
	checkBasicHeaders();
	if (_request.getMethod() == "POST")
	{
		_bytes_read = requestStr.size() - (header_end + 4);
		_state = requestState::PARSING_BODY;
		return true;
	}
	_state = requestState::COMPLETE;
	return true;
}

bool RequestParser::parseBody(std::string &requestStr, ssize_t bytes)
{
	_bytes_read += bytes;
	if (_bytes_read == _request.getContentLength())
	{
		size_t pos = requestStr.find("\r\n\r\n");
		if (pos == std::string::npos)
		{
			_request.setBody("");
		}
		else
		{
			_request.setBody(requestStr.substr(pos + 4, _bytes_read));
		}
		_state = requestState::COMPLETE;
		return true;
	}
	if (_bytes_read > _request.getContentLength())
	{
		_request.setErrorCode("400");
		_state = requestState::ERROR;
		return true;
	}
	return false;
}

void RequestParser::parseRequestLine(std::istringstream &stream)
{
	std::string method, uri, httpVersion;
	stream >> method >> uri >> httpVersion;
	_request.setRequestLine(method, uri, httpVersion);
	if (!checkMethod())
	{
		_request.setErrorCode("501");
		return;
	}
	if (!checkHTTP())
	{
		_request.setErrorCode("505");
		return;
	}
	splitUri(uri);
	if (!checkPath() || !checkQuery())
	{
		_request.setErrorCode("400");
		return;
	}
}

void RequestParser::parseHeaders(std::istringstream &stream)
{
	std::unordered_map<std::string, std::string> headers;
	std::string line, key, value;

	stream >> std::ws;
	while (std::getline(stream, line))
	{
		size_t pos = line.find(':');
		if (pos == std::string::npos)
		{
			_request.setErrorCode("400");
			return;
		}
		key = trim(line.substr(0, pos));
		value = trim(line.substr(pos + 1));
		if (key.empty() || value.empty())
		{
			_request.setErrorCode("400");
			return;
		}
		_request.addHeader(key, value);
	}
}

void RequestParser::splitUri(std::string uri)
{
	size_t pos = uri.find("?");
	if (pos != std::string::npos)
	{
		_request.setQueryString(uri.substr(pos + 1));
		_request.setPath(uri.substr(0, pos));
	}
	else
	{
		_request.setPath(uri);
	}
}

std::string RequestParser::trim(std::string str)
{
	size_t first, last;
	first = str.find_first_not_of(" \n\t\r\f\v");
	last = str.find_last_not_of(" \n\t\r\f\v");
	if (first == std::string::npos || last == std::string::npos)
	{
		return ("");
	}
	return (str.substr(first, (last - first + 1)));
}

void RequestParser::checkBasicHeaders()
{
	const std::unordered_map<std::string, std::string> &headers = _request.getHeaders();

	if (!checkHost(headers))
	{
		return;
	}
	if (!checkContentLength(headers))
	{
		return;
	}
	return;
}

bool RequestParser::checkHost(const std::unordered_map<std::string, std::string> &headers)
{
	auto it = headers.find("Host");
	if (it == headers.end())
	{
		_request.setErrorCode("400");
		return false;
	}
	else
	{
		std::string host = it->second;
		size_t pos = host.find(":");
		if (pos != std::string::npos)
		{
			_request.setHost(host.substr(0, pos));
			_request.setPort(host.substr(pos + 1));
		}
		else
		{
			_request.setHost(host);
		}
	}
	return true;
}

bool RequestParser::checkContentLength(const std::unordered_map<std::string, std::string> &headers)
{
	const std::string &method = _request.getMethod();
	int contentLength;
	auto it_content_length = headers.find("Content-Length");
	if (it_content_length == headers.end())
	{
		if (method == "POST")
		{
			_request.setErrorCode("411");
		}
		return false;
	}
	if (method == "GET")
	{
		_request.setErrorCode("400");
		return false;
	}
	if (method == "POST")
	{
		try
		{
			std::cout << "converting content length: " << it_content_length->second << std::endl;
			std::cout << std::stol(it_content_length->second) << std::endl;
			contentLength = static_cast<ssize_t>(std::stol(it_content_length->second));
		}
		catch (...)
		{
			std::cout << "\t\tError while converting content length\n\n"
						<< std::endl;
			_request.setErrorCode("500");
			return false;
		}
		_request.setContentLength(contentLength);
	}

	return true;
}

bool RequestParser::checkMethod() const
{
	std::string method = _request.getMethod();
	return (method == "GET" || method == "POST" || method == "DELETE");
}

bool RequestParser::checkPath() const
{
	std::string path, uri;
	std::regex path_regex(R"(^\/([a-zA-Z0-9\-_~.]+(?:\/[a-zA-Z0-9\-_~.]+)*$))");
	std::smatch match;
	path = _request.getPath();
	uri = _request.getURI();
	if (std::regex_match(path, match, path_regex) && uri.find('\n') == std::string::npos && uri.find('\r') == std::string::npos)
	{
		return true;
	}
	return true;
}

bool RequestParser::checkQuery() const
{
	std::string query = _request.getQueryString();
	std::regex query_regex(R"(^([a-zA-Z0-9\-_.~]+)=([a-zA-Z0-9\-_.~%]+))");
	std::smatch match;
	if (query.empty() || std::regex_match(query, match, query_regex))
	{
		return true;
	}
	return false;
}

bool RequestParser::checkHTTP() const
{
	return (_request.getHTTPVersion() == "HTTP/1.1");
}

void RequestParser::checkServerDependentHeaders(const Server &server, Location &location)
{
	if (!location.getRedirection().first.empty())
	{
		_request.setErrorCode(location.getRedirection().first);
		_request.setRedirectionURI(location.getRedirection().second);
		return;
	}
	if (!checkMatchURI(server, location) || (location.getRedirection().first.empty() && !checkFile(server, location)))
	{
		std::cout << "\n\n\tFail: Check Match uri or CheckFile (404)" << std::endl;
		_request.setErrorCode("404");
		return;
	}
	if (!checkAllowedMethods(location))
	{
		std::cout << "\n\n\tFail: Check Allowed Methods (405)" << std::endl;
		_request.setErrorCode("405");
		return;
	}
	if (_request.getMethod() == "POST")
	{
		if (!checkBodyLength(server, location))
		{
			std::cout << "\n\n\tFail: Check Body Length (413)" << std::endl;
			_request.setErrorCode("413");
			return;
		}
	}
}

bool RequestParser::checkMatchURI(const Server &server, Location &location)
{
	const std::string &uri = _request.getURI();
	const std::string &loc_path = location.getPath();
	const std::string &base_root = location.getRoot().empty() ? server.getRoot() : location.getRoot();
	_request.setBaseRoot("." + base_root);
	if (uri.compare(0, loc_path.size(), loc_path) != 0)
	{
		_request.setRootedUri(_request.getBaseRoot());
		return false;
	}
	std::string rest_uri = uri.substr(loc_path.size());
	if (rest_uri[0] != '/')
	{
		rest_uri = "/" + rest_uri;
	}
	if (rest_uri.empty())
	{
		rest_uri = "/";
	}
	_request.setRootedUri(_request.getBaseRoot() + rest_uri);
	return true;
}

bool RequestParser::checkAllowedMethods(Location &location)
{
	if (location.getAllowedMethods().empty())
	{
		return true;
	}
	for (const std::string &allowed_method : location.getAllowedMethods())
	{
		if (allowed_method == _request.getMethod())
		{
			return true;
		}
	}
	return false;
}

bool RequestParser::checkReadingAccess()
{
	if (access(_request.getRootedURI().c_str(), R_OK) != 0)
	{
		return false;
	}
	return true;
}

bool RequestParser::checkRequestURI()
{
	struct stat sb;
	if (stat(_request.getRootedURI().c_str(), &sb) == -1)
	{
		std::cout << "CheckRequestURI: stat problem\n\n\n\n"
				  << std::endl;
		return false;
	}
	if (!checkReadingAccess())
	{
		std::cout << "CheckRequestURI:access problem\n\n\n\n"
				  << std::endl;
		return false;
	}
	if (S_ISDIR(sb.st_mode))
	{
		_request.setFileType(DIRECTORY);
		return true;
	}
	else if (S_ISREG(sb.st_mode))
	{
		_request.setFileType(REGULAR_FILE);
		return true;
	}
	std::cout << "CheckRequestURI: no regular file nor directory" << std::endl;
	return false;
}

bool RequestParser::hasCgiPrefix(const std::string &uri) const
{
	static const std::string cgiPrefix = "/cgi";
	return uri.find(cgiPrefix) != std::string::npos;
}

bool RequestParser::extractQueryString(std::string &uri)
{
	size_t pos = uri.find('?');
	if (pos == std::string::npos)
		return false;
	_request.setQueryString(uri.substr(pos + 1));
	uri.erase(pos);
	return true;
}

bool RequestParser::checkCgiScript()
{
	std::string &uri = _request.getRootedURI();
	if (!hasCgiPrefix(uri))
	{
		std::cerr << "CGI: Your root folder should contain a CGI folder" << std::endl;
		return false;
	}
	if (extractQueryString(uri))
	{
		return false;
	}
	_request.setRootedUri(uri);
	return checkRequestURI();
}

bool RequestParser::checkFile(const Server &server, Location &location)
{
	if (checkCgiScript())
	{
		return true;
	}
	if (!checkRequestURI())
	{

		return false;
	}
	if (_request.getFileType() == DIRECTORY)
	{
		if (!location.getIndex().empty())
		{
			_request.setRootedUri(_request.getRootedURI() + location.getIndex());
		}
		else if (location.getAutoIndex())
		{
			_request.setFileType(AUTOINDEX);
		}
		else if (!server.getIndex().empty())
		{
			_request.setRootedUri(_request.getRootedURI() + server.getIndex());
		}
		else if (server.getAutoIndex())
		{
			_request.setFileType(AUTOINDEX);
		}
		else
		{
			return false;
		}
		if (_request.getFileType() != AUTOINDEX && !checkReadingAccess())
		{
			return false;
		}
	}
	return true;
}

bool RequestParser::checkBodyLength(const Server &server, Location &location)
{
	ssize_t contentLength = _request.getContentLength();
	std::cout << "Content-Length (CheckBOdyLength): " << contentLength << std::endl;
	std::cout << "CLient Max Body: " << location.getClientMaxBody() << std::endl;
	if (location.getClientMaxBody() && contentLength > static_cast<ssize_t>(location.getClientMaxBody()))
	{
		return false;
	}
	else if (location.getClientMaxBody())
	{
		return true;
	}
	else if (contentLength > static_cast<ssize_t>(server.getClientMaxBody()))
	{
		return false;
	}
	return true;
}

std::string RequestParser::getErrorCode()
{
	return _request.getErrorCode();
}

requestState RequestParser::getState()
{
	return _state;
}

Request RequestParser::getRequest() &&
{
	return std::move(_request);
}

const Request &RequestParser::getRequest() const &
{
	return _request;
}
