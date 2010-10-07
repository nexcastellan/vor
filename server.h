#ifndef _SERVER_H_
#define _SERVER_H_

#include "search.h"

class Server {
public:
	Server(All_data_t &the_data);
	virtual ~Server();
	
	// Bind to the necessary ports and prepare to accept connections.
	void create_server();
	
	// Spawn a new thread to handle incoming connections.
	// Those incoming connections are each handled in their own thread.
	void threaded_accept();
	
	// Do not return until the connection handling thread is finished.
	// Note that we detach the incoming connections, so we do not care
	// about those.
	void wait_on_threads();

private:
	friend void* server_accept_connections(void *);
	friend void* server_handle_request(void *);
	
private:
	// Accept incoming connections, and respond to search requests.
	void accept_connections();

	// Process the incoming request, do the search, and return results.
	void handle_request(int incoming_socket) const;

	// Output server stats to the socket
	void stats(FILE *conn) const;
	
	// Output help info to the socket
	void help(FILE *conn) const;
	
private:
	Server();
	Server(const Server& other);
	Server& operator=(const Server& other);

private:
	All_data_t &data;
	Search search;
	int sock;
	pthread_t thread;
};

// Remap back to Server::accept_connections
void* server_accept_connections(void *);

// Remap back to Server::handle_request
void* server_handle_request(void *);

#endif
