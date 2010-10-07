#include "lock.h"

#include <cassert>
#include <errno.h>
#include <iostream>

#include "program_options.h"

RWLock::RWLock() {
	if (pthread_rwlock_init(&this->lock, NULL) != 0) {
		throw "Unable to initialise read/write lock";
	}
}

RWLock::~RWLock() {
	pthread_rwlock_destroy(&this->lock);
}

	
ReadLock::ReadLock(boost::shared_ptr<RWLock> the_rwlock) :
	rwlock(the_rwlock)
{
	assert(the_rwlock);
	if (pthread_rwlock_rdlock(&this->rwlock->lock) != 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Unable to get read lock" << std::endl;
		}
		throw "Unable to get read lock";
	}
}

ReadLock::~ReadLock() {
	if (pthread_rwlock_unlock(&this->rwlock->lock) != 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Unable to release read lock" << std::endl;
		}
		throw "Unable to release read lock";
	}
}

WriteLock::WriteLock(boost::shared_ptr<RWLock> the_rwlock) :
	rwlock(the_rwlock)
{
	assert(the_rwlock);
	assert(&this->rwlock->lock != 0);
	int res;
	if ((res = pthread_rwlock_wrlock(&this->rwlock->lock)) != 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Unable to get write lock" << std::endl;
		}
		throw "Unable to get write lock";
	}
}

WriteLock::~WriteLock() {
	if (pthread_rwlock_unlock(&this->rwlock->lock) != 0) {
		if (program_options->verbose() >= 0) {
			std::cout << "Unable to release write lock" << std::endl;
		}
		throw "Unable to release write lock";
	}
}
