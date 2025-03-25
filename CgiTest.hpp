/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiTest.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marieke <marieke@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:10:01 by maraasve          #+#    #+#             */
/*   Updated: 2025/03/25 15:10:18 by marieke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGITEST_HPP
# define CGITEST_HPP

# include <iostream>
# include <map>
# include <unistd.h>
# include <sys/wait.h>
#include <signal.h>

# define TIMEOUT 5000

class CgiTest 
{
	private:
		pid_t	_cgiPid;
		int		_exitStatus;
		char	*_cgiPath;
		char	**_args;
		char	**_env;
	
	public:
		CgiTest();
		// CgiTest(const CgiTest &other);
		// CgiTest& operator=(const CgiTest &other);
		~CgiTest();

		std::string	executeCGI();

};

#endif
