#include "stats.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <sstream>

boost::shared_ptr<Stats> global_stats;

using namespace boost::interprocess;

Stats::Stats(const All_data_t& new_data, pid_t parent_pid) :
	data(new_data), search_time(0), search_time_network(0),
	data_reloads_full(0), data_reloads_fast(0), in_flight(0) {
		
	boost::shared_ptr<RWLock> new_lock(new RWLock);
	this->rwlock.swap(new_lock);
	WriteLock lock(this->rwlock);
	
	std::stringstream shared_memory_name_s;
	shared_memory_name_s << "vor_shared_memory-" << parent_pid;
	this->shared_memory_name = shared_memory_name_s.str();
	
	std::stringstream mutex_name_s;
	mutex_name_s << "vor_mutex-" << parent_pid;
	this->mutex_name = mutex_name_s.str();
	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	this->start_time = tv.tv_sec;
	
	try {
		named_mutex mutex(open_or_create, this->mutex_name.c_str());
		scoped_lock<named_mutex> lock(mutex);
		
		managed_shared_memory segment(create_only,
		 	this->shared_memory_name.c_str(), 65536);
	
		// If there's no data in the shared memory area, let's create some.
		if (segment.find<time_t>("start_time").second != 1)
			segment.construct<time_t>("start_time")(this->start_time);
		if (segment.find<unsigned long>("search_time").second != 1)
			segment.construct<unsigned long>("search_time")(0);
		if (segment.find<unsigned long>("search_time_network").second != 1)
			segment.construct<unsigned long>("search_time_network")(0);
		if (segment.find<unsigned int>("data_reloads_full").second != 1)
			segment.construct<unsigned int>("data_reloads_full")(0);
		if (segment.find<unsigned int>("data_reloads_fast").second != 1)
			segment.construct<unsigned int>("data_reloads_fast")(0);
		if (segment.find<unsigned int>("in_flight").second != 1)
			segment.construct<unsigned int>("in_flight")(0);
		this->keys.push_back("searcher_userid");
		this->keys.push_back("searcher_school");
		this->keys.push_back("searcher_location");
		this->keys.push_back("min_age");
		this->keys.push_back("max_age");
		this->keys.push_back("sex");
		this->keys.push_back("username");
		this->keys.push_back("firstname");
		this->keys.push_back("lastname");
		this->keys.push_back("interest");
		this->keys.push_back("location");
		this->keys.push_back("school");
		this->keys.push_back("sexuality");
		this->keys.push_back("with_picture");
		this->keys.push_back("single");
		this->keys.push_back("birthday");
		this->keys.push_back("online");
		this->keys.push_back("new_users");
		this->keys.push_back("active_recently");
		this->keys.push_back("might_know");
		this->keys.push_back("search_reqs");
		for (std::vector<std::string>::const_iterator it = this->keys.begin();
			it != this->keys.end();
			++it) {

			this->search_reqs[*it] = 0;
			if (segment.find<unsigned int>(it->c_str()).second != 1)
				segment.construct<unsigned int>(it->c_str())(0);
		}
	} catch (...) {
		// Ignore error
	}
}

Stats::~Stats() {
}

void
Stats::save_stats() const {
	try {
		named_mutex mutex(open_only, this->mutex_name.c_str());
		scoped_lock<named_mutex> lock(mutex, try_to_lock);
		if (!lock) throw "Unable to acquire lock";

		managed_shared_memory segment(open_only,
			this->shared_memory_name.c_str());

		{
			std::pair<time_t * const, size_t> res =
				segment.find<time_t>("start_time");
			if (res.second == 1) (*res.first) = this->start_time;
		}
		{
			std::pair<unsigned long * const, size_t> res =
				segment.find<unsigned long>("search_time");
			if (res.second == 1) (*res.first) = this->search_time;
		}
		{
			std::pair<unsigned long * const, size_t> res =
				segment.find<unsigned long>("search_time_network");
			if (res.second == 1) (*res.first) = this->search_time_network;
		}
		{
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>("data_reloads_full");
			if (res.second == 1) (*res.first) = this->data_reloads_full;
		}
		{
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>("data_reloads_fast");
			if (res.second == 1) (*res.first) = this->data_reloads_fast;
		}
		for (std::vector<std::string>::const_iterator it = this->keys.begin();
			it != this->keys.end();
			++it) {
				
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>(it->c_str());
			if (res.second == 1) {
				std::map<std::string, unsigned int>::const_iterator found;
				found = this->search_reqs.find(*it);
				if (found != this->search_reqs.end()) {
					(*res.first) = found->second;
				}
			}
		}
	} catch (...) {
		// Ignore
	}
}

void
Stats::load_stats() {
	try {
		named_mutex mutex(open_only, this->mutex_name.c_str());
		scoped_lock<named_mutex> lock(mutex, try_to_lock);
		if (!lock) {
			return;
		}

		managed_shared_memory segment(open_only,
		 	this->shared_memory_name.c_str());
		
		
		{
			std::pair<time_t * const, size_t> res =
				segment.find<time_t>("start_time");
			if (res.second == 1) this->start_time = (*res.first);
		}
		{
			std::pair<unsigned long * const, size_t> res =
				segment.find<unsigned long>("search_time");
			if (res.second == 1) this->search_time = (*res.first);
		}
		{
			std::pair<unsigned long * const, size_t> res =
				segment.find<unsigned long>("search_time_network");
			if (res.second == 1) this->search_time_network = (*res.first);
		}
		{
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>("data_reloads_full");
			if (res.second == 1) this->data_reloads_full = (*res.first);
		}
		{
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>("data_reloads_fast");
			if (res.second == 1) this->data_reloads_fast = (*res.first);
		}
		for (std::vector<std::string>::const_iterator it = this->keys.begin();
			it != this->keys.end();
			++it) {
				
			std::pair<unsigned int * const, size_t> res =
				segment.find<unsigned int>(it->c_str());
			if (res.second == 1) {
				this->search_reqs[*it] = (*res.first);
			}
		}
	} catch (...) {
		// Ignore
	}
	
}

void
Stats::clear_stats() const {
	try {
		{
			named_mutex mutex(open_only, this->mutex_name.c_str());
			scoped_lock<named_mutex> lock(mutex, try_to_lock);
			if (!lock) throw "Unable to acquire lock";
			shared_memory_object::remove(this->shared_memory_name.c_str());
		}
		named_mutex::remove(this->mutex_name.c_str());
	} catch (...) {
		// Just ignore		
	}
}

unsigned int
Stats::getRunningTime() const {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (this->start_time > tv.tv_sec) {
		return 0;
	} else {
		return tv.tv_sec - this->start_time;
	}
}

size_t
Stats::getMemoryUse() const {
	return this->data.size_of();
}

unsigned long
Stats::getSearchTime() const {
	ReadLock lock(this->rwlock);
	return this->search_time;
}

unsigned long
Stats::incrSearchTime(const unsigned long taken) {
	WriteLock lock(this->rwlock);
	this->search_time += taken;
	return this->search_time;
}

unsigned long
Stats::getSearchTimeNetwork() const {
	ReadLock lock(this->rwlock);
	return this->search_time_network;
}

unsigned long
Stats::incrSearchTimeNetwork(const unsigned long taken) {
	WriteLock lock(this->rwlock);
	this->search_time_network += taken;
	return this->search_time;
}

unsigned int
Stats::getDataReloadFull() const {
	ReadLock lock(this->rwlock);
	return this->data_reloads_full;
}

unsigned int
Stats::incrDataReloadFull() {
	WriteLock lock(this->rwlock);
	return ++this->data_reloads_full;
}

unsigned int
Stats::getDataReloadFast() const {
	ReadLock lock(this->rwlock);
	return this->data_reloads_fast;
}

unsigned int
Stats::incrDataReloadFast() {
	WriteLock lock(this->rwlock);
	return ++this->data_reloads_fast;
}

unsigned int
Stats::getInFlight() const {
	ReadLock lock(this->rwlock);
	return this->in_flight;
}

unsigned int
Stats::incrInFlight() {
	WriteLock lock(this->rwlock);
	return ++this->in_flight;
}

unsigned int
Stats::decrInFlight() {
	WriteLock lock(this->rwlock);
	return --this->in_flight;
}

std::map<std::string, unsigned int>
Stats::getSearchReqs() const {
	ReadLock lock(this->rwlock);
	return this->search_reqs;  // Return a copy
}

unsigned int
Stats::getSearchReq(const std::string& which) const {
	ReadLock lock(this->rwlock);
	std::map<std::string, unsigned int>::const_iterator found;
	found = this->search_reqs.find(which);
	if (found == this->search_reqs.end()) {
		return 0;
	} else {
		return found->second;
	}
}

unsigned int
Stats::incrSearchReq(const std::string& which) {
	WriteLock lock(this->rwlock);
	return ++this->search_reqs[which];
}
