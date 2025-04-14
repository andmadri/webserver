/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiTest.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:09:57 by maraasve          #+#    #+#             */
/*   Updated: 2025/04/14 18:29:53 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiTest.hpp"

CgiTest::CgiTest()
{
	_filePath = (char *)"test.py";
	_filePathString = "test.py";
	_exitStatus = 0;
	_cgiPid = -1;
	setArgs();
	if (!_args)
		return ; //error handling
	_execPath = getExecPath();
	if (!_execPath)
	{
		freeArray(_args);
		return ; //error handling
	}
	_env = nullptr; //need to do something with this
}

CgiTest::~CgiTest()
{
	freeArray(_args);
	free(_execPath);
}

// CgiTest::CgiTest(const CgiTest &other)
// {
// }

// CgiTest&	CgiTest::operator=(const CgiTest &other)
// {
// }

void	CgiTest::setArgs()
{
	std::string					extension;
	std::vector<std::string>	argsv;

	extension = _filePathString.substr(_filePathString.find_last_of('.') + 1);
	if (extension == "py")
		argsv.push_back("python3");
	else if (extension == "php")
		argsv.push_back("php-cgi");
	//else
		//invalid script return???

	//get args from querystring from request

	argsv.push_back(_filePath);
	_args = vecToArray(argsv);
}

char	*CgiTest::getExecPath()
{
	char						*execPath;
	char						*pathStr;
	std::vector <std::string>	pathVec;

	pathStr = getenv("PATH"); // can we use this?
	if (!pathStr)
		return (nullptr); //error handling
	pathVec = vecSplit(pathStr, ':');
	for (std::string str : pathVec)
	{
		str += "/" + (std::string)_args[0];
		if (access(str.c_str(), X_OK) == 0)
		{
			execPath = strdup(str.c_str());
			if (!execPath)
				return (nullptr); // errorhandling sterror??
			return (execPath); // i can just return execPath if allocation fails, will return nullptr anyway
		}
	}
	return (nullptr);
}

std::string	CgiTest::executeCGI()
{
	int			pipeFD[2];
	int			waitResult;
	int			elapsedTime = 0;
	char		buffer[1024];
	std::string	cgiOutput;
	size_t		bytesRead;

	if (pipe(pipeFD) == -1)
	{
		perror("pipe");
		return (nullptr);
	}
	
	_cgiPid = fork();
	if (_cgiPid == -1)
	{
		perror("fork"); // error handling
		return (nullptr);
	}

	if (_cgiPid == 0)
	{
		dup2( pipeFD[1], STDOUT_FILENO);
		dup2( pipeFD[0], STDIN_FILENO);
		close(pipeFD[0]);
		close(pipeFD[1]);
		execve(_execPath, _args, _env);
		perror("execve failure");
		//didnt allocate anything here but in case i should free childs memory first
		exit(EXIT_FAILURE);
	}
	close(pipeFD[1]);
	while (elapsedTime < TIMEOUT)
	{
		waitResult = waitpid(_cgiPid, &_exitStatus, WNOHANG); //doing this otherwise i think it could become blocking
		if (waitResult == _cgiPid)
			break ;
		usleep(100000); //i dont think we're allowed to use usleep, maybe we should do something with epoll here or select???
		elapsedTime += 100;
	}
	if (elapsedTime > TIMEOUT)
	{
		kill(_cgiPid, SIGKILL);
		waitpid(_cgiPid, &_exitStatus, 0);
	}
	while((bytesRead = read(pipeFD[0], buffer, sizeof(buffer) -1)) > 0)
	{
		buffer[bytesRead] = '\0';
		cgiOutput += buffer;
	}
	close(pipeFD[0]);
	if (bytesRead == -1)
	{
		perror("read failed");
		return (nullptr);
	}
	return (cgiOutput);
}