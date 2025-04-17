#include "./WebServer.hpp"

WebServer::WebServer(const std::string& config_file) {
	ConfigParser parser(config_file, _servers); //_servers is not set in Webserver
	_servers = parser.getServers(); //Thats why im doing this here now, but it's still passed to parser
}

void WebServer::run() {
	setupServerSockets(_epoll);

	while(true) {
		int	ready_fds = _epoll.getReadyFd();
		struct epoll_event *ready_events = _epoll.getEvents();
		for (int i = 0; i < ready_fds; ++i) {
			struct epoll_event event = ready_events[i];
			int event_fd = event.data.fd;
			auto handler = _eventHandlers.find(event_fd);
			if (handler != _eventHandlers.end()) {
				if (event.events & EPOLLIN) {
					handler->second->handleIncoming();
				}
				if (event.events & EPOLLOUT) {
					handler->second->handleOutgoing();
				}
			}
		}
	}
}

void	WebServer::handleNewClient(int client_fd, Server &server) {
	auto newClient = std::make_shared<Client>(client_fd, _epoll, server.getSocketFd());
	_eventHandlers[client_fd] = newClient;
	_epoll.addFd(client_fd, EPOLLIN);
	newClient->assignServer = [this, newClient]() mutable {
		this->assignServer(*newClient);
	};
}

void	WebServer::assignServer(Client &client) {
	int			fd = client.getFd();
	std::string	host = client.getRequest().getHost();

	for (Server& server : _servers) {
		if (fd == server.getSocketFd()) {
			for (std::string serverName : server.getServerNames()) {
				if (host == serverName) { //if host includes port (Host: example.com:8080) we need to trim that off first, and host might be case insensitive
					client.setServer(server); //if socket matches, but not hostname, we can implement fallback server that matched. NGINX works likes this, but we'll see
					return ;
				}
			}
		}
	}
	client.getRequest().setErrorCode("400");
}

void WebServer::setupServerSockets(Epoll& epoll) {
	std::unordered_map <std::pair<u_long, unsigned int>, std::shared_ptr<Socket>, hashPair> addressToFd;

	for (auto& server : _servers) {
		u_long			host = server.getHost_u_long();
		unsigned int	port = server.getPort();
		std::pair<u_long,unsigned int> key = {host, port};

		int	socketFd;
		if (addressToFd.find(key) == addressToFd.end()) {
			auto serverSocket = std::make_shared<Socket>();
			socketFd = serverSocket->getSocketFd();
			serverSocket->bindSocket(port, host);
			serverSocket->listenSocket();
			epoll.addFd(socketFd, EPOLLIN);
			addressToFd[key] = serverSocket;
			server.setSocket(serverSocket);
			server.onClientAccepted = [this, &server](int client_fd) {
				this->handleNewClient(client_fd, server);
			};
			_eventHandlers[socketFd] = std::make_shared<Server>(server);
		}
		else {
			server.setSocket(addressToFd[key]);
		}
	}
}

