/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiTest.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marieke <marieke@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:10:01 by maraasve          #+#    #+#             */
/*   Updated: 2025/03/28 16:28:57 by marieke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGITEST_HPP
# define CGITEST_HPP

# include <iostream>
# include <map>
# include <unistd.h>
# include <sys/wait.h>
# include <signal.h>
# include "utils.hpp"

# define TIMEOUT 5000

class CgiTest 
{
	private:
		pid_t		_cgiPid;
		int			_exitStatus;
		char		*_filePath;
		char		*_execPath;
		std::string	_filePathString;
		char		**_args;
		char		**_env;
	
	public:
		CgiTest();
		// CgiTest(const CgiTest &other);
		// CgiTest& operator=(const CgiTest &other);
		~CgiTest();

		void		setArgs();
		std::string	executeCGI();

};

#endif
