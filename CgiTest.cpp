/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiTest.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:09:57 by maraasve          #+#    #+#             */
/*   Updated: 2025/03/24 18:11:46 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiTest.hpp"

CgiTest::CgiTest()
{
	_exitStatus = 0;
	_cgiPid = -1;
	
}

CgiTest::~CgiTest()
{
}

CgiTest::CgiTest(const CgiTest &other)
{
}

CgiTest&	CgiTest::operator=(const CgiTest &other)
{
}

std::string	CgiTest::executeCGI()
{
	int			pipeFD[2];
	int			waitResult;
	int			elapsedTime;
	char		buffer[1024];
	std::string	cgiOutput;
	size_t		bytesRead;

	if (pipe(pipeFD) == -1)
		perror("pipe");
	
	_cgiPid = fork();
	if (_cgiPid == -1)
		perror("fork"); // dont know what to do in this case yet

	if (_cgiPid == 0)
	{
		dup2( pipeFD[1], STDOUT_FILENO);
		close(pipeFD[0]);
		close(pipeFD[1]);
		execve(_cgiPath, _args, _env);
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
		usleep(100000);
		elapsedTime += 100;
	}
	if (elapsedTime > TIMEOUT)
	{
		kill(_cgiPid, SIGKILL);
		waitpid(_cgiPid, &_exitStatus, 0);
	}
	while((bytesRead = read(pipeFD[0], buffer, sizeof(buffer) -1)) > 0)
	{
		buffer[bytesRead] = '/0';
		cgiOutput += buffer;
	}
	if (bytesRead == -1)
		perror("read failed");
	return (cgiOutput);
}