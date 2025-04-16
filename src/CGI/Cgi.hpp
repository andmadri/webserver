
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:10:01 by maraasve          #+#    #+#             */
/*   Updated: 2025/03/31 13:34:56 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include <iostream>
# include <map>
# include <vector>
# include <unistd.h>
# include <sys/wait.h>
# include <signal.h>

# define TIMEOUT 5000

class Cgi {
	private:
		pid_t		_cgiPid;
		int			_exitStatus;
		char		*_filePath;
		char		*_execPath;
		std::string	_filePathString;
		char		**_args;
		char		**_env;
        int         _pipefd[2];
	
	public:
		Cgi();
		// Cgi(const Cgi &other);
		// Cgi& operator=(const Cgi &other);
		~Cgi();

		void		setArgs();
		char		*getExecPath();
		std::string	executeCGI();
        int         getWriteFd();
        int         getReadFd();

        char						**vecToArray(std::vector<std::string> vec);
        std::vector<std::string>	vecSplit(char *str, char delim);
        void						freeArray(char **array);

};

//why does _args has to be a char ** can it be a vector of vectors?
//we need to know where we are calling the CGI and what to do in case of problems so when we return we gotta just handle the exceptions


#endif
