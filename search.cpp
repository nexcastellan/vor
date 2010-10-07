#include "search.h"

#include <algorithm>
#include <assert.h>
#include <iostream>

#include "program_options.h"
#include "utility.h"

Search::Search(const All_data_t &the_data) :
	data(the_data) {
		
	assert(&the_data != NULL);	
}

std::vector<Id_t>
Search::do_search(
	Id_t searcher_userid,
	Id_t searcher_school,
	Id_t searcher_location,
	Params_t params,
	std::vector<Id_t> interests
) const {
	Id_set_t all_results;
	Id_set_t local_results;
	bool allow_copy = true;
	char *end_ptr;
	std::vector<Id_t> retval; // Appropriately sorted
	// We want to pull the following out to the front of the results.
	Id_t exact_match_username;
	Id_set_t exact_matches_realname;

 	// Pointer to data for sex and ages of interest.  Most of our searching
	// is limited to a specific sex and age range, and so we store pointers
	// to the data there.
	std::vector<const Data_chunk_t *> age_sex_data;
	unsigned int min_age = ::strtol(params["min_age"].c_str(), &end_ptr, 10);
	unsigned int max_age = ::strtol(params["max_age"].c_str(), &end_ptr, 10);
	if (min_age > 80) min_age = 13;
	if (min_age < 13) min_age = 13;
	if (max_age > 80) max_age = 80;
	if (max_age < 13) max_age = 80;
	for (unsigned int age = min_age; age <= max_age; ++age) {
		if ((params["sex"] == "f") || (params["sex"] == "F") ||
		 	(params["sex"] == "")) {
			
			const Age_to_data_t& gender(this->data.data_chunks[1]);
			assert(&gender != NULL);
			Age_to_data_t::const_iterator by_age = gender.find(age);
			if (by_age != gender.end()) {
				age_sex_data.push_back(&(by_age->second));
			}
		}
		if ((params["sex"] == "m") || (params["sex"] == "M") ||
		 	(params["sex"] == "")) {
			
			const Age_to_data_t& gender(this->data.data_chunks[0]);
			assert(&gender != NULL);
			Age_to_data_t::const_iterator by_age = gender.find(age);
			if (by_age != gender.end()) {
				age_sex_data.push_back(&(by_age->second));
			}
		}
	}
	
	// Do name searches
	Name_t name = params["name"];
	if (name.length() > 0) {
		std::pair<Id_t, Id_set_t> local_results_username;
		local_results_username = search_usernames(age_sex_data, name);
		
		size_t separator = name.find(" ");
		std::string firstname;
		std::string lastname;
		if (separator == std::string::npos) {
			firstname = name;
			lastname = name;
		} else {
			firstname = name.substr(0, separator);
			lastname = name.substr(separator + 1);
		}

		std::pair<Id_set_t, Id_set_t> local_results_firstname;
		local_results_firstname = search_firstnames(age_sex_data, firstname);
		std::pair<Id_set_t, Id_set_t> local_results_lastname;
		local_results_lastname = search_lastnames(age_sex_data, lastname);
		
		std::pair<Id_set_t, Id_set_t> local_results_name;
		if (separator == std::string::npos) {
			// Only passed a first or a last name so valid results are all
			// those that appear in either result set
			local_results_name = local_results_firstname;
			for (Id_set_t::const_iterator it = local_results_lastname.first.begin();
				it != local_results_lastname.first.end();
				++it) {
				
				local_results_name.first.insert(*it);
			}
			for (Id_set_t::const_iterator it = local_results_lastname.second.begin();
				it != local_results_lastname.second.end();
				++it) {
				
				local_results_name.second.insert(*it);
			}
		} else {
			// Passed both first and last name so valid results are all
			// those that appear in both result sets
			std::set_intersection(
				local_results_firstname.first.begin(),
				local_results_firstname.first.end(),
				local_results_lastname.first.begin(),
				local_results_lastname.first.end(),
				std::inserter(local_results_name.first,
				local_results_name.first.end()));
			std::set_intersection(
				local_results_firstname.second.begin(),
				local_results_firstname.second.end(),
				local_results_lastname.second.begin(),
				local_results_lastname.second.end(),
				std::inserter(local_results_name.second,
				local_results_name.second.end()));
		}

		// Okay, now we have a list of matching usernames and a list of
		// matching realnames.  We accept any results in either of
		// these lists.
		local_results = local_results_username.second;
		for (Id_set_t::const_iterator it = local_results_name.second.begin();
			it != local_results_name.second.end();
			++it) {
			
			local_results.insert(*it);
		}

		intersect(all_results, local_results, allow_copy);

		exact_match_username = local_results_username.first;
		exact_matches_realname.swap(local_results_name.first);
		// Remove the username match if one exists
		Id_set_t::iterator found;
		found = exact_matches_realname.find(exact_match_username);
		if (found != exact_matches_realname.end()) {
			exact_matches_realname.erase(found);
		}

		allow_copy = false;
	}
	
	
	// Great, let's search on interests
	if (!interests.empty()) {
		local_results = search_interests(age_sex_data, interests);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}

	// Location
	Id_t location = ::strtol(params["location"].c_str(), &end_ptr, 10);
	if (location != 0) {
		// Handle all decendent locations as well
		Id_to_id_set_t::const_iterator found;
		ReadLock lock(this->data.lock);
		found = this->data.location_hierarchy.find(location);
		Id_set_t location_results;
		if (found != this->data.location_hierarchy.end()) {
			for (Id_set_t::const_iterator it = found->second.begin();
				it != found->second.end();
				++it) {
				
				local_results = search_location(age_sex_data, *it);
				for (Id_set_t::const_iterator itLocal = local_results.begin();
					itLocal != local_results.end();
					++itLocal) {
						
					location_results.insert(*itLocal);
				}
			}
		} else {
			location_results = search_location(age_sex_data, location);
		}
		intersect(all_results, location_results, allow_copy);
		allow_copy = false;
	}
	
	// School
	Id_t school = ::strtol(params["school"].c_str(), &end_ptr, 10);
	if (school != 0) {
		local_results = search_school(age_sex_data, school);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}

	// Sexuality
	unsigned short sexuality = ::strtol(params["sexuality"].c_str(), &end_ptr, 10);
	if ((sexuality >= 1) && (sexuality <= 3)) {
		local_results = search_sexuality(age_sex_data, sexuality);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// With picture?
	if (params["with_picture"] == "true") {
		local_results = search_with_picture(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// Single?
	if (params["single"] == "true") {
		local_results = search_single_users(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// Birthday?
	if (params["birthday"] == "true") {
		local_results = search_birthdays(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// Online?
	if (params["online"] == "true") {
		local_results = search_online(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// New users?
	if (params["new_users"] == "true") {
		local_results = search_new_users(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// Active recently?
	if (params["active_recently"] == "true") {
		local_results = search_active_recently(age_sex_data);
		intersect(all_results, local_results, allow_copy);
		allow_copy = false;
	}
	
	// Should we be reordering the result set?
	bool reorder = false;
	if (params["might_know"] == "true") {
		reorder = true;
	}

	// Should we be stripping out friends from the result set?
	bool no_friends = false;
	if (params["no_friends"] == "true") {
		no_friends = true;
	}

	// Did we actually perform a search?  If not, [sigh] grab all
	// the results
	if (allow_copy) {
		local_results = dump_all_users(age_sex_data, reorder);
		intersect(all_results, local_results, allow_copy);
		allow_copy = true;
	}
	
	
	// Now, pull out our exact matches to the front, if they are
	// still there.  If not, they failed matching other criteria and so
	// we don't want them in our result set.
	if (exact_match_username > 0) {
		Id_set_t::iterator found;
		found = all_results.find(exact_match_username);
		if (found != all_results.end()) {
			all_results.erase(found);
			retval.push_back(exact_match_username);
		}
	}
	for (Id_set_t::const_iterator it = exact_matches_realname.begin();
		it != exact_matches_realname.end();
		++it) {
	
		Id_set_t::iterator found = all_results.find(*it);
		std::vector<Id_t> local_results;
		if (found != all_results.end()) {
			all_results.erase(found);
			local_results.push_back(*it);
		}
		std::random_shuffle(local_results.begin(), local_results.end());
		std::copy(local_results.begin(), local_results.end(),
			std::inserter(retval, retval.end()));
	}

	
	// Extract the subset of friends, placing them first
	// TODO: Should replace with set union
	Id_set_t friends;
	std::vector<Id_t> only_friends;
	{
		Friend_list_t::const_iterator itFriends;
		itFriends = this->data.friends.find(searcher_userid);
		if (itFriends != this->data.friends.end()) {
			friends = itFriends->second;
		}
	}
	
	if (no_friends) {
		Id_set_t::iterator found;
		// Remove those from the all_results list.
		for (Id_set_t::const_iterator itFriends = friends.begin();
			itFriends != friends.end();
			++itFriends) {
			
			found = all_results.find(*itFriends);
			if (found != all_results.end()) {
				all_results.erase(found);
			}
		}
		
		// Also, remove the searcher's userid
		if (searcher_userid > 0) {
			found = all_results.find(searcher_userid);
			if (found != all_results.end()) {
				all_results.erase(found);
			}
		}
	}

#if 0
	for (Id_set_t::iterator it = all_results.begin();
		it != all_results.end(); 
		/* Increment below */ ) {

		Id_set_t::const_iterator found;
		found = friends.find(*it);
		if (found != friends.end()) {
			// In list of friends
			only_friends.push_back(*it);
			all_results.erase(it++);
		} else {
			++it;
		}
	}
	std::random_shuffle(only_friends.begin(), only_friends.end());
#endif
	
	// Now, friends-of-friends
	std::vector<Id_t> only_f_of_f;
	if (reorder) {
		Id_set_t friends_of_friends(friends);
		// For each friend, bring in their friends as well.
		for (Id_set_t::const_iterator itFriends = friends.begin();
			itFriends != friends.end();
			++itFriends) {

			Friend_list_t::const_iterator itFofF;
			itFofF = this->data.friends.find(*itFriends);
			if (itFofF != this->data.friends.end()) {
				// Found a batch of friends of friends, copy them in.
				std::copy(itFofF->second.begin(), itFofF->second.end(),
					std::inserter(friends_of_friends,
					friends_of_friends.end()));
			}
		}
		for (Id_set_t::iterator it = all_results.begin();
			it != all_results.end(); 
			/* Increment below */ ) {

			Id_set_t::const_iterator found;
			found = friends_of_friends.find(*it);
			if (found != friends_of_friends.end()) {
				// In list of friends
				only_f_of_f.push_back(*it);
				all_results.erase(it++);
			} else {
				++it;
			}
		}
		std::random_shuffle(only_f_of_f.begin(), only_f_of_f.end());
	}
	
	// Now, by school
	std::vector<Id_t> only_school;
	if (reorder) {
		Id_set_t in_school; // users in the searcher's school
		{
			if (searcher_school != 0) {
				in_school = search_school(age_sex_data, searcher_school);
			}
		}
		// Find only those in the searcher's school
		std::set_intersection(all_results.begin(), all_results.end(),
			in_school.begin(), in_school.end(),
			std::inserter(only_school, only_school.end()));
		// And remove those from the all_results list.
		for (std::vector<Id_t>::const_iterator itSchool = only_school.begin();
			itSchool != only_school.end();
			++itSchool) {
			
			Id_set_t::iterator found = all_results.find(*itSchool);
			if (found != all_results.end()) {
				all_results.erase(found);
			}
		}
		std::random_shuffle(only_school.begin(), only_school.end());
	}

	// Now, by location
	std::vector<Id_t> only_location;
	if (reorder) {
		Id_set_t in_location; // users in the searcher's location
		{
			if (searcher_location != 0) {
				// Handle all decendent locations as well
				Id_to_id_set_t::const_iterator found;
				ReadLock lock(this->data.lock);
				found = this->data.location_hierarchy.find(searcher_location);
				if (found != this->data.location_hierarchy.end()) {
					for (Id_set_t::const_iterator it = found->second.begin();
						it != found->second.end();
						++it) {

						local_results = search_location(age_sex_data, *it);
						for (Id_set_t::const_iterator itLocal = local_results.begin();
							itLocal != local_results.end();
							++itLocal) {

							in_location.insert(*itLocal);
						}
					}
				} else {
					in_location = search_location(age_sex_data, location);
				}
			}
		}
		// Find only those in the searcher's location
		std::set_intersection(all_results.begin(), all_results.end(),
			in_location.begin(), in_location.end(),
			std::inserter(only_location, only_location.end()));
		// And remove those from the all_results list.
		for (std::vector<Id_t>::const_iterator itLocation = only_location.begin();
			itLocation != only_location.end();
			++itLocation) {
			
			Id_set_t::iterator found = all_results.find(*itLocation);
			if (found != all_results.end()) {
				all_results.erase(found);
			}
		}
		std::random_shuffle(only_location.begin(), only_location.end());
	}

	// And the rest
	std::vector<Id_t> remaining_results;
	std::copy(all_results.begin(), all_results.end(),
		std::inserter(remaining_results, remaining_results.end()));
	std::random_shuffle(remaining_results.begin(), remaining_results.end());

	// Join the rest of our results together
	std::copy(only_friends.begin(), only_friends.end(),
		std::inserter(retval, retval.end()));
	std::copy(only_f_of_f.begin(), only_f_of_f.end(),
		std::inserter(retval, retval.end()));
	std::copy(only_school.begin(), only_school.end(),
		std::inserter(retval, retval.end()));
	std::copy(only_location.begin(), only_location.end(),
		std::inserter(retval, retval.end()));
	std::copy(remaining_results.begin(), remaining_results.end(),
		std::inserter(retval, retval.end()));
	
	return retval;
}

Id_set_t
Search::dump_all_users(
	const std::vector<const Data_chunk_t *>& age_sex_data,
	bool dump_all_users) const {
		
	assert(&age_sex_data != NULL);
		
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		if (dump_all_users) {
			// Pull from our full list of userids in this chunk.
			for (Id_set_t::const_iterator itFull = (*it)->userids.begin();
				itFull != (*it)->userids.end();
				++itFull) {
			
				found_list.insert(*itFull);
			}
		} else {
			// Pull from our short list, which contains enough userids but
			// hopefully much less than the full list.
			for (Id_set_t::const_iterator itShort = (*it)->shortlist.begin();
				itShort != (*it)->shortlist.end();
				++itShort) {
			
				found_list.insert(*itShort);
			}
		}
	}
	return found_list;
}


std::pair<Id_t, Id_set_t>
Search::search_usernames(const std::vector<const Data_chunk_t *>& age_sex_data,
	Name_t username) const {
		
	assert(&age_sex_data != NULL);
	Name_t username_unprocessed = Utility::downcase(username);
	username = Utility::strip_string(username);
	// The set of matches for all ages and genders.
	Id_set_t found_list;
	
	std::vector<const Data_chunk_t *>::const_iterator it;
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		if (username.length() == 1) {
			// Special case, length == 1 is hard to search for
			const Data_chunk_t& data_chunk(**it);
			assert(&data_chunk != NULL);
			ReadLock lock(this->data.lock);
			for (Id_to_name_t::const_iterator itData = data_chunk.usernames.begin();
				itData != data_chunk.usernames.end();
				++itData) {
				
				size_t substring_found = itData->second.find(username);
				if (substring_found != std::string::npos) {
					found_list.insert(itData->first);
				}
			}
		} else if (username.length() > 1) {
			Id_set_t chunk_found_list; // The set of matches for this data chunk
			const Data_chunk_t& data_chunk(**it);
			assert(&data_chunk != NULL);
			ReadLock lock(this->data.lock);
		
			// For each 'element' in the username
			// For 'greg', this would be "gr", "re", and "eg"
			for (size_t i = 0; (i + 1) < username.length(); ++i) {
				const char& a(username[i]);
				const char& b(username[i+1]);
				assert(&a != NULL);
				assert(&b != NULL);
				assert(a >= 'a');
				assert(a <= 'z');
				assert(b >= 'a');
				assert(b <= 'z');
				if (i == 0) {
					// Insert first set of matches
					chunk_found_list =
					 	data_chunk.username_suffixes[a-'a'][b-'a'];
				} else {
					// Intersect subsequent matches
					const Id_set_t& new_found_list(
						data_chunk.username_suffixes[a-'a'][b-'a']);
					assert(&new_found_list != NULL);
					Id_set_t intersection;
					std::set_intersection(
						chunk_found_list.begin(), chunk_found_list.end(),
						new_found_list.begin(), new_found_list.end(),
						std::inserter(intersection, intersection.end()));
					chunk_found_list.swap(intersection);
				} // if (i == 0)
			} // for (size_t i = 0...)
		
			// Now, we have a list of matches from searching the suffixes.
			// However, they may not be real matches.  Searching "greg",
			// for example, would match "regr".  That's fine, we have
			// seriously narrowed down our set of matches.  So, let's
			// quickly prune these by doing full substring searches on
			// this narrowed set.  We'll throw out anything that does
			// not match.
			for (Id_set_t::iterator itNarrow = chunk_found_list.begin();
				itNarrow != chunk_found_list.end(); /* Nothing */ ) {
				
				size_t substring_found = std::string::npos;
				Id_to_name_t::const_iterator username_found;
				username_found = data_chunk.usernames.find(*itNarrow);
				if (username_found != data_chunk.usernames.end()) {
					substring_found = username_found->second.find(username);
				}

				if (substring_found == std::string::npos) {
					chunk_found_list.erase(itNarrow++);
				} else {
					// Match is good
					++itNarrow;
				}
			}

			// Copy the new results into the overall found_list.  It's
			// a set, so duplicates are automatically eliminated.
			std::copy(chunk_found_list.begin(), chunk_found_list.end(),
				std::inserter(found_list, found_list.end()));
		} // for (it = age_sex_data.begin()...)
	}
	
	// Okay, we have a list of all usernames.  Do we have an exact match?
	// If so, we'll pull it to the front.
	ReadLock lock(this->data.lock);

	Name_to_id_t::const_iterator found;
	found = this->data.usernames_unprocessed.find(username_unprocessed);
	Id_t exact_userid = 0;
	if (found != this->data.usernames_unprocessed.end()) {
		exact_userid = found->second;
	}

	return std::make_pair(exact_userid, found_list);
}

std::pair<Id_set_t, Id_set_t>
Search::search_firstnames(const std::vector<const Data_chunk_t *>& age_sex_data,
	Name_t name) const {

	assert(&age_sex_data != NULL);
	name = Utility::strip_string(name);
	// The set of exact matches and inexact matches.
	Id_set_t exact_matches, found_list;
	std::vector<const Data_chunk_t *>::const_iterator it;
	
	// Search firstnames
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		Id_set_t chunk_found_list; // The set of matches for this data chunk
		const Data_chunk_t& data_chunk(**it);
		assert(&data_chunk != NULL);
		ReadLock lock(this->data.lock);
		
		if (name.length() == 1) {
			// Special case, length == 1 is hard to search for
			ReadLock lock(this->data.lock);
			for (Id_to_name_t::const_iterator it = data_chunk.firstnames.begin();
				it != data_chunk.firstnames.end();
				++it) {
				
				size_t substring_found = it->second.find(name);
				if (substring_found != std::string::npos) {
					found_list.insert(it->first);
				}
			}
		} else if (name.length() > 1) {
			// For each 'element' in the real name
			// For 'greg', this would be "gr", "re", and "eg"
			for (size_t i = 0; (i + 1) < name.length(); ++i) {
				const char& a(name[i]);
				const char& b(name[i+1]);
				assert(&a != NULL);
				assert(&b != NULL);
				assert(a >= 'a');
				assert(a <= 'z');
				assert(b >= 'a');
				assert(b <= 'z');
				if (i == 0) {
					// Insert first set of matches
					chunk_found_list =
					 	data_chunk.firstname_suffixes[a-'a'][b-'a'];
				} else {
					// Intersect subsequent matches
					const Id_set_t& new_found_list(
						data_chunk.firstname_suffixes[a-'a'][b-'a']);
					assert(&new_found_list != NULL);
					Id_set_t intersection;
					std::set_intersection(
						chunk_found_list.begin(), chunk_found_list.end(),
						new_found_list.begin(), new_found_list.end(),
						std::inserter(intersection, intersection.end()));
					chunk_found_list.swap(intersection);
				} // if (i == 0)
			} // for (size_t i = 0...)
		
			// Now, we have a list of matches from searching the suffixes.
			// However, they may not be real matches.  Searching "greg",
			// for example, would match "regr".  That's fine, we have
			// seriously narrowed down our set of matches.  So, let's
			// quickly prune these by doing full substring searches on
			// this narrowed set.  We'll throw out anything that does
			// not match.
			for (Id_set_t::iterator itNarrow = chunk_found_list.begin();
				itNarrow != chunk_found_list.end(); /* Nothing */ ) {
				
				size_t substring_found = std::string::npos;
				Id_to_name_t::const_iterator name_found;
				name_found = data_chunk.firstnames.find(*itNarrow);
				if (name_found != data_chunk.firstnames.end()) {
					substring_found = name_found->second.find(name);
				}

				if (substring_found == std::string::npos) {
					chunk_found_list.erase(itNarrow++);
				} else {
					// Match is good, is it an exact match?
					if (name_found->second == name) {
						exact_matches.insert(*itNarrow);
					}
					
					++itNarrow;
				}
			}
			
			// Copy the new results into the overall found_list.  It's
			// a set, so duplicates are automatically eliminated.
			std::copy(chunk_found_list.begin(), chunk_found_list.end(),
				std::inserter(found_list,
				found_list.end()));
		} // for (it = age_sex_data.begin()...)
	}
	
	return std::make_pair(exact_matches, found_list);
}

std::pair<Id_set_t, Id_set_t>
Search::search_lastnames(const std::vector<const Data_chunk_t *>& age_sex_data,
	Name_t name) const {
		
	assert(&age_sex_data != NULL);
	name = Utility::strip_string(name);
	// The set of exact matches and inexact matches.
	Id_set_t exact_matches, found_list;
	std::vector<const Data_chunk_t *>::const_iterator it;
	
	// Search lastnames
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		Id_set_t chunk_found_list; // The set of matches for this data chunk
		const Data_chunk_t& data_chunk(**it);
		assert(&data_chunk != NULL);
		ReadLock lock(this->data.lock);
		
		if (name.length() == 1) {
			// Special case, length == 1 is hard to search for
			ReadLock lock(this->data.lock);
			for (Id_to_name_t::const_iterator it = data_chunk.lastnames.begin();
				it != data_chunk.lastnames.end();
				++it) {
				
				size_t substring_found = it->second.find(name);
				if (substring_found != std::string::npos) {
					found_list.insert(it->first);
				}
			}
		} else if (name.length() > 1) {
			// For each 'element' in the real name
			// For 'greg', this would be "gr", "re", and "eg"
			for (size_t i = 0; (i + 1) < name.length(); ++i) {
				const char& a(name[i]);
				const char& b(name[i+1]);
				assert(&a != NULL);
				assert(&b != NULL);
				assert(a >= 'a');
				assert(a <= 'z');
				assert(b >= 'a');
				assert(b <= 'z');
				if (i == 0) {
					// Insert first set of matches
					chunk_found_list =
					 	data_chunk.lastname_suffixes[a-'a'][b-'a'];
				} else {
					// Intersect subsequent matches
					const Id_set_t& new_found_list(
						data_chunk.lastname_suffixes[a-'a'][b-'a']);
					assert(&new_found_list != NULL);
					Id_set_t intersection;
					std::set_intersection(
						chunk_found_list.begin(), chunk_found_list.end(),
						new_found_list.begin(), new_found_list.end(),
						std::inserter(intersection, intersection.end()));
					chunk_found_list.swap(intersection);
				} // if (i == 0)
			} // for (size_t i = 0...)
		
			// Now, we have a list of matches from searching the suffixes.
			// However, they may not be real matches.  Searching "greg",
			// for example, would match "regr".  That's fine, we have
			// seriously narrowed down our set of matches.  So, let's
			// quickly prune these by doing full substring searches on
			// this narrowed set.  We'll throw out anything that does
			// not match.
			for (Id_set_t::iterator itNarrow = chunk_found_list.begin();
				itNarrow != chunk_found_list.end(); /* Nothing */ ) {
				
				size_t substring_found = std::string::npos;
				Id_to_name_t::const_iterator name_found;
				name_found = data_chunk.lastnames.find(*itNarrow);
				if (name_found != data_chunk.lastnames.end()) {
					substring_found = name_found->second.find(name);
				}

				if (substring_found == std::string::npos) {
					chunk_found_list.erase(itNarrow++);
				} else {
					// Match is good, is it an exact match?
					if (name_found->second == name) {
						exact_matches.insert(*itNarrow);
					}
					
					++itNarrow;
				}
			}
			
			// Copy the new results into the overall found_list.  It's
			// a set, so duplicates are automatically eliminated.
			std::copy(chunk_found_list.begin(), chunk_found_list.end(),
				std::inserter(found_list,
				found_list.end()));
		} // for (it = age_sex_data.begin()...)
	}
	
	return std::make_pair(exact_matches, found_list);
}

Id_set_t
Search::search_interests(const std::vector<const Data_chunk_t *>& age_sex_data,
	const std::vector<Id_t>& interests) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		// For each interest that we care about
		Id_set_t all_interests_found_list;
		std::vector<Id_t>::const_iterator itInterests;
		for (itInterests = interests.begin();
			itInterests != interests.end();
			++itInterests) {
				
			Id_to_id_set_t::const_iterator itFound;
			itFound = (*it)->interests.find(*itInterests);
			if (itFound != (*it)->interests.end()) {
				// Found a set of userids for the given interest.
				Id_set_t local_results = itFound->second;
				intersect(all_interests_found_list, local_results, true);
			} else {
				// No users with this interest
				all_interests_found_list.clear();
			}
			if (all_interests_found_list.empty())
				break;
		}
		for (Id_set_t::const_iterator itLocal = all_interests_found_list.begin();
			itLocal != all_interests_found_list.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	
	return found_list;
}

Id_set_t
Search::search_location(const std::vector<const Data_chunk_t *>& age_sex_data,
	const Id_t location) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		Id_to_id_set_t::const_iterator itFound;
		itFound = (*it)->locations.find(location);
		if (itFound != (*it)->locations.end()) {
			// Found a set of userids for the given location
			for (Id_set_t::const_iterator itLocal = itFound->second.begin();
				itLocal != itFound->second.end();
				++itLocal) {
					
				found_list.insert(*itLocal);
			}
		}
	}
	return found_list;
}

Id_set_t
Search::search_school(const std::vector<const Data_chunk_t *>& age_sex_data,
	const Id_t school) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		Id_to_id_set_t::const_iterator itFound;
		itFound = (*it)->schools.find(school);
		if (itFound != (*it)->schools.end()) {
			// Found a set of userids for the given location
			for (Id_set_t::const_iterator itLocal = itFound->second.begin();
				itLocal != itFound->second.end();
				++itLocal) {
					
				found_list.insert(*itLocal);
			}
		}
	}
	return found_list;
}

Id_set_t
Search::search_sexuality(const std::vector<const Data_chunk_t *>& age_sex_data,
	const unsigned short sexuality) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		if (sexuality == 1) {
			for (Id_set_t::const_iterator itLocal = (*it)->heterosexual.begin();
				itLocal != (*it)->heterosexual.end();
				++itLocal) {
					
				found_list.insert(*itLocal);
			}
		} else if (sexuality == 2) {
			for (Id_set_t::const_iterator itLocal = (*it)->homosexual.begin();
				itLocal != (*it)->homosexual.end();
				++itLocal) {
					
				found_list.insert(*itLocal);
			}
		} else if (sexuality == 3) {
			for (Id_set_t::const_iterator itLocal = (*it)->bisexual.begin();
				itLocal != (*it)->bisexual.end();
				++itLocal) {
					
				found_list.insert(*itLocal);
			}
		}
	}
	return found_list;
}

Id_set_t
Search::search_with_picture(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		for (Id_set_t::const_iterator itLocal = (*it)->with_picture.begin();
			itLocal != (*it)->with_picture.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	return found_list;
}

Id_set_t
Search::search_single_users(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		for (Id_set_t::const_iterator itLocal = (*it)->single_users.begin();
			itLocal != (*it)->single_users.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	return found_list;
}

Id_set_t
Search::search_birthdays(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		for (Id_set_t::const_iterator itLocal = (*it)->birthdays.begin();
			itLocal != (*it)->birthdays.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	return found_list;
}

Id_set_t
Search::search_online(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		ReadLock lock_online((*it)->online_lock);
		for (Id_set_t::const_iterator itLocal = (*it)->online.begin();
			itLocal != (*it)->online.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	
	return found_list;
}

Id_set_t
Search::search_new_users(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		ReadLock lock_new_users((*it)->new_users_lock);
		for (Id_set_t::const_iterator itLocal = (*it)->new_users.begin();
			itLocal != (*it)->new_users.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	return found_list;
	
}

Id_set_t
Search::search_active_recently(
	const std::vector<const Data_chunk_t *>& age_sex_data) const {

	assert(&age_sex_data != NULL);
	// The set of matches
	Id_set_t found_list;
	
	// We already know the gender and age range we are interested in:
	std::vector<const Data_chunk_t *>::const_iterator it;
	ReadLock lock(this->data.lock);
	for (it = age_sex_data.begin(); it != age_sex_data.end(); ++it) {
		for (Id_set_t::const_iterator itLocal = (*it)->active_recently.begin();
			itLocal != (*it)->active_recently.end();
			++itLocal) {
				
			found_list.insert(*itLocal);
		}
	}
	return found_list;
}


void
Search::intersect(Id_set_t& all_results, Id_set_t& local_results,
	const bool allow_copy) const {

	assert(&all_results != NULL);
	assert(&local_results != NULL);	
	if (program_options->verbose() >= 3) {
		std::cout << "Before intersect, " << all_results.size() << ":";
		std::cout << local_results.size() << std::endl;
	}
	if (allow_copy && all_results.empty()) {
		all_results.swap(local_results);
	} else {
		Id_set_t intersection;
		std::set_intersection(
			all_results.begin(), all_results.end(),
			local_results.begin(), local_results.end(),
			std::inserter(intersection, intersection.end()));
		all_results.swap(intersection);
	}
	if (program_options->verbose() >= 3) {
		std::cout << "After intersect, " << all_results.size() << ":";
		std::cout << local_results.size() << std::endl;
	}
}
