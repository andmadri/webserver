/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 15:06:59 by maraasve          #+#    #+#             */
/*   Updated: 2025/04/16 17:54:08 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "./Request.hpp"
# include "../Parsing/RequestParser.hpp"
# include "./Server.hpp"
# include "./Response.hpp"
# include "../Networks/Epoll.hpp"
# include "EventHandler.hpp"

# include <string>
# include <unordered_map>
# include <optional>

enum clientState {
	READING_HEADERS = 0,
	READING_BODY,
	READY,
	CGI,
	RESPONDING,
	COMPLETE,
	ERROR
};

class Client : public EventHandler {
	private:
		int						_state = READING_HEADERS;
		int						_fd;
		Server*					_serverPtr;
		Epoll&					_epoll;
		id_t					_socketFd;
		std::string				_requestString;
		std::string				_responseString; 
		std::optional<Request>	_request;
		RequestParser			_requestParser;
		Response				_response;

	public:
		Client(int fd, Epoll& epoll, int socket_fd);

		void			parseRequest();
		void			handleIncoming() override;
		void			handleHeaderState();
		void			handleBodyState();
		void			handleCGIState();
		void			handleResponseState();
		void			handleErrorState();
		void			handleCompleteState();
		bool			sendResponse();

		void			setRequestStr(std::string request);
		void			setResponseStr(Request& request);
		void			setServer(Server& server);
		
		int				getFd();
		std::string&	getRequestStr();
		std::string&	getResponseStr();
		Server*			getServer();
		Request&		getRequest();
		
		void			closeConnection();

		std::function<void()>	assignServer;
};

#endif
