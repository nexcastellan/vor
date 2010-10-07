#include "load.h"

#include <boost/tokenizer.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>

#include "http_client.h"
#include "program_options.h"
#include "stats.h"
#include "thread.h"
#include "utility.h"

// A structure passed to the load functions, representing arguments.
class Load_args_t {
public:
	All_data_t *data;
	Id_t start_userid;
	Id_t max_userid;
	unsigned int since;

	// Initialising constructor	
	Load_args_t(
		All_data_t *new_data,
		Id_t new_start_userid = 0,
		Id_t new_max_userid = 0,
		unsigned int new_since = 0
	);
};

Load::Load(All_data_t& the_data) :
	data(the_data) {
}

Load::~Load() {}

bool
Load::load_all_data(All_data_t& the_data) {
	Load load(the_data);
	return load.load_all_data();
}

bool
Load::reload_online_and_new(All_data_t& the_data) {
	Load load(the_data);
	return load.reload_online_and_new();
}

bool
Load::load_all_data() {
	try {
		// Get overview data
		Id_t min_userid, max_userid;
		load_overview(min_userid, max_userid);
	
		if ((min_userid == 0) && (max_userid == 0)) {
			if (program_options->verbose() >= 0) {
				std::cout << "Failed to load data" << std::endl;
			}
			throw "Failed to load overview data";
		}
	
		load_common_data(min_userid, max_userid);
		load_rare_data();
		WriteLock lock(this->data.lock);
		this->data.last_loaded_userid = max_userid;
		return true;
	} catch (...) {
		if (program_options->verbose() >= 0) {
			std::cout << "Failed to load data" << std::endl;
		}
	}
	return false;
}

void
Load::load_common_data(Id_t min_userid, Id_t max_userid) {
	// Load friend data
	std::deque<boost::shared_ptr<Load_args_t> > args_list;
	for (Id_t i = min_userid; i <= max_userid; i += 1000) {
		boost::shared_ptr<Load_args_t> args(new Load_args_t(
			&this->data,
			i, max_userid
		));
		args_list.push_back(args);
		this->threads.push_back(
			Thread::create(load_friends,
				static_cast<void *>(args_list.back().get()))
		);
		prune_threads(this->threads, args_list, program_options->min_threads(), program_options->max_threads());
	}
	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for friend data to finish loading" << std::endl;
	}

	// Wait for friend data to be loaded
	prune_threads(this->threads, args_list, 0, 0);
	assert(this->threads.empty());
	assert(args_list.empty());
	if (program_options->verbose() >= 2) {
		std::cout << "Friend data loading finished" << std::endl;
	}


	// Load detail data
	for (Id_t i = min_userid; i <= max_userid; i += 1000) {
		boost::shared_ptr<Load_args_t> args(new Load_args_t(
			&this->data,
			i, max_userid
		));
		args_list.push_back(args);
		this->threads.push_back(
			Thread::create(load_details,
				static_cast<void *>(args_list.back().get()))
		);
		prune_threads(this->threads, args_list, program_options->min_threads(), program_options->max_threads());
	}
	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for detail data to finish loading" << std::endl;
	}

	// Wait for detail data to be loaded
	prune_threads(this->threads, args_list, 0, 0);
	assert(this->threads.empty());
	assert(args_list.empty());
	if (program_options->verbose() >= 2) {
		std::cout << "Detail data loading finished" << std::endl;
	}


	// Load username data
	for (Id_t i = min_userid; i <= max_userid; i += 10000) {
		boost::shared_ptr<Load_args_t> args(new Load_args_t(
			&this->data,
			i, max_userid
		));
		args_list.push_back(args);
		this->threads.push_back(
			Thread::create(load_usernames,
				static_cast<void *>(args_list.back().get()))
		);
		prune_threads(this->threads, args_list, program_options->min_threads(), program_options->max_threads());
	}
	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for username data to finish loading" << std::endl;
	}

	// Wait for username data to be loaded
	prune_threads(this->threads, args_list, 0, 0);
	assert(this->threads.empty());
	assert(args_list.empty());
	if (program_options->verbose() >= 2) {
		std::cout << "Username data loading finished" << std::endl;
	}


	// Load realname data
	for (Id_t i = min_userid; i <= max_userid; i += 10000) {
		boost::shared_ptr<Load_args_t> args(new Load_args_t(
			&this->data,
			i, max_userid
		));
		args_list.push_back(args);
		this->threads.push_back(
			Thread::create(load_realnames,
				static_cast<void *>(args_list.back().get()))
		);
		prune_threads(this->threads, args_list, program_options->min_threads(), program_options->max_threads());
	}
	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for realname data to finish loading" << std::endl;
	}

	// Wait for realname data to be loaded
	prune_threads(this->threads, args_list, 0, 0);
	assert(this->threads.empty());
	assert(args_list.empty());
	if (program_options->verbose() >= 2) {
		std::cout << "Realname data loading finished" << std::endl;
	}


	// Load interests data
	for (Id_t i = min_userid; i <= max_userid; i += 1000) {
		boost::shared_ptr<Load_args_t> args(new Load_args_t(
			&this->data,
			i, max_userid
		));
		args_list.push_back(args);
		this->threads.push_back(
			Thread::create(load_interests,
				static_cast<void *>(args_list.back().get()))
		);
		prune_threads(this->threads, args_list, program_options->min_threads(), program_options->max_threads());
	}
	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for interests data to finish loading" << std::endl;
	}

	// Wait for interests data to be loaded
	prune_threads(this->threads, args_list, 0, 0);
	assert(this->threads.empty());
	assert(args_list.empty());
	if (program_options->verbose() >= 2) {
		std::cout << "Interests data loading finished" << std::endl;
	}


	// Because we join on all threads before dropping out of this scope,
	// we are sure it is safe to pass this.  Also, when the threads change
	// any of the data, they do so using RWLocks.
	Load_args_t args(&this->data);

	this->threads.push_back(
		Thread::create(load_online_statuses, static_cast<void *>(&args))
	);

	this->threads.push_back(
		Thread::create(load_new_users, static_cast<void *>(&args))
	);

	this->threads.push_back(
		Thread::create(load_active_recently, static_cast<void *>(&args))
	);

	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for load threads to complete" << std::endl;
	}
	// Wait for all threads to complete.
	for (std::deque<pthread_t>::const_iterator itJoin = this->threads.begin();
		itJoin != this->threads.end();
		++itJoin) {

		pthread_join(*itJoin, NULL);
	}
	this->threads.clear();
	
	// Speed up browses by keeping a denormalised set of age data so we can
	// quickly serve unrestricted browses
	WriteLock lock(data.lock);
	std::vector<Age_to_data_t>::iterator itGender;
	for (itGender = data.data_chunks.begin();
		itGender != data.data_chunks.end();
		++itGender) {
		for (Age_to_data_t::iterator itAge = itGender->begin();
			itAge != itGender->end();
			++itAge) {
				
			// Randomise, trim, and store denormalised
			std::vector<Id_t> userids_as_vector;
			std::copy(itAge->second.userids.begin(),
			 	itAge->second.userids.end(),
				std::inserter(userids_as_vector,
				userids_as_vector.end()));
			std::random_shuffle(
				userids_as_vector.begin(), userids_as_vector.end());
			if (userids_as_vector.size() > 1000) {
				userids_as_vector.resize(1000);
			}
			itAge->second.shortlist.clear();
			std::copy(userids_as_vector.begin(), userids_as_vector.end(),
				std::inserter(itAge->second.shortlist,
				itAge->second.shortlist.end()));
		}
	}
}

void
Load::load_rare_data() {
	// Because we join on all threads before dropping out of this scope,
	// we are sure it is safe to pass this.  Also, when the threads change
	// any of the data, they do so using RWLocks.
	Load_args_t args(&this->data);

	this->threads.push_back(
		Thread::create(load_birthdays, static_cast<void *>(&args))
	);

	this->threads.push_back(
		Thread::create(load_locations, static_cast<void *>(&args))
	);

	if (program_options->verbose() >= 2) {
		std::cout << "Waiting for load threads to complete" << std::endl;
	}
	// Wait for all threads to complete.
	for (std::deque<pthread_t>::const_iterator itJoin = this->threads.begin();
		itJoin != this->threads.end();
		++itJoin) {

		pthread_join(*itJoin, NULL);
	}
	this->threads.clear();	
}

void
Load::load_overview(Id_t &min_userid, Id_t &max_userid) {
	char comma;
	unsigned int time;
	min_userid = 0;
	max_userid = 0;
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/overview";
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	// ... and read the data
	if (!request.eof()) {
		request >> time >> comma >> min_userid >> comma >> max_userid;
		if (program_options->verbose() >= 2) {
			std::cout << "min_userid is " << min_userid << " and max_userid is " << max_userid << std::endl;
		}
		if (program_options->min_userid_mult() < 0.999) {
			min_userid = max_userid * program_options->min_userid_mult();
		}
	}
}

bool
Load::reload_online_and_new() {
	try {
		Id_t min_userid, max_userid;
		load_overview(min_userid, max_userid);
		{
			ReadLock lock(this->data.lock);
			if (min_userid < this->data.last_loaded_userid) {
				min_userid = this->data.last_loaded_userid + 1;
			}
		}
		load_common_data(min_userid, max_userid);
		{
			WriteLock lock(this->data.lock);
			this->data.last_loaded_userid = max_userid;
		}
		global_stats->incrDataReloadFast();
		return true;
	} catch (...) {
		if (program_options->verbose() >= 0) {
			std::cout << "Failed to load online and new user data" << std::endl;
		}
	}
	return false;
}

void
Load::prune_threads(std::deque<pthread_t>& threads,
	std::deque<boost::shared_ptr<Load_args_t> >& args_list,
	const unsigned int min_threads, const unsigned int max_threads) {

	assert(threads.size() == args_list.size());
	if (threads.size() >= max_threads) {
		while (threads.size() > min_threads) {
			pthread_join(threads.front(), NULL);
			threads.pop_front();
			args_list.pop_front();
		}
	}
	assert(threads.size() == args_list.size());
	assert(threads.size() <= max_threads);
}

std::set<std::pair<char, char> >
string_elems(const Name_t& in) {
	std::set<std::pair<char, char> > retval;
	for (size_t i = 0; (i + 1) < in.length(); ++i) {
		retval.insert(std::make_pair(in.at(i), in.at(i + 1)));
	}
	return retval;
}

void *
load_birthdays(void *the_data) {
	Load_args_t args = *static_cast<Load_args_t *>(the_data);
	All_data_t& data(*args.data);
	
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/birthdays";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	Id_t userid;
	unsigned int age;
	char sex;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		
		WriteLock lock(data.lock);
		data.data_chunks[sex == 'f' ? 1 : 0][age].birthdays.insert(userid);
	}
	return static_cast<void *>(0);
}

void *
load_online_statuses(void *the_data) {
	Load_args_t args = *static_cast<Load_args_t *>(the_data);
	All_data_t& data(*args.data);

	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/online";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	// Drop existing data
	{
		ReadLock readlock(data.lock);
		std::vector<Age_to_data_t>::iterator itGender;
		for (itGender = data.data_chunks.begin();
			itGender != data.data_chunks.end();
			++itGender) {
			for (Age_to_data_t::iterator itAge = itGender->begin();
				itAge != itGender->end();
				++itAge) {
					
				WriteLock lock_online(itAge->second.online_lock);
				itAge->second.online.clear();
			}
		}
	}

	Id_t userid;
	unsigned int age;
	char sex;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		
		WriteLock lock(data.data_chunks[sex == 'f' ? 1 : 0][age].online_lock);
		data.data_chunks[sex == 'f' ? 1 : 0][age].online.insert(userid);
	}
	return static_cast<void *>(0);
}

void *
load_new_users(void *the_data) {
	Load_args_t args = *static_cast<Load_args_t *>(the_data);
	All_data_t& data(*args.data);

	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/new_users";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	// Drop existing data
	{
		ReadLock readlock(data.lock);
		std::vector<Age_to_data_t>::iterator itGender;
		for (itGender = data.data_chunks.begin();
			itGender != data.data_chunks.end();
			++itGender) {
			for (Age_to_data_t::iterator itAge = itGender->begin();
				itAge != itGender->end();
				++itAge) {
					
				WriteLock lock_new_users(itAge->second.new_users_lock);
				itAge->second.new_users.clear();
			}
		}
	}

	Id_t userid;
	unsigned int age;
	char sex;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		
		WriteLock lock(data.data_chunks[sex == 'f' ? 1 : 0][age].new_users_lock);
		data.data_chunks[sex == 'f' ? 1 : 0][age].new_users.insert(userid);
	}
	return static_cast<void *>(0);
}

void *
load_active_recently(void *the_data) {
	Load_args_t args = *static_cast<Load_args_t *>(the_data);
	All_data_t& data(*args.data);

	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/active_recently";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	// Drop existing data
	{
		WriteLock writelock(data.lock);
		std::vector<Age_to_data_t>::iterator itGender;
		for (itGender = data.data_chunks.begin();
			itGender != data.data_chunks.end();
			++itGender) {
			for (Age_to_data_t::iterator itAge = itGender->begin();
				itAge != itGender->end();
				++itAge) {
					
				itAge->second.active_recently.clear();
			}
		}
	}

	Id_t userid;
	unsigned int age;
	char sex;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		
		WriteLock lock(data.lock);
		data.data_chunks[sex == 'f' ? 1 : 0][age].active_recently.insert(userid);
	}
	return static_cast<void *>(0);
}

void *
load_locations(void *the_data) {
	Load_args_t args = *static_cast<Load_args_t *>(the_data);
	All_data_t& data(*args.data);

	// Will swap this in to the "global" data once we are done loading.
	// This minimises the necessary lock time.
	Id_to_id_set_t new_location_hierarchy;
	
	// Load location data and generate location hierarchy
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/locations";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	Id_t locationid, parentid;
	char comma;
	if (program_options->verbose() >= 3) {
		std::cout << "Building location hierarchy" << std::endl;
	}
	// Temporarily store a reverse of the location hierarchy.
	// This speeds up building hierarchy by approximately 250x.
	Id_to_id_set_t reverse_hierarchy;
	// Let's assume the existing set of sets is correct, and we are
	// adding (parentid, locationid) => (2, 5).
	// We insert 2 => 2 and 5 => 5.
	// We then scan the existing list to see any time we have 2 in
	// the list, and also insert 5 there.
	// The end result is that we can look up a parent location and find
	// all descendents of that parent.
	while (!request.eof()) {
		request >> locationid >> comma >> parentid;
		new_location_hierarchy[parentid].insert(parentid);
		new_location_hierarchy[locationid].insert(locationid);
		reverse_hierarchy[parentid].insert(parentid);
		reverse_hierarchy[locationid].insert(locationid);
		Id_set_t::iterator itRev;
		Id_set_t to_add_reverse;
		for (itRev = reverse_hierarchy[parentid].begin();
			itRev != reverse_hierarchy[parentid].end();
			++itRev) {

			new_location_hierarchy[*itRev].insert(locationid);
			to_add_reverse.insert(*itRev);
		}
		Id_set_t set_union;
		std::set_union(reverse_hierarchy[locationid].begin(),
			reverse_hierarchy[locationid].end(),
			to_add_reverse.begin(), to_add_reverse.end(),
			std::inserter(set_union, set_union.end()));
		reverse_hierarchy[locationid].swap(set_union);
	}
	
	{
		WriteLock lock2(data.lock);
		data.location_hierarchy.swap(new_location_hierarchy);
	}

	return static_cast<void *>(0);
}

void *
load_friends(void *the_data) {
	Load_args_t &args(*static_cast<Load_args_t *>(the_data));
	All_data_t& data(*args.data);
	Id_t start_userid = args.start_userid;
			
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/friends/" << start_userid << "/" << start_userid + 1000;
		
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
		
	Id_t userid, friendid;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> friendid;
		WriteLock lock2(data.lock);
		data.friends[userid].insert(friendid);
	}
	
	return static_cast<void *>(0);
}

void *
load_details(void *the_data) {
	Load_args_t &args(*static_cast<Load_args_t *>(the_data));
	All_data_t& data(*args.data);
	Id_t start_userid = args.start_userid;
			
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/details/" << start_userid << "/" << start_userid + 1000 << "/0";

	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));

	Id_t userid;
	unsigned int age;
	char sex;
	Id_t school;
	Id_t loc;
	unsigned short sexuality;
	unsigned short with_picture;
	unsigned short single;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex >> comma >> school >> comma >> loc >> comma >> sexuality >> comma >> with_picture >> comma >> single;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		WriteLock lock(data.lock);
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].userids.insert(userid);
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].locations[loc].insert(userid);
		if (school != 0) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].schools[school].insert(userid);
		}
		if (sexuality == 1) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].heterosexual.insert(userid);
		} else if (sexuality == 2) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].homosexual.insert(userid);
		} else if (sexuality == 3) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].bisexual.insert(userid);
		}
		if (with_picture != 0) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].with_picture.insert(userid);
		}
		if (single != 0) {
			data.data_chunks[sex == 'f' ? 1 : 0]
				[age].single_users.insert(userid);
		}
	}
		
	return static_cast<void *>(0);
}

void *
load_usernames(void *the_data) {
	Load_args_t &args(*static_cast<Load_args_t *>(the_data));
	All_data_t& data(*args.data);
	Id_t start_userid = args.start_userid;
			
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/usernames/" << start_userid << "/" << start_userid + 10000 << "/0";

	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));

	Name_t username, username_unprocessed;
	Id_t userid;
	std::set<std::pair<char, char> > elems;
	unsigned int age;
	char sex;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex >> comma;
		getline(request, username); // Some usernames have embedded spaces
		username_unprocessed = Utility::strip_whitespace(
			Utility::downcase(username));
		username = Utility::strip_string(username);
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		elems = string_elems(username);

		WriteLock lock(data.lock);
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].userids.insert(userid);
		data.usernames_unprocessed[username_unprocessed] = userid;
		std::set<std::pair<char, char> >::const_iterator elemIt;
		for (elemIt = elems.begin(); elemIt != elems.end(); ++elemIt) {
			data.data_chunks[sex == 'f' ? 1 : 0][age].
				username_suffixes[elemIt->first-'a'][elemIt->second-'a'].
				insert(userid);
		}
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].usernames[userid] = username;

	}

	return static_cast<void *>(0);
}

void *
load_realnames(void *the_data) {
	Load_args_t &args(*static_cast<Load_args_t *>(the_data));
	All_data_t& data(*args.data);
	Id_t start_userid = args.start_userid;
			
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/realnames/" << start_userid << "/" << start_userid + 10000 << "/0";
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	Name_t firstname, lastname;
	Id_t userid;
	unsigned int age;
	char sex;
	char *end_ptr;
	std::set<std::pair<char, char> > elems_first, elems_last;
	while (!request.eof()) {
		std::string s_buf;
		getline(request, s_buf);
		boost::tokenizer<boost::escaped_list_separator<char> > tok(s_buf);
		boost::tokenizer<boost::escaped_list_separator<char> >::iterator beg;
		beg = tok.begin(); // Userid
		if (beg == tok.end()) continue;
		userid = ::strtol(beg->c_str(), &end_ptr, 10);
		if (++beg == tok.end()) continue;
		age = ::strtol(beg->c_str(), &end_ptr, 10);
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		if (++beg == tok.end()) continue;
		sex = beg->c_str()[0];
		if (++beg == tok.end()) continue;
		firstname = Utility::strip_string(*beg);
		if (++beg == tok.end()) continue;
		lastname = Utility::strip_string(*beg);
		
		elems_first = string_elems(firstname);
		elems_last = string_elems(lastname);

		WriteLock lock(data.lock);
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].userids.insert(userid);
		std::set<std::pair<char, char> >::const_iterator elemIt;
		for (elemIt = elems_first.begin(); elemIt != elems_first.end(); ++elemIt) {
			data.data_chunks[sex == 'f' ? 1 : 0][age].
				firstname_suffixes[elemIt->first-'a'][elemIt->second-'a'].
				insert(userid);
		}
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].firstnames[userid] = firstname;

		for (elemIt = elems_last.begin(); elemIt != elems_last.end(); ++elemIt) {
			data.data_chunks[sex == 'f' ? 1 : 0][age].
				lastname_suffixes[elemIt->first-'a'][elemIt->second-'a'].
				insert(userid);
		}
		data.data_chunks[sex == 'f' ? 1 : 0]
			[age].lastnames[userid] = lastname;
	}

	return static_cast<void *>(0);
}

void *
load_interests(void *the_data) {
	Load_args_t &args(*static_cast<Load_args_t *>(the_data));
	All_data_t& data(*args.data);
	Id_t start_userid = args.start_userid;
			
	std::stringstream url;
	url << program_options->source_url();
	url << "userinfo/interests/" << start_userid << "/" << start_userid + 1000;
	
	if (program_options->verbose() >= 2) {
		std::cout << url.str() << std::endl;
	}
	std::stringstream request(HttpClient::request(url.str()));
	
	Id_t userid;
	unsigned int age;
	char sex;
	Id_t interest;
	char comma;
	while (!request.eof()) {
		request >> userid >> comma >> age >> comma >> sex;
		if (age > 80) age = 0;
		if (age < 13) age = 0;
		
		// Load the rest of the line, so we can parse it.
		std::string s_buf;
		getline(request, s_buf);
		std::stringstream interests_stream(s_buf);
		WriteLock lock2(data.lock);
		while (interests_stream >> comma >> interest) {
			data.data_chunks[sex == 'f' ? 1 : 0][age].interests[interest].insert(userid);
		}
	}

	return static_cast<void *>(0);
}


Load_args_t::Load_args_t(
	All_data_t *new_data,
	Id_t new_start_userid,
	Id_t new_max_userid,
	unsigned int new_since
) :
	data(new_data),
	start_userid(new_start_userid),
	max_userid(new_max_userid),
	since(new_since)
{ }
