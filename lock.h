#ifndef _LOCK_H_
#define _LOCK_H_

#include <boost/shared_ptr.hpp>
#include <pthread.h>

// Provide access to a read/write lock using the RAII design pattern.
class RWLock {
public:
	// Initialise a read/write lock.
	RWLock();
	// Destroy the read/write lock.
	~RWLock();

private:
	RWLock(const RWLock& other);
	RWLock& operator=(const RWLock& rhs);
	
private:
	pthread_rwlock_t lock;
	friend class ReadLock;
	friend class WriteLock;
};

// Lock a specific RWLock for reading.  This can be locked any number of
// times for reading, but if someone wants to lock it for writing, all
// readers will be blocked for the duration.
// We use RAII, so the lock will be dropped once the object falls out of
// scope.
class ReadLock {
public:
	ReadLock(boost::shared_ptr<RWLock> the_rwlock);
	~ReadLock();
	
private:
	boost::shared_ptr<RWLock> rwlock;
	
private:
	ReadLock();
	ReadLock(const ReadLock& other);
	ReadLock& operator=(const ReadLock& rhs);
};

// Lock a specific RWLock for reading.  This can be locked any number of
// times for reading, but if someone wants to lock it for writing, all
// readers will be blocked for the duration.
// We use RAII, so the lock will be dropped once the object falls out of
// scope.
class WriteLock {
public:
	WriteLock(boost::shared_ptr<RWLock> the_rwlock);
	~WriteLock();
	
private:
	boost::shared_ptr<RWLock> rwlock;
	
private:
	WriteLock();
	WriteLock(const WriteLock& other);
	WriteLock& operator=(const WriteLock& rhs);
};

#endif
