/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 15:14:13 by maraasve          #+#    #+#             */
/*   Updated: 2025/05/14 13:03:43 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./Request.hpp"

Request::~Request()
{
}

std::string Request::getRedirectionURI() const
{
	return (_redirection_uri);
}

std::string Request::getMethod() const
{
	return (_method);
}

std::string Request::getURI() const
{
	return (_uri);
}

std::string &Request::getRootedURI()
{
	return (_rooted_uri);
}

std::string &Request::getErrorPagePath()
{
	return (_error_page_path);
}

std::string Request::getRootedURI() const
{
	return (_rooted_uri);
}

std::string Request::getQueryString() const
{
	return (_query_string);
}

std::string Request::getHTTPVersion() const
{
	return (_http_version);
}

std::string Request::getBody() const
{
	return (_body);
}

std::string Request::getPath() const
{
	return (_path);
}

std::string Request::getHost() const
{
	return (_host);
}
std::string &Request::getBaseRoot()
{
	return (_base_root);
}

std::unordered_map<std::string, std::string> Request::getHeaders() const
{
	return (_headers);
}

std::string Request::getErrorCode() const
{
	return (_error_code);
}

int Request::getFileType() const
{
	return _file_type;
}

ssize_t Request::getContentLength() const
{
	return (_content_length);
}

void Request::setErrorCode(std::string code)
{
	_error_code = code;
}

void Request::setURI(std::string uri)
{
	_uri = uri;
}

void Request::setRootedUri(std::string rootedUri)
{
	_rooted_uri = rootedUri;
}

void Request::setPort(std::string port)
{
	_port = port;
}

void Request::setErrorPagePath(std::string error_path)
{
	_error_page_path = error_path;
}

void Request::setHTTPVersion(std::string httpVersion)
{
	_http_version = httpVersion;
}

void Request::setBody(std::string body)
{
	_body = body;
}

void Request::setHeaders(std::unordered_map<std::string, std::string> headers)
{
	_headers = headers;
}

void Request::setBaseRoot(std::string baseroot)
{
	_base_root = baseroot;
}

void Request::setHost(std::string host)
{
	_host = host;
}

void Request::setMethod(std::string method)
{
	_method = method;
}

void Request::setPath(std::string path)
{
	_path = path;
}

void Request::setRequestLine(std::string method, std::string uri, std::string version)
{
	_method = method;
	_uri = uri;
	_http_version = version;
}

void Request::setFileType(int file_type)
{
	_file_type = file_type;
}

void Request::setContentLength(ssize_t contentLength)
{
	_content_length = contentLength;
}

void Request::setQueryString(std::string query_string)
{
	_query_string = query_string;
}

void Request::setRedirectionURI(std::string redirection_uri)
{
	_redirection_uri = redirection_uri;
}

void Request::addHeader(std::string key, std::string value)
{
	_headers.emplace(key, value);
}
