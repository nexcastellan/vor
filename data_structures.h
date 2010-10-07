#ifndef _DATA_STRUCTURES_H_
#define _DATA_STRUCTURES_H_

#include <boost/shared_ptr.hpp>
#include <map>
#include <set>
#include <sys/time.h>
#include <string>
#include <vector>

#include "lock.h"

typedef std::string Name_t;
typedef unsigned int Id_t;

typedef std::set<Id_t> Id_set_t;
typedef std::map<Id_t, Id_set_t> Id_to_id_set_t;
typedef std::map<Id_t, Name_t> Id_to_name_t;
typedef std::map<Name_t, Id_t> Name_to_id_t;
typedef std::map<std::string, std::string> Params_t;

// Each chunk of data represents all we know about
// users with a given gender and age.
class Data_chunk_t {
public:
	Id_to_name_t usernames;
	Id_to_name_t firstnames;
	Id_to_name_t lastnames;
	// A short list (1000) of random userids, used to speed up browsing.
	Id_set_t shortlist;
	Id_set_t userids;
	// Location id to list of userids
	Id_to_id_set_t locations;
	// School id to list of userids
	Id_to_id_set_t schools;
	// Interest id to list of userids
	Id_to_id_set_t interests;
	// Sexuality sets
	Id_set_t heterosexual;
	Id_set_t homosexual;
	Id_set_t bisexual;
	Id_set_t with_picture; // Only users with picture(s)
	Id_set_t single_users; // Only users single || single_and_looking
	Id_set_t birthdays; // Users whose birthday it is today
	Id_set_t online;
	mutable boost::shared_ptr<RWLock> online_lock;
	Id_set_t new_users;
	mutable boost::shared_ptr<RWLock> new_users_lock;
	Id_set_t active_recently;

	// We do a variant of a suffix tree.  See
	// http://en.wikipedia.org/wiki/Suffix_tree
	// Consider the case, "greg" with userid 1 and
	// "eggy" with userid 2.  We would store
	// the following:
	// username_suffixes['g']['r'] => [1]
	// username_suffixes['r']['e'] => [1, 2]
	// username_suffixes['e']['g'] => [1]
	// username_suffixes['g']['g'] => [2]
	// username_suffixes['g']['y'] => [2]
	Id_set_t username_suffixes[26][26];
	Id_set_t firstname_suffixes[26][26];
	Id_set_t lastname_suffixes[26][26];
	
	Data_chunk_t();
};

// A structure storing data for each age.
typedef std::map<int, Data_chunk_t> Age_to_data_t;

typedef std::map<Id_t, Id_set_t> Friend_list_t;

class All_data_t {
public:
	// Lock everything apart from the online and new users lists
	// (which change frequently)
	mutable boost::shared_ptr<RWLock> lock;
	std::vector<Age_to_data_t> data_chunks; // Male, female
	// Keep track of everyone's username, complete with symbols.
	// We do, however, convert to lower case.
	Name_to_id_t usernames_unprocessed;
	Friend_list_t friends;
	// Store location hierarchy, so that we can look up a value (say, Alberta)
	// and get all of the child locations (e.g. Edmonton, Calgary, St. Albert).
	Id_to_id_set_t location_hierarchy;
	// The last userid that we loaded.  This is used for our regular reload of
	// new users, to pull information about any new userids.
	Id_t last_loaded_userid;

	All_data_t();
	// Calculate the size, in bytes, of the data structure.
	// This is a ballpark estimate and can only ever hope to work on systems
	// where std::string, std::map, and std::set are roughly similar to that
	// of the GNU ISO C++ Library version 4.0.0.  Even there, we ignore some
	// overhead because it should be insignificant compared to the overall
	// data use.
	size_t size_of() const;
};

// Global data
extern All_data_t data;
extern struct tm last_data_loaded;
extern std::vector<pid_t> child_pids;

#endif
