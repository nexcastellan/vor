#include "server.h"

#include <boost/algorithm/string/trim.hpp>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>

#include "load.h"
#include "program_options.h"
#include "stats.h"
#include "thread.h"
#include "utility.h"

class HandleRequestArgs {
public:
	Server *server;
	int sock;	
};

Server::Server(All_data_t& the_data) :
	data(the_data), search(the_data), sock(-1) {
		
	create_server();
}

Server::~Server() {
	if (this->sock != -1) {
		close(this->sock);
	}
}

void
Server::create_server() {
	// First, shut down any existing server process
	pid_t my_pid = getpid();
	for (std::vector<pid_t>::const_iterator it = child_pids.begin();
		it != child_pids.end();
		++it) {
			
		if (*it != my_pid) {
			kill(*it, SIGTERM);
		}
	}
	
	// Create socket
	if ((this->sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		throw "Unable to create socket";
	}
	
	// Set it reusable
	int opt = 1;
	setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	// Bind the socket
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(program_options->port());
	/* bind the socket to the port specified above */
	bool bound = false;
	for (unsigned int retries = 0; retries < 60; ++retries) {
		if (bind(this->sock, (struct sockaddr *)&address, sizeof(address)) != -1) {
			// Update stats with any data from old child
			global_stats->load_stats();
			// We loaded our own data, so increase the data-loaded count.
			global_stats->incrDataReloadFull();
			bound = true;
			break;
		} else {
			sleep(1);
		}
	}
	if (!bound) {
		throw "Unable to bind socket";
	}
}

void
Server::accept_connections() {
	if (listen(this->sock, 8) == -1) {
		throw "Unable to listen to socket";
	}
	
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in address;

	while (true) {
		int new_socket;
		new_socket = accept(this->sock, (struct sockaddr *)&address, &addrlen);
		if (new_socket < 0) {
			throw "Unable to accept connection";
		}
	
		HandleRequestArgs *args = new HandleRequestArgs;
		args->server = this;
		args->sock = new_socket;
		Thread::create(server_handle_request, static_cast<void *>(args), false);
	}
}

void
Server::handle_request(int incoming_socket) const {
	struct timeval tv_start_network;
	gettimeofday(&tv_start_network, NULL);

 	// Open as file, so we can do fgets, etc.
	FILE *conn = fdopen(incoming_socket, "r+");
	if (conn == NULL) {
		throw "Unable to handle incoming connection";
	}

	char buf[10240];
	
	// Get the arguments
	Id_t searcher_userid = 0;
	Id_t searcher_school = 0;
	Id_t searcher_location = 0;
	Params_t params; // General search parameters
	std::vector<Id_t> interests;
	bool perform_search = false;
	while (fgets(buf, sizeof(buf), conn)) {
		std::stringstream sbuf(buf);
		std::string key;
		std::string value;
		
		sbuf >> key;
		key = Utility::downcase(key);
		if ((key == "end") || (key == "quit") || (key == "exit")) {
			break;
		} else if (key == "help") {
			help(conn);
		} else if (key == "stats") {
			stats(conn);
			fclose(conn);
			return;
		} else if (key == "terminate") {
			exit(0);
		} else if (key == "reload") {
			fclose(conn);
			Load::reload_online_and_new(data);
			return;
		} else if (key == "searcher_userid") {
			sbuf >> searcher_userid;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "searcher_school") {
			sbuf >> searcher_school;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "searcher_location") {
			sbuf >> searcher_location;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "min_age") {
			sbuf >> value;
			params["min_age"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "max_age") {
			sbuf >> value;
			params["max_age"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "sex") {
			sbuf >> value;
			params["sex"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "name") {
			std::getline(sbuf, value);
			boost::algorithm::trim(value);
			params["name"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "interest") {
			sbuf >> value;
			char *end_ptr;
			interests.push_back(::strtol(value.c_str(), &end_ptr, 10));
			if (interests.size() == 1) {
				global_stats->incrSearchReq(key);
			}
			perform_search = true;
		} else if (key == "location") {
			sbuf >> value;
			params["location"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "school") {
			sbuf >> value;
			params["school"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "sexuality") {
			sbuf >> value;
			params["sexuality"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "with_picture") {
			sbuf >> value;
			params["with_picture"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "single") {
			sbuf >> value;
			params["single"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "birthday") {
			sbuf >> value;
			params["birthday"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "online") {
			sbuf >> value;
			params["online"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "new_users") {
			sbuf >> value;
			params["new_users"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "active_recently") {
			sbuf >> value;
			params["active_recently"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "might_know") {
			sbuf >> value;
			params["might_know"] = value;
			global_stats->incrSearchReq(key);
			perform_search = true;
		} else if (key == "no_friends") {
			sbuf >> value;
			params["no_friends"] = value;
		} else {
			fprintf(conn, "Unknown command.\n\n");
			help(conn);
		}
	}
	std::vector<Id_t> results;
	struct timeval tv_start_search, tv_end_search;
	if (perform_search) {
		// Increase overall searches, because an individual search can have
		// multiple parameters.
		global_stats->incrSearchReq("search_reqs");
		global_stats->incrInFlight();	
	
		// Do the search
		gettimeofday(&tv_start_search, NULL);
		results = this->search.do_search(
			searcher_userid, searcher_school, searcher_location,
			params, interests);
		if (results.size() > 1000) {
			results.resize(1000);
		}
		gettimeofday(&tv_end_search, NULL);
	}

	// Output the results
	if (program_options->verbose() >= 2) {
		std::cout << "Found " << results.size() << " matches" << std::endl;
	}
	for (std::vector<Id_t>::const_iterator it = results.begin();
		it != results.end();
		++it) {
			
		fprintf(conn, "%d\n", *it);
	}
	fclose(conn);
	
	// Update the time spent
	struct timeval tv_end_network;
	gettimeofday(&tv_end_network, NULL);
	// In milliseconds
	unsigned long time_spent;
	time_spent = (tv_end_network.tv_sec - tv_start_network.tv_sec) * 1000;
	time_spent += (tv_end_network.tv_usec - tv_start_network.tv_usec) / 1000;
	global_stats->incrSearchTimeNetwork(time_spent);
	if (perform_search) {
		time_spent = (tv_end_search.tv_sec - tv_start_search.tv_sec) * 1000;
		time_spent += (tv_end_search.tv_usec - tv_start_search.tv_usec) / 1000;
		global_stats->incrSearchTime(time_spent);
		global_stats->decrInFlight();
	}
}

void
Server::threaded_accept() {
	this->thread = Thread::create(server_accept_connections,
		static_cast<void *>(this));
}

void
Server::wait_on_threads() {
	pthread_join(this->thread, NULL);
}

void
Server::help(FILE *conn) const {
	if (conn == NULL) {
		throw "Unable to handle incoming connection";
	}
	fprintf(conn, "help                      display this info\n");
	fprintf(conn, "stats                     display statistics\n");
	fprintf(conn, "\n");
	fprintf(conn, "searcher_userid    <uid>  searcher's userid\n");
	fprintf(conn, "searcher_school    <id>   searcher's school id\n");
	fprintf(conn, "searcher_location  <id>   searcher's location\n");
	fprintf(conn, "min_age            <age>  minimum age in result set\n");
	fprintf(conn, "max_age            <age>  maximum age in result set\n");
	fprintf(conn, "sex                {m,f}  search only for this gender\n");
	fprintf(conn, "name               <str>  username/realname substring search\n");
	fprintf(conn, "interest           <id>   user interest\n");
	fprintf(conn, "                          line can be included multiple times\n");
	fprintf(conn, "location           <id>   only in this location (or children)\n");
	fprintf(conn, "school             <id>   only in this school\n");
	fprintf(conn, "sexuality          <x>    1 - heterosexual only\n");
	fprintf(conn, "                          2 - homosexual only\n");
	fprintf(conn, "                          3 - bisexual only\n");
	fprintf(conn, "with_picture       true   only include users with a profile pic\n");
	fprintf(conn, "single             true   only include single users\n");
	fprintf(conn, "birthday           true   birthday list\n");
	fprintf(conn, "online             true   only users currently online\n");
	fprintf(conn, "new_users          true   only new users\n");
	fprintf(conn, "active_recently    true   only users active in past 30 days\n");
	fprintf(conn, "might_know         true   prioritise users the searcher may know\n");
	fprintf(conn, "end                       perform search\n");
	fprintf(conn, "\nInternal commands:\n");
	fprintf(conn, "terminate                 shut down the server\n");
	fprintf(conn, "reload                    reload online and new users\n");
}

void
Server::stats(FILE *conn) const {
	if (conn == NULL) {
		throw "Unable to handle incoming connection";
	}
	fprintf(conn, "in_flight %u\n", global_stats->getInFlight());
	fprintf(conn, "running_time %d\n", global_stats->getRunningTime());
	fprintf(conn, "memory_use %lu\n",
		static_cast<long unsigned>(global_stats->getMemoryUse()));
	fprintf(conn, "search_time %lu\n", global_stats->getSearchTime());
	fprintf(conn, "search_time_network %lu\n",
	 	global_stats->getSearchTimeNetwork());
	fprintf(conn, "data_reloads_full %u\n", global_stats->getDataReloadFull());
	fprintf(conn, "data_reloads_fast %u\n", global_stats->getDataReloadFast());
	std::map<std::string, unsigned int> search_reqs;
	search_reqs = global_stats->getSearchReqs();
	for (std::map<std::string, unsigned int>::const_iterator it = search_reqs.begin();
		it != search_reqs.end();
		++it) {

		fprintf(conn, "%s %u\n", it->first.c_str(), it->second);
	}
}

void* server_accept_connections(void *arg) {
	Server *server = static_cast<Server *>(arg);
	server->accept_connections();
	return NULL;
}

void* server_handle_request(void *arg) {
	HandleRequestArgs *hra = static_cast<HandleRequestArgs *>(arg);
	hra->server->handle_request(hra->sock);
	delete hra;
	return NULL;
}

