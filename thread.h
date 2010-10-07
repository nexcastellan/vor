#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

class Thread {
public:
	// Create a new thread and return a pthread_t.
	// If we are not able to create the thread, we will throw an
	// exception.  See pthread_create() for documentation on
	// start_routine and arg.  If joinable is true, we will create
	// a thread that can be joined.  Otherwise, we create a detached
	// thread.
	static pthread_t create(void *(*start_routine)(void *),
		void *arg, const bool joinable = true);
};

// This class is used to implement the boss/worker pattern.
// Boss:
// ThreadCondition condition = ThreadCondition::create();
// condition.lock();
// # spawn a worker, passing it the condition variable.
// condition.wait();
// # Now, the worker has signalled us that it is finished.
// condition.destroy();
// 
// Worker:
// # Do some work
// condition.signal();
//
// Note that we can spawn multiple workers if we wish.
class ThreadCondition {
public:
	ThreadCondition(const ThreadCondition& other);
	ThreadCondition& operator=(const ThreadCondition& rhs);
	
	// Create and initialise the necessary variables.
	static ThreadCondition create();
	
	// Clean up after ourselves.  Only the original owner should call this.
	void destroy();
	
	// Lock the mutex.  This should be called prior to spawning a thread which
	// may signal us, or immediately prior to calling wait().
	void lock();
	
	// Unlock the mutex, presumably because we won't care about any more
	// signals.
	void unlock();
	
	// Wait to be signalled.  We must have locked the mutex already.
	void wait();
	
	// Send a signal indicating that the condition has been met.  We will
	// lock and then unlock the mutex.
	void signal();
	
public:
	// Used sometimes to keep track of number of spawned threads.
	unsigned int count;

public:
	pthread_cond_t *condition;
	pthread_mutex_t *mutex;
	
private:
	ThreadCondition();
};

#endif
