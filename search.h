#ifndef _SEARCH_H_
#define _SEARCH_H_

#include "data_structures.h"

class Search {
public:
	Search(const All_data_t &the_data);
	virtual ~Search() {}

	// Perform the search and return results.
	// If doing an ALL search, the search results are made up of matches
	// in user's friends list, ordered randomly, followed by matches in
	// user's friends-of-friends list, ordered randomly, followed by
	// school, location, and all matches, again all ordered randomly.
	std::vector<Id_t> do_search(
		Id_t searcher_userid,
		Id_t searcher_school,
		Id_t searcher_location,
		Params_t params,
		std::vector<Id_t> interests
	) const;
	
private:
	// Return all the users we know about.
	Id_set_t dump_all_users(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		bool full_results) const;
	
	// Perform username substring matches.
	// Return a pair of result sets.  The first is a single result, which
	// may be 0 (no result), indicating an exact match.  The second is a
	// set of regular results based on our lazy substring matches.
	std::pair<Id_t, Id_set_t> search_usernames(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		Name_t username) const;
	
	// Perform firstname substring matches.
	// Return a pair of result sets.  The first is a set of exact realname
	// matches.  The second is a set of inexact realname matches.
	std::pair<Id_set_t, Id_set_t> search_firstnames(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		Name_t name) const;
	
	// Perform firstname substring matches.
	// Return a pair of result sets.  The first is a set of exact realname
	// matches.  The second is a set of inexact realname matches.
	std::pair<Id_set_t, Id_set_t> search_lastnames(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		Name_t name) const;
	
	// Search for users matching ALL of the given interests.
	Id_set_t search_interests(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		const std::vector<Id_t>& interests) const;
	
	// Search for users in the given location.
	Id_set_t search_location(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		const Id_t location) const;
	
	// Search for users in the given school
	Id_set_t search_school(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		const Id_t school) const;
	
	// Search for users with given sexuality
	Id_set_t search_sexuality(
		const std::vector<const Data_chunk_t *>& age_sex_data,
		const unsigned short sexuality) const;
	
	// Search for users with picture(s)
	Id_set_t search_with_picture(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// Search for single users (single, single-and-looking)
	Id_set_t search_single_users(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// Search for users whose birthday it is today
	Id_set_t search_birthdays(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// Search for users who are currently online
	Id_set_t search_online(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// Search for new users only
	Id_set_t search_new_users(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// Search for users active recently
	Id_set_t search_active_recently(
		const std::vector<const Data_chunk_t *>& age_sex_data) const;
	
	// This is used so that we can AND together two sets of results.
	// If allow_copy is true, we will simply copy (actually, swap) from
	// local_results to all_results if all_results is empty.
	void intersect(Id_set_t& all_results, Id_set_t& local_results,
		const bool allow_copy) const;
	
private:
	Search();
	Search(const Search& other);
	Search& operator=(const Search& other);

private:
	const All_data_t& data;
};

#endif
