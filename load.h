#ifndef _LOAD_H_
#define _LOAD_H_

#include <boost/shared_ptr.hpp>
#include <deque>
#include <pthread.h>

#include "data_structures.h"
#include "thread.h"
#include "lock.h"

class Load_args_t;

class Load {
public:
	// Load all the user data.
	// Return true if the data load succeeded.
	static bool load_all_data(All_data_t& the_data);
	
	// Reload only the online and new user data.
	// Return true if the data load succeeded.
	static bool reload_online_and_new(All_data_t& the_data);
	
private:
	friend void* load_friends(void *);
	friend void* load_details(void *);
	friend void* load_usernames(void *);
	friend void* load_realnames(void *);
	friend void* load_interests(void *);
	friend void* load_birthdays(void *);
	friend void* load_online_statuses(void *);
	friend void* load_new_users(void *);
	friend void* load_active_recently(void *);
	friend void* load_locations(void *);
	
	// Retrieve overview data.
	// We return min_userid and max_userid retrieved from server.
	void load_overview(Id_t &min_userid, Id_t &max_userid);
	
	// Load most data, for a given range of userids, from the server.
	// See load_rare_data.
	void load_common_data(Id_t min_userid, Id_t max_userid);
	
	// Load data that does not change very often.  We only need to load this
	// data on initial startup and then once per day.
	void load_rare_data();
	
	// Prune the list of running threads, so our virtual memory
	// usage doesn't go sky high.  We'll join on the threads in
	// the list.  Once the thread has completed, we'll remove it
	// from the args_list.
	// We will only prune if we are running at least max_threads,
	// and we'll prune down to min_threads.
	static void prune_threads(std::deque<pthread_t>& threads,
		std::deque<boost::shared_ptr<Load_args_t> >& args_list,
		const unsigned int min_threads, const unsigned int max_threads);
		
	// Load all the user data.
	bool load_all_data();

	// Reload only the online and new user data.
	bool reload_online_and_new();
	
private:
	// A reference to the data we are loading.
	All_data_t& data;
	
	// Keep track of the threads we've spawned.
	std::deque<pthread_t> threads;
	
private:
	Load();
	Load(All_data_t& the_data);
	~Load();
	Load(const Load& other);
	Load& operator=(const Load& rhs);
};

// Convert input string into a set of char-pairs.
// For example, "chris" becomes [[ch],[hr],[ri],[is]]
std::set<std::pair<char, char> >
string_elems(const Name_t& in);

// Load the list of friends so we can quickly find friends_of.
void *load_friends(void *data);

// Load the user detail information.
void *load_details(void *data);

// Load list of usernames.
void *load_usernames(void *data);

// Load list of real names.
void *load_realnames(void *data);

// Load list of user interests.
void *load_interests(void *data);

// Load the list of users who have a birthday today.
void *load_birthdays(void *data);

// Load the list of users currently online.
void *load_online_statuses(void *data);

// Load the list of new users.
void *load_new_users(void *data);

// Load the list of users who have been active recently.
void *load_active_recently(void *data);

// Load the locations hierarchy.
void *load_locations(void *data);

#endif
