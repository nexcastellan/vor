#include "data_structures.h"

#include "config.h"

#ifdef HAVE_MALLOC_H
#include <fstream>
//#include <malloc.h>
#include <sstream>
#endif

#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif

All_data_t data;
struct tm last_data_loaded;
std::vector<pid_t> child_pids;

Data_chunk_t::Data_chunk_t() :
	online_lock(new RWLock),
	new_users_lock(new RWLock)
{ }

All_data_t::All_data_t() :
	lock(new RWLock), last_loaded_userid(0)
{
	for (unsigned int gender = 0; gender <= 1; ++gender) {
		Age_to_data_t by_gender;
		for (unsigned int age = 0; age <= 80; ++age) {
			Data_chunk_t chunk;
			by_gender[age] = chunk;
		}
		data_chunks.push_back(by_gender); // Male
	}
}

size_t
All_data_t::size_of() const {
#ifdef HAVE_MALLOC_H
//	return mallinfo().arena;
	// Get RSS through proc info
	std::ifstream ifs("/proc/self/status");
	std::string line;
	while (std::getline(ifs, line)) {
		std::stringstream stream(line);
		std::string string_column;
		stream >> string_column;
		if (string_column == "VmRSS:") {
			size_t rss;
			stream >> rss;
			return rss;
		}
	}
	return 0;
#endif
#ifdef HAVE_MALLOC_MALLOC_H
	return mstats().bytes_used;
#endif
	return 0;
}

