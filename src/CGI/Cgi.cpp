#include "Cgi.hpp"
#include "../Server/Client.hpp"

Cgi::Cgi(Client *client) : _client(client), _request(client->getRequest()), _filePathString(_request.getRootedURI()),
						   _extension(client->getCgiExtension()), _filePath(_filePathString.c_str()), _exitStatus(0),
						   _state(cgiState::INITIALIZED), _writeToChild{-1, -1}, _readFromChild{-1, -1}, _cgiPid(-1),
						   _body(_request.getBody()), _method(_request.getMethod()), _args(nullptr), _env(nullptr)
{
}

Cgi::~Cgi()
{
	int	status;
	pid_t result = waitpid(_cgiPid, &status, WNOHANG);
	if (result == 0)
	{
		kill(_cgiPid, SIGQUIT);
	}
	else if (_state == cgiState::ERROR)
	{
		std::cout << "The CGI had an error" << std::endl;
	}
	std::cout << "The CGI is deleted" << std::endl;
	freeArgs(_args);
	freeArgs(_env);
}

bool Cgi::init()
{
	_args = setArgs();
	_execPath = getExecPath();
	_env = setUpEnvironment();
	if (!_args || _execPath.empty() || !_env)
	{
		errorHandler(_args);
		_client->handleIncoming();
		return false;
	}
	return true;
}

bool Cgi::childExited()
{
	int status;
	pid_t result = waitpid(_cgiPid, &status, WNOHANG);
	if (result == _cgiPid)
	{
		return true;
	}
	else if (result == -1)
	{
		return true;
	}
	std::cout << "CGI: Child exited" << std::endl;
	return false;
}

void Cgi::handleIncoming()
{
	char buffer[BUFSIZ];
	std::cout << "CGI: Parent reads output from Child" << std::endl;
	ssize_t bytes = read(_readFromChild[0], buffer, sizeof(buffer));
	if (bytes > 0)
	{
		_body.append(buffer, bytes);
	}
	else if (bytes < 0)
	{
		errorHandler(_args);
		_client->handleIncoming();
		return;
	}
	if (childExited() || !bytes)
	{
		if (onCgiPipeDone)
		{
			onCgiPipeDone(_readFromChild[0]);
		}
		close(_readFromChild[0]);
		_readFromChild[0] = -1;
		_state = cgiState::COMPLETE;
		std::cout << "CGI: COMPLETE" << std::endl;
		_client->handleIncoming();
		return;
	}
}

void Cgi::handleOutgoing()
{
	size_t writeSize;
	if (_body.size() > 0)
	{
		if (_body.size() < BUFSIZ)
		{
			writeSize = _body.size();
		}
		else
		{
			writeSize = BUFSIZ;
		}
		ssize_t bytes = write(_writeToChild[1], _body.c_str(), writeSize);
		if (bytes > 0)
		{
			_body.erase(0, bytes);
		}
		else if (bytes < 0)
		{
			close(_writeToChild[1]);
			_writeToChild[1] = -1;
			errorHandler(_args);
			_client->handleIncoming();
			return;
		}
	}
	if (_body.empty() && _writeToChild[1] != -1 && onCgiPipeDone)
	{
		onCgiPipeDone(_writeToChild[1]);
		close(_writeToChild[1]);
		_writeToChild[1] = -1;
		std::cout << "CGI: SENDING BODY DONE!" << std::endl;
	}
}

char **Cgi::vecTo2DArray(std::vector<std::string> &vec)
{
	size_t size = vec.size();
	char **array = static_cast<char **>(malloc((size + 1) * sizeof(char *)));
	if (!array)
	{
		return nullptr;
	}
	for (size_t i = 0; i < size; ++i)
	{
		array[i] = strdup(vec[i].c_str());
		if (!array[i])
		{
			errorHandler(array);
			return nullptr;
		}
	}
	array[size] = nullptr;
	return array;
}

char **Cgi::setArgs()
{
	std::vector<std::string> argsv;

	if (_extension == "py")
	{
		argsv.emplace_back("python3");
	}
	else if (_extension == "php")
	{
		argsv.emplace_back("php-cgi");
	}
	argsv.emplace_back(_filePath);
	return vecTo2DArray(argsv);
}

std::string Cgi::getExecPath()
{
	const char *pathStr = getenv("PATH");
	if (!pathStr)
		return "";
	std::string command(_args[0]);
	std::string path;
	std::stringstream stream(pathStr);
	while (std::getline(stream, path, ':'))
	{
		std::string full_path = path + "/" + command;
		if (access(full_path.c_str(), X_OK) == 0)
		{
			return full_path;
		}
	}
	return "";
}

void Cgi::executeChildProcess()
{
	std::cout << "CGI: Child Executing" << std::endl;
	if (_method == "POST")
	{
		close(_writeToChild[1]);
		_writeToChild[1] = -1;
		if (dup2(_writeToChild[0], STDIN_FILENO) == -1)
		{
			errorHandler(_args);
			exit(1);
		}
		close(_writeToChild[0]);
		_writeToChild[0] = -1;
	}
	close(_readFromChild[0]);
	_readFromChild[0] = -1;
	if (dup2(_readFromChild[1], STDOUT_FILENO) == -1)
	{
		errorHandler(_args);
		exit(1);
	}
	close(_readFromChild[1]);
	_readFromChild[1] = -1;
	closeInheritedFds();
	execve(_execPath.c_str(), _args, _env);
	freeArgs(_args);
	freeArgs(_env);
	exit(1);
}

void Cgi::startCgi()
{
	std::cout << "CGI: Starting CGI" << std::endl;
	if (_method == "POST")
	{
		if (pipe(_writeToChild) == -1)
		{
			errorHandler(_args);
			_client->handleIncoming();
			return;
		}
		_client->onCgiAccepted(_writeToChild[1], EPOLLOUT);
	}
	if (pipe(_readFromChild) == -1)
	{
		errorHandler(_args);
		_client->handleIncoming();
		return;
	}
	_client->onCgiAccepted(_readFromChild[0], EPOLLIN | EPOLLHUP);
	_cgiPid = fork();
	if (_cgiPid < 0)
	{
		std::cerr << "CGI: Error Forking" << std::endl;
		errorHandler(_args);
		_client->handleIncoming();
		return;
	}
	if (_cgiPid == 0)
	{
		executeChildProcess();
	}
	else
	{
		std::cout << "CGI: Parent Process Forked & Closing Unnecessary Pipes" << std::endl;
		if (_method == "POST")
		{
			close(_writeToChild[0]);
			_writeToChild[0] = -1;
		}
		close(_readFromChild[1]);
		_readFromChild[1] = -1;
	}
}

int Cgi::getWriteFd()
{
	return _writeToChild[1];
}

int Cgi::getReadFd()
{
	return _readFromChild[0];
}

void Cgi::freeArgs(char **array)
{
	if (array)
	{
		size_t i = 0;
		while (array[i])
		{
			free(array[i]);
			i++;
		}
		free(array);
		array = nullptr;
	}
	return;
}

void Cgi::setBody(std::string body)
{
	_body = body;
}

char **Cgi::setUpEnvironment()
{
	std::vector<std::string> env_strings;

	if (!_body.empty())
	{
		std::string content_length = std::to_string(_body.size());
		env_strings.push_back("CONTENT_LENGTH=" + content_length);
	}
	env_strings.push_back("REQUEST_METHOD=" + _method);
	auto headers = _request.getHeaders();
	auto it = headers.find("Content-Type");
	if (it != headers.end())
	{
		env_strings.push_back("CONTENT_TYPE=" + it->second);
	}
	if (!_request.getQueryString().empty())
	{
		env_strings.push_back("QUERY_STRING=" + _request.getQueryString());
	}
	if (_client->getLocation().getUploadDir().empty())
	{
		if (_request.getBaseRoot().empty())
		{
			_client->getLocation().setUploadDir("./uploads");
		}
		_client->getLocation().setUploadDir(_request.getBaseRoot());
	}
	if (_client->getLocation().getUploadDir()[0] != '.')
	{
		_client->getLocation().setUploadDir("." + _client->getLocation().getUploadDir());
	}
	env_strings.push_back("UPLOAD_DIR=" + _client->getLocation().getUploadDir());
	return vecTo2DArray(env_strings);
}

std::string Cgi::getBody() const
{
	return _body;
}

cgiState Cgi::getState() const
{
	return _state;
}

int Cgi::getExitStatus() const
{
	return _exitStatus;
}

pid_t	Cgi::getPid()
{
	return _cgiPid;
}

void Cgi::errorHandler(char **array)
{
	if (_state == cgiState::ERROR)
	{
		return;
	}
	_state = cgiState::ERROR;
	freeArgs(array);
	freeArgs(_env);
	if (_writeToChild[0] != -1)
	{
		close(_writeToChild[0]);
	}
	if (_writeToChild[1] != -1)
	{
		close(_writeToChild[1]);
	}
	if (_readFromChild[0] != -1)
	{
		close(_readFromChild[0]);
	}
	if (_readFromChild[1] != -1)
	{
		close(_readFromChild[1]);
	}
}
