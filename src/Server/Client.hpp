/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/07 15:06:59 by maraasve          #+#    #+#             */
/*   Updated: 2025/05/21 17:53:54 by maraasve         ###   ########.fr       */
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
# include "../CGI/Cgi.hpp"

# include <chrono>
# include <string>
# include <unordered_map>
# include <optional>
# include <set>
# include <experimental/filesystem>

# define TIMEOUT_CLIENT 30

enum class clientState {
	READING_HEADERS = 0,
	READING_BODY,
	PARSING_CHECKS,
	CGI,
	RESPONDING,
	ERROR
};

class Client : public EventHandler {
	private:
		clientState				_state = clientState::READING_HEADERS;
		int						_fd;
		Server*					_serverPtr;
		Location				_location;
		Epoll&					_epoll;
		id_t					_socketFd;
		std::string				_requestString;
		std::string				_responseString; 
		Request					_request;
		RequestParser			_requestParser;
		std::shared_ptr<Cgi>	_Cgi;
		std::string				_cgi_extension;
		std::chrono::steady_clock::time_point	_startTime;

	public:
		Client(int fd, Epoll& epoll, int socket_fd);
		~Client();

		void	handleIncoming() override;
		void	handleOutgoing() override;
		void	handleHeaderState();
		void	handleBodyState();
		void	handleParsingCheckState();
		void	handleCgiState();
		void	handleResponseState();
		void	handleErrorState();
		bool	resolveLocation(std::string uri);
	
		void	setRequestStr(std::string request);
		void	setServer(Server& server);
		void	setState(clientState state);
		
		int						getFd();
		int						getSocketFd();
		std::string&			getRequestStr();
		std::string&			getResponseStr();
		Server*					getServer();
		Request&				getRequest();
		RequestParser&			getRequestParser();
		std::shared_ptr<Cgi>	getCgi();
		std::string&			getCgiExtension();
		Location&				getLocation();
		std::chrono::steady_clock::time_point getStartTime() const;
		
		bool					shouldRunCgi();

		std::function<void(Client& client)>		assignServer;
		std::function<void(int, int)>			onCgiAccepted;
		std::function<void(int)>				closeClientConnection;
};

#endif
