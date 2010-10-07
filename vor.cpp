#include <boost/shared_ptr.hpp>
#include <cstring>
#include <curl/curl.h>
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "data_structures.h"
#include "load.h"
#include "program_options.h"
#include "server.h"
#include "stats.h"

// Reload the new users and users online data.
void reload_data_fast(int);

// Reload all the data.
void reload_data_slow(int);

// Deal with dead child
void dead_child(int);

// Child is dying, dump the stats to shared memory so they can
// be loaded by the replacement.
void dump_stats(int);

// Parent is shutting down, so clear the stats shared memory.
void shutdown(int);

// Handle timed refreshes of data.
void parent();

// Handle data loading and searching.
void child();

char *program_name;
pid_t parent_pid;

int main(int argc, char *argv[]) {
	program_name = argv[0];
	parent_pid = getpid();
	
	boost::shared_ptr<ProgramOptions> new_program_options(
		new ProgramOptions(argc, argv));
	program_options.swap(new_program_options);
	
	boost::shared_ptr<Stats> new_stats(
		new Stats(data, parent_pid));
	global_stats.swap(new_stats);

	int fork_pid = fork();
	if (fork_pid < 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Fork error" << std::endl;
		}
		exit(1);
	} else if (fork_pid > 0) {
		child_pids.push_back(fork_pid);
		parent();
	} else {
		strncpy(program_name, "vor-child", strlen(program_name));
		child();
	}
	
	return 0;
}

void parent() {
	// Set up a timer to handle data reloading
	sigset_t new_set;
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGHUP);
	sigaddset(&new_set, SIGTERM);
	sigprocmask(SIG_UNBLOCK, &new_set, NULL);
	signal(SIGHUP, reload_data_slow);
	signal(SIGCHLD, dead_child);
	signal(SIGTERM, shutdown);
	signal(SIGINT, shutdown);
	
	while (true) {
		sleep(60); // forever, or at least until we get SIGTERM
	}
}

void child() {
	signal(SIGHUP, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGTERM, dump_stats);
	signal(SIGINT, dump_stats);
	
	curl_global_init(CURL_GLOBAL_ALL);

	if (program_options->verbose() >= 1) {
		std::cout << "Loading data..." << std::endl;
	}
	if (Load::load_all_data(data)) {
		time_t ltime;
		ltime = time(&ltime);
		localtime_r(&ltime, &last_data_loaded);
		last_data_loaded.tm_yday--;
		if (program_options->verbose() >= 1) {
			std::cout << "Data loaded" << std::endl;
		}
	}
	
	// Set up a timer to handle data reloading
	sigset_t new_set;
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &new_set, NULL);
	struct itimerval tout_val;
	tout_val.it_interval.tv_sec = 0;
	tout_val.it_interval.tv_usec = 0;
	tout_val.it_value.tv_sec = program_options->reload_frequency();
	tout_val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &tout_val, 0);
	signal(SIGALRM, reload_data_fast);
	
	// Start up our own server
	try {
		Server server(data);
		if (program_options->verbose() >= 0) {
			std::cout << "Server ready for connections" << std::endl;
		}
		server.threaded_accept();
		server.wait_on_threads();
	} catch (char const *e) {
		if (program_options->verbose() >= 0) {
			std::cout << "SERVER FAILED: ";
			std::cout << e << std::endl;
		}
	}
	
	curl_global_cleanup();
}

void reload_data_fast(int) {
	// Mask the SIGALRM signal
	sigset_t new_set;
	sigemptyset(&new_set);
	sigaddset(&new_set, SIGALRM);
	sigprocmask(SIG_BLOCK, &new_set, NULL);
	
	// Should we do a daily run?
	struct tm newtime;
	time_t ltime;
	ltime = time(&ltime);
	localtime_r(&ltime, &newtime);
	if (((newtime.tm_yday > last_data_loaded.tm_yday) ||
		 (newtime.tm_yday == 0)) &&
		(newtime.tm_hour == program_options->reload_hour()) ) {
			
		if (program_options->verbose() >= 1) {
			std::cout << "Reloading all data" << std::endl;
		}
		// Send a HUP to the parent to force it to spawn a new
		// child.  The child will load data, kill us, and take over.
		kill(parent_pid, SIGHUP);
	} else {
		if (program_options->verbose() >= 1) {
			std::cout << "Reloading fast data" << std::endl;
		}
		Load::reload_online_and_new(data);
		if (program_options->verbose() >= 1) {
			std::cout << "Data loaded" << std::endl;
		}
	}
	
	// Set up a timer again
	sigprocmask(SIG_UNBLOCK, &new_set, NULL);
	struct itimerval tout_val;
	tout_val.it_interval.tv_sec = 0;
	tout_val.it_interval.tv_usec = 0;
	tout_val.it_value.tv_sec = program_options->reload_frequency();
	tout_val.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &tout_val, 0);
}

void reload_data_slow(int) {
	int fork_pid = fork();
	if (fork_pid < 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Fork error" << std::endl;
		}
		exit(1);
	} else if (fork_pid == 0) {
		strncpy(program_name, "vor-child", strlen(program_name));
		child();
	} else {
		child_pids.push_back(fork_pid);
	}
}

void dead_child(int) {
	pid_t pid = wait(NULL);
	child_pids.erase(std::remove(child_pids.begin(), child_pids.end(), pid));
	if (child_pids.empty()) {
		int fork_pid = fork();
		if (fork_pid < 0) {
			if (program_options->verbose() >= 0) {
				std::cout << "Fork error" << std::endl;
			}
			exit(1);
		} else if (fork_pid == 0) {
			strncpy(program_name, "vor-child", strlen(program_name));
			child();
		} else {
			child_pids.push_back(fork_pid);
		}
	}
}

void dump_stats(int) {
	global_stats->save_stats();
	exit(0);
}

void shutdown(int) {
	// Stop trying to respawn children
	signal(SIGCHLD, SIG_IGN);

	// Kill all children
	for (std::vector<pid_t>::const_iterator it = child_pids.begin();
		it != child_pids.end();
		++it) {

		kill(*it, SIGTERM);
	}
	
	sleep(1);
	global_stats->clear_stats();
	exit(0);
}
