/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 15:06:22 by maraasve          #+#    #+#             */
/*   Updated: 2025/05/19 14:10:04 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./Client.hpp"

Client::Client(int fd, Epoll &epoll, int socket_fd) : _fd(fd), _serverPtr(nullptr), _epoll(epoll),
													 _socketFd(socket_fd), _startTime(std::chrono::steady_clock::now())
{
	std::cout << "Client socket(" << _fd << ") is created" << std::endl;
}

Client::~Client()
{
	std::cout << "Client is deleted" << std::endl;
}

void Client::handleIncoming()
{
	// auto now = std::chrono::steady_clock::now();
	// auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _startTime).count();
	// if (elapsed >= TIMEOUT)
	// {
	// 	closeClientConnection(_fd);
	// 	return;
	// }
	switch (_state)
	{
		case clientState::READING_HEADERS:
			handleHeaderState();
			break;
		case clientState::READING_BODY:
			handleBodyState();
			break;
		case clientState::PARSING_CHECKS:
			handleParsingCheckState();
			break;
		case clientState::CGI:
			handleCgiState();
			break;
		case clientState::ERROR:
			handleErrorState();
			break;
		case clientState::RESPONDING:
			handleResponseState();
			break;
	}
}

void Client::handleOutgoing()
{
	std::cout << "\n\t----Handle Outgoing-- " << std::endl;
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _startTime).count();
	if (elapsed >= TIMEOUT) 
	{
		closeClientConnection(_fd);
		return;
	}
	ssize_t bytes = send(_fd, _responseString.c_str(), _responseString.size(), 0);
	if (bytes > 0)
	{
		_responseString.erase(0, bytes);
	}
	else if ((_responseString.empty() && closeClientConnection) || bytes <= 0)
	{
		closeClientConnection(_fd);
	}
}

void Client::handleHeaderState()
{
	std::cout << "\n\t--Handling Header State--" << std::endl;
	ssize_t bytes = readIncomingData(_requestString, _fd);
	if (bytes == 0)
	{
		closeClientConnection(_fd);
	}
	if (bytes < 0)
	{
		_state = clientState::ERROR;
		handleIncoming();
		return;
	}
	if (_requestParser.parseHeader(_requestString))
	{
		if (_requestParser.getRequest().getErrorCode() != "200")
		{
			_request = std::move(_requestParser).getRequest();
			assignServer(*this);
			_state = clientState::RESPONDING;
			handleIncoming();
			return;
		}
		assignServer(*this);
		if (_requestParser.getState() == requestState::PARSING_BODY)
		{
			_state = clientState::READING_BODY;
			_requestParser.parseBody(_requestString, 0);
		}
		if (_requestParser.getState() == requestState::COMPLETE)
		{
			_state = clientState::PARSING_CHECKS;
			handleIncoming();
			return;
		}
	}
}

void Client::handleBodyState()
{
	std::cout << "\n\t--Handling Body State--" << std::endl;
	ssize_t bytes = readIncomingData(_requestString, _fd);
	if (bytes > 0)
	{
		if (_requestParser.parseBody(_requestString, bytes))
		{
			_state = clientState::PARSING_CHECKS;
			handleIncoming();
			return;
		}
		if (_requestParser.getRequest().getErrorCode() != "200")
		{
			_request = std::move(_requestParser).getRequest();
			_state = clientState::RESPONDING;
			handleIncoming();
			return;
		}
	}
	else
	{
		_state = clientState::ERROR;
		handleIncoming();
	}
}

void Client::handleParsingCheckState()
{
	std::cout << "\n\t--Handling Parsing Check State--" << std::endl;
	if (_requestParser.getRequest().getErrorCode() != "200")
	{
		_request = std::move(_requestParser).getRequest();
		_state = clientState::RESPONDING;
		handleIncoming();
		return;
	}
	if (!resolveLocation(_requestParser.getRequest().getURI()))
	{
		_request = std::move(_requestParser).getRequest();
		_state = clientState::RESPONDING;
		handleIncoming();
		return;
	}
	_requestParser.checkServerDependentHeaders(*_serverPtr, _location);
	_request = std::move(_requestParser).getRequest();
	if (_request.getErrorCode() != "200")
	{
		_state = clientState::RESPONDING;
		handleIncoming();
		return;
	}
	_state = shouldRunCgi() ? clientState::CGI : clientState::RESPONDING;
	handleIncoming();
}

bool Client::shouldRunCgi()
{
	static const std::set<std::string> cgiExtensions = {"py", "php"};
	const std::string &uri = _request.getRootedURI();
	size_t pos = uri.find_last_of('.');
	if (pos == std::string::npos || pos == uri.size() - 1)
	{
		return false;
	}
	_cgi_extension = uri.substr(pos + 1);
	return cgiExtensions.count(_cgi_extension) > 0;
}

bool Client::resolveLocation(std::string uri)
{
	if (_serverPtr->getLocations().empty())
	{
		std::cout << "No location on Config File, making one" << std::endl;
		_serverPtr->getLocations().emplace_back();
		_location = _serverPtr->getLocations().back();
		_location.setPath("/");
		_location.setRoot(_serverPtr->getRoot());
		_location.setIndex(_serverPtr->getIndex());
		return true;
	}
	size_t bestMatchLength = 0;
	for (auto &location : _serverPtr->getLocations())
	{
		size_t path_size = location.getPath().size();
		if (uri.compare(0, path_size, location.getPath()) == 0)
		{
			if (path_size < bestMatchLength)
			{
				continue;
			}
			bestMatchLength = path_size;
			_location = location;
		}
	}
	if (bestMatchLength == 0)
	{
		return false;
	}
	return true;
}

void Client::handleErrorState()
{
	std::cout << "\n\t--Handle Error State--" << std::endl;
	_request.setErrorCode("500");
	_state = clientState::RESPONDING;
	handleIncoming();
}

void Client::handleCgiState()
{
	if (!_Cgi)
	{
		_startTimeCgi = std::chrono::steady_clock::now();
		std::cout << "CGI: Will be initialised" << std::endl;
		_Cgi = std::make_shared<Cgi>(this);
		if (!_Cgi->init())
		{
			std::cout << "CGI: Init failed" << std::endl;
			return;
		}
		_Cgi->startCgi();
	}
	if (_Cgi->getState() == cgiState::COMPLETE)
	{
		_request.setBody(_Cgi->getBody());
		_request.setFileType(CGI_FILE);
		_state = clientState::RESPONDING;
		handleIncoming();
		return;
	}
	else if (_Cgi->getState() == cgiState::ERROR)
	{
		_state = clientState::ERROR;
		handleIncoming();
		return;
	}
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _startTimeCgi).count();
	if (elapsed >= TIMEOUT_CGI) 
	{
		kill(_Cgi->getPid(), SIGQUIT);
		_state = clientState::ERROR;
		handleIncoming();
		return;
	}
}

void printRequestObject(Request &request)
{
	std::cout << "---Printing Request From Client---" << std::endl;
	std::cout << "Method: " << request.getMethod() << std::endl;
	std::cout << "Uri: " << request.getURI() << std::endl;
	std::cout << "Http Version: " << request.getHTTPVersion() << std::endl;
	std::cout << "Host: " << request.getHost() << std::endl;
	std::cout << "Root: " << request.getBaseRoot() << std::endl;
	std::cout << "Rooted Uri: " << request.getRootedURI() << std::endl;
}

void Client::handleResponseState()
{
	std::cout << "\t\t\nHandle Response State" << std::endl;
	printRequestObject(_request);
	Response response(_request, &_location, *_serverPtr);
	_responseString = response.createResponseStr();
	_epoll.modifyFd(_fd, EPOLLOUT);
}

void Client::setRequestStr(std::string request)
{
	_requestString = request;
}

void Client::setServer(Server &server)
{
	_serverPtr = &server;
}

int Client::getSocketFd()
{
	return _socketFd;
}

int Client::getFd()
{
	return _fd;
}

std::string &Client::getRequestStr()
{
	return _requestString;
}

std::string &Client::getResponseStr()
{
	return _responseString;
}

Server *Client::getServer()
{
	return _serverPtr;
}

std::shared_ptr<Cgi> Client::getCgi()
{
	return _Cgi;
}

Request &Client::getRequest()
{
	return _request;
}

RequestParser &Client::getRequestParser()
{
	return _requestParser;
}

std::string &Client::getCgiExtension()
{
	return _cgi_extension;
}

Location &Client::getLocation()
{
	return _location;
}