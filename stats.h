#ifndef _STATS_H_
#define _STATS_H_

#include <map>
#include <string>
#include <sys/time.h>

#include "data_structures.h"
#include "lock.h"

// Keep track of vor statistics.
class Stats {
public:
	Stats(const All_data_t& new_data, pid_t parent_pid);
	~Stats();
	
public:
	// Save all of the stats to shared memory so they can
	// be reloaded by another child process.
	void save_stats() const;
	
	// Retrieve stats saved in shared memory by another child.
	void load_stats();
	
	// Drop the shared memory for stats storage.
	void clear_stats() const;
	
	// How many seconds has the server been running?
	unsigned int getRunningTime() const;
	// What's the approximate amount of memory used?
	// This actually returns KB used, at least on Linux.
	size_t getMemoryUse() const;
	// Get the total time spent running all the searches.
	// This is in milliseconds.
	unsigned long getSearchTime() const; 
	// Increase time spent performing searches.
	unsigned long incrSearchTime(const unsigned long taken);
	// Get the total time spent running all the searches, including
	// network time.
	// This is in milliseconds.
	unsigned long getSearchTimeNetwork() const; 
	// Increase time spent performing searches, including
	// network time.
	unsigned long incrSearchTimeNetwork(const unsigned long taken);
	// How many times have we fully reloaded data?
	unsigned int getDataReloadFull() const;
	// Increase the number of times we've fully reloaded data.
	unsigned int incrDataReloadFull();
	// How many times have we reloaded new user, online data?
	unsigned int getDataReloadFast() const;
	// Increase the number of times we've reloaded new user, online data.
	unsigned int incrDataReloadFast();
	// How many searches are currently in flight?
	unsigned int getInFlight() const;
	// Increase searches in flight by one.
	unsigned int incrInFlight();
	// Decrease searches in flight by one.
	unsigned int decrInFlight();
	
	// Get all the statistics on the number of searches executed.
	// See server.cpp for more information.
	std::map<std::string, unsigned int> getSearchReqs() const;
	// Get one statistic on the number of searches executed.
	// See server.cpp for more information.
	unsigned int getSearchReq(const std::string& which) const;
	// Increment the count of search requests.
	// See server.cpp for more information.
	unsigned int incrSearchReq(const std::string& which);

private:
	const All_data_t &data;
	boost::shared_ptr<RWLock> rwlock;
	std::string shared_memory_name; // Used to identify shared memory
	std::string mutex_name; // Used to identify shared mutex name
	std::vector<std::string> keys; // Search query keys
	
private:
	// When did the server start up?
	time_t start_time;
	// Total time spent running all searches, in milliseconds.
	// This is just the time spent performing the search itself.
	unsigned long search_time;
	// Total time spent running all searches, in milliseconds.
	// This includes time spent communicating over the network.
	unsigned long search_time_network;
	// Full data loads
	unsigned int data_reloads_full;
	// New user, online data reloads.
	unsigned int data_reloads_fast;
	// How many searches in flight?
	unsigned int in_flight;
	// How many searches, and of what types, have been run?
	// See server.cpp for more information.
	std::map<std::string, unsigned int> search_reqs;

private:
	Stats(const Stats& other);
	Stats& operator=(const Stats& rhs);
};

// Global statistics
extern boost::shared_ptr<Stats> global_stats;

#endif
