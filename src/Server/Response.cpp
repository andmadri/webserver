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

std::string base64_encode(const std::string& in) {
    static const char* const base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    size_t len = in.length();
    size_t pad = len % 3;
    unsigned char char_array_3[3], char_array_4[4];

    for (size_t i = 0; i < len;) {
        char_array_3[0] = in[i++];
        char_array_3[1] = (i < len) ? in[i++] : 0;
        char_array_3[2] = (i < len) ? in[i++] : 0;

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) | ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) | ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; (j < 4); j++) {
            out += base64_table[char_array_4[j]];
        }
    }

    if (pad) {
        for (size_t i = 0; i < pad; i++) {
            out[out.size() - 1 - i] = '=';
        }
    }
    return out;
}

void Response::ErrorImageHTML(std::string& html_content) {
    std::string image_path = "./variables/img/error_img/" + _error_code + ".jpg";
    std::ifstream image_file(image_path, std::ios::binary);
    if (image_file) {
        std::ostringstream image_buffer;
        image_buffer << image_file.rdbuf();
        std::string image_data = image_buffer.str();
        std::string base64_image = base64_encode(image_data);
        std::string img_tag = "<img src=\"data:image/jpeg;base64," + base64_image + "\">";
        html_content += "<div>" + img_tag + "<div>";

    }
}

void Response::createErrorPage() {
    std::string filename = "./variables/errors/" + _error_code + ".html";
    std::ifstream file(filename);
    if (!file) {
        _body = "<h1>" + _error_code + "-" + _error_text + "</h1>";
    } else {
        std::stringstream buffer;
        buffer << file.rdbuf();
        _body = buffer.str();
    }
    createHeaders("text/html", std::to_string(_body.size()));
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
    createErrorPage();
    createHeaders("text/html", std::to_string(_body.size()));
}

std::string Response::formatStatusLine() {
    return "HTTP/1.1 " + _error_code + " " + _error_text + "\r\n";
}

std::string Response::formatHeaders() {
    std::stringstream ss;
    for (auto it = _headers.begin(); it != _headers.end(); ++it) {
        ss << it->first << ":" << it->second << "\r\n";
    }
    return ss.str();
}


bool    Response::checkAllowedMethods() {
    if (_location._allowed_methods.empty()) {
        return true;
    }
    for (const std::string& allowed_method : _location._allowed_methods) {
        std::cout << "This is the allowed: " << allowed_method << std::endl;
        if (allowed_method == _method) {
            return true;
        }
    }
    return false;
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
    _error_code = "403";
    return "ERROR";
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

void    Response::serverDirectoryListing(const std::string& dir_path, const std::string& uri_path) {
_body = createDirectoryListing(dir_path, uri_path);
        if (_error_code == "200") {
            createHeaders("text/html", std::to_string(_body.size()));
        }
}

std::string Response::setContentType(const std::string& path) {
    size_t pos = path.find_last_of(".");
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }
    std::string extension = path.substr(pos + 1);
    if ((extension == "html") | (extension == "htm")) {
        return "text/html";
    } else if ((extension == "jpg") | (extension == "jpeg")) {
        return "image/jpeg";
    } else if (extension == "txt") {
        return "text/plain";
    } else if (extension == "png") {
        return "image/png";
    } else {
        return "application/octet-stream";
    }
}

void    Response::serveFile(const std::string& file_path) {
    if (access(file_path.c_str(), R_OK) != 0) {
        setErrorResponse("403");
        return ;
    }
	//Here we would need to handle the CGI for GET
	if (isCGIScript(file_path)) {
		//CGI cgi(...)
		//epoll.add(cgi.getReadPipe(), ..)
		// add the cgi_pair of fd's to an unordered map
		//return;

	}
    std::ifstream file(file_path);
    if (!file) {
        setErrorResponse("500");
        return ;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    _body = buffer.str();
    if (_error_code == "200") {
        createHeaders(setContentType(file_path), std::to_string(_body.size()));
    }
    file.close();
}

bool isCGIScript(std::string& file_path) {
    size_t pos = file_path.find_last_of('.');
    if (pos == std::string::npos) {
        return false;
    }
    std::string ext = file_path.substr(pos + 1);
    return (ext == "py" || ext == "php");
}

void Response::handleGET(Request& request) {
    std::string file_type = checkRequestURI(_rooted_uri, R_OK);
    if (file_type == "ERROR") {
        setErrorResponse(_error_code);
    } else if (file_type == "ISDIR") {
        if (!_location._index.empty()) {
            serveFile(_rooted_uri + _location._index);
        } else if (_location._auto_index ) {
            serverDirectoryListing(_rooted_uri, request.getURI());
        } else if (!_server->getIndex().empty()) {
            serveFile(_rooted_uri + _server->getIndex());
        } else if (_server->getAutoIndex()) {
            serverDirectoryListing(_rooted_uri, request.getURI());
        } else {
            setErrorResponse("404");
        }
    } else if (file_type == "ISFILE") {
        if (isCGIScript(_rooted_uri)) {
            
        } else {
            serveFile(_rooted_uri);
        }
    }
}

std::string Response::findFileName(Request& request) {
	auto it = request.getHeaders().find("Content-Disposition");
	if (it == request.getHeaders().end()) {
		std::cout << "HERE 111111111111" << std::endl;
		return "unknown-file";
	}
	auto it_second = it->second.find("filename=");
	if (it_second == std::string::npos) {
		std::cout << "HERE 222222222222222" << std::endl;
		return "unknown-file";
	}
	std::string file_name = it->second.substr(it_second + 1, it->second.find_last_not_of('\"'));
	return file_name;
}

void Response::uploadFile(Request& request) {
	std::string file_name = _rooted_uri + findFileName(request);
	std::cout << file_name << std::endl;
    std::ofstream outfile(file_name.c_str(), std::ios::out | std::ios::trunc);
    if (!outfile.is_open()) {
        setErrorResponse("500");
        return ;
    }
    outfile << request.getBody();
    if (outfile.fail()) {
        setErrorResponse("500");
        outfile.close();
        return ;
    }
    outfile.close();
    _error_code = "201";
    _error_text = "Created";
    _body = "<html><body>Upload Successful</body></html>";
}

void Response::handlePOST(Request& request) {
    if (checkPostTooBig(request.getHeaders())) {
        if (!_error_code.empty()) { //in the case we set it already inside the checkPostTooBig
            setErrorResponse("413");
        }
    }
    uploadFile(request);
}

// void Response::handleDELETE(Request& request) {
//     //how should you handle DELETE
// }

void printRequestObject(Request& request) {
    std::cout << "---REQUES FROM CLIENT---" << std::endl;
    // std::cout << request.getMethod() << std::endl;
    // std::cout << request.getURI() << std::endl;
    // std::cout << request.getHTTPVersion() << std::endl;
    // std::cout << request.getHost() << std::endl;
	//print the headers;
	// for (auto it = request.getHeaders().begin() ; it != request.getHeaders().end() ; ++it) {
	// 	std::cout << it->first << ": " << it->second << std::endl;
	// }
    //std::cout << request.getBody().substr(0, 700) << std::endl;
}

void Response::handleRequest(Request& request) {
    std::string method = request.getMethod();
    std::string error_code = request.getErrorCode();
    // printRequestObject(request);
    if (error_code != "200") {
		std::cout << "It comes from the request" << std::endl;
        setErrorResponse(error_code);
    }
    else if (method == "GET") {
        handleGET(request);
    } else if (method == "POST") {
		std::cout << " We are going to handle POST " << std::endl;
        handlePOST(request); 
	} else if (method == "DELETE") {
		//handleDELETE()
	}
    else {
        setErrorResponse("400");
    }
}

std::string Response::createResponseStr(Request& request, Server* server) {
    
    setServer(server);
    handleRequest(request);
	//check if we have cgi if so response.str would remain empty and 
	// return "";
    
    std::stringstream response;
    response << formatStatusLine();
    response << formatHeaders();
    response << "\r\n";
    response << _body;
    // std::cout << response.str() << std::endl;
    return response.str();
}
