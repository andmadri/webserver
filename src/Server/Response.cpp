// void Response::sendErrorResponse(int client_fd, int status_code, const std::string& msg) {
//     //I need to check if I can send the client anything with EPOLLOUT right?
//     std::string body = loadErrorPage(status_code);
//     std::stringstream response;
//     response << "HTTP/1.1 " << status_code << " " << msg << "\r\n";
//     //response << "Server: " << *OUR SERVER NAME*
//     response << "Content-Type: text/html; charset=UTF-8/\r\n";
//     response << "Content-Length: " << body.size() << "\r\n";
//     response << "Connection: close\r\n\r\n";
//     response << body;
//     //need to check if I can send the thing to the client right?
// }

/*
Status Line = HTTP/1.1 201 Created
Headers: General | Response | Entity
Empty Line: /r/n/r/n
Body:

if _error_code is empty then that means we did not set up an error before that
*/

/*
    Errors handled by the request parser:
    501 (Not Implemented) --> when the server does not recognize the method

    505 (HTTP Version Not Supported) --> The response SHOULD contain
   an entity describing why that version is not supported and what other
   protocols are supported by that server.

   400 (Bad Request)

   Errors that need to be handled by the response:

   403 (Forbidden) --> Directory listening is off and there is no index.html, a file exists but it has
   no read permissions for your server. A directory exists but it has autoindex off and there is no index file

   404 (Not Found) --> Nothing matches the Request-URI

   405 (Method Not Allowed) --> Method specified in the request line is not allowed
   for the resource identified by the Request-URI. The response MUST include an Allow header
   containing a list of valid methods for the requested resource.

   408 (Request Timeout) --> The client did not produce a request within the time the server was prepared to wait
   Do we need this??

   411 (Length Required) --> The server refused to accept the request without a defined Content-Length header

   412??

   413 (Request Entity Too Large)

   500 (Internal Server Error) --> The server encountered an unexpected condition which prevented it from fulfilling the reques

   502 (Bad Gateway) --> The server while acting as a gateway (CGI?) received an invalid response
   from the upstream server it accessed in attempting to fulfill the request



*/


#include "./Response.hpp"

Response::Response() : _errorsTexts({
    {"200", "OK"},
    {"201", "Created"},
    {"400", "Bad Request"},
    {"403", "Forbidden"},
    {"404", "Not Found"},
    {"405", "Method Not Allowed"},
    {"408", "Request Timeout"},
    {"413", "Request Entity Too Large"},
    {"500", "Internal Server Error"},
    {"501", "Not Implemented"},
    {"502", "Bad Gateway"},
    {"505", "HTTP Version Not Supported"}
}) {}

void Response::setServer(Server* server) {
    _server = server;
}

void Response::createHeaders(const std::string& content_type, const std::string& content_length) {
    _headers["Content-Type"] = content_type;
    _headers["Content-Length"] = content_length;
    _headers["Connection"] = "close";
}

std::string Response::createErrorPage(const std::string& error_code) {
    std::string filename = "/var/errors/" + error_code + ".html";
    std::ifstream file(filename);
    if (!file) {
        if (error_code != "500") {
            return loadErrorPage("500");
        }
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Response::setErrorText(const std::string& error_code) {
    _error_code = error_code;
    auto it = _errorsTexts.find(error_code);
    if (it != _errorsTexts.end()) {
        _error_text = it->second;
    } else {
        _error_text = "Unknown";
    }
}

void Response::setErrorResponse(const std::string& error_code) {
    setErrorText(error_code);
    _body = loadErrorPage(error_code);
    createHeaders("text/html", std::to_string(_body.size()));
}

std::string Response::formatStatusLine() {
    return "HTTP/1.1 " + _error_code + " " + _error_text + "\r\n";
}

std::string Response::formatHeaders() {
    std::stringstream ss;
    for (const auto& [key, value] : _headers) {
        ss << key << ": " << value << "\r\n";
    }
    return ss.str();
}


bool Response::checkAllowedMethods(const std::string& method) {
    if (_location._allowed_methods.empty()) {
        return true;
    }
    for (const std::string& allowed_method : _location._allowed_methods) {
        if (allowed_method == method) {
            return true;
        }
    }
    return false;
}

bool Response::checkMatchURI(const std::string& uri) {
    for (const auto& location : _server->getLocations()) {
        if (location._path == uri) {
            if (!location._root.empty()) {
                _rooted_uri = location._root + uri;
            } else {
                _rooted_uri = _server->getRoot() + uri;
            }
            _location = location;
            return true;
        }
    }
    return false;
}

bool Response::checkPostTooBig(const std::unordered_map<std::string, std::string> request_headers) {
    auto it = request_headers.find("Content-Length");
    if (it != request_headers.end()) {
        try {
            unsigned long long content_length = std::stoull(it->second);
            if (_location._client_max_body > 0 && content_length <= _location._client_max_body) {
                return false;
            } else if (content_length <= _server->getClientMaxBody()) {
                return false;
            }
        } catch (const std::exception& e) {
            setErrorResponse("500"); //Internal Error with Server due 
            return true;
        }
    } else  {
        setErrorResponse("411"); //Content-Length is not provided (Length Required)
        return true;
    }
    return true;
}

std::string Response::checkRequestURI(const std::string& rooted_uri, int mode) {
    struct stat sb;
    if (stat(rooted_uri.c_str(), &sb) == -1) {
        _error_code = "404";
        return "ERROR";
    }
    if (access(_rooted_uri.c_str(), mode) != 0) {
        _error_code = "403";
        return "ERROR";
    }
    if (S_ISDIR(sb.st_mode)) {
        return "ISDIR";
    } else if (S_ISREG(sb.st_mode)) {
        return "ISFILE";
    }
}

bool Response::checkEnabledAutoIndex() {
    return _location._auto_index || _server->getAutoIndex();
}

std::string Response::createDirectoryListing(const std::string& dir_path, const std::string& uri_path) {
    DIR* dirp = opendir(dir_path.c_str());
    if (!dirp) {
        setErrorResponse("404");
        return "";
    }
    std::stringstream html;
    html << "<html><body><h1>Index of " << uri_path << "</h1><ul>";

    struct dirent *dp;
    while (!(dp = readdir(dirp))) {
        std::string name = dp->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        html << "<li><a href=" << uri_path << "/" << name << "\">" << "</a></li>";
    }
    html << "</ul></body></html>";
    closedir(dirp);
    return html.str();
}

void Response::handleGET(Request& request) {
    std::string file_type = checkRequestURI(_rooted_uri, R_OK);
    if (file_type == "ERROR") {
        setErrorResponse(_error_code);
    } else if (file_type == "ISDIR") {
        if (checkEnabledAutoIndex() && check) {
            _body = createDirectoryListing(_rooted_uri, request.getURI());
            if (!_error_code.empty()) {
                createHeaders("text/html", std::to_string(_body.size()));
                _error_code = "200";
                _error_text = "OK";
            }
        }

    } else if (file_type == "ISFILE") {
        //serve the file
    }
}

void Response::handlePOST(Request& request) {
    if (checkPostTooBig(request.getHeaders())) {
        if (!_error_code.empty()) { //in the case we set it already inside the checkPostTooBig
            setErrorResponse("413");
        }
    }

}

void Response::handleDELETE(Request& request) {

}


void Response::handleRequest(Request& request) {
    std::string& method = request.getMethod();
    std::string& error_code = request.getErrorCode();

    if (!error_code.empty()) {
        setErrorResponse(error_code);
    } else if (!checkMatchURI(request.getURI())) {
        setErrorResponse("404");
    } else if (!checkAllowedMethods(method)) {
        setErrorResponse("405");
    }
    else if (method == "GET") {
        handleGET(request);
    } else if (method == "POST") {
        handlePOST(request);
    } else if (method == "DELETE") {
        handleDELETE(request);
    } else {
        setErrorResponse("400");
    }
}

std::string Response::createResponseStr(Request& request, Server* server) {
    setServer(server);
    handleRequest(request);
    
    std::stringstream response;
    response << formatStatusLine();
    response << formatHeaders();
    response << "\r\n";
    response << _body;
    return response.str();
}
