#include "thread.h"

#include <cassert>

pthread_t
Thread::create(void *(*start_routine)(void *), void *arg, const bool joinable) {
	pthread_t thread;
	
	// Set joinable
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if (joinable) {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	} else {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
	
	if (pthread_create(&thread, &attr, start_routine, arg) != 0) {
		throw "Unable to create thread";
	}
	return thread;
}

ThreadCondition::ThreadCondition() : count(0), condition(0), mutex(0)
{}

ThreadCondition::ThreadCondition(const ThreadCondition& other) :
	count(other.count), condition(other.condition), mutex(other.mutex)
{
	assert(other.mutex != NULL);
	assert(other.condition != NULL);
}

ThreadCondition&
ThreadCondition::operator=(const ThreadCondition& rhs) {
	assert(rhs.mutex != NULL);
	assert(rhs.condition != NULL);
	this->count = rhs.count;
	this->condition = rhs.condition;
	this->mutex = rhs.mutex;
	return *this;
}

ThreadCondition
ThreadCondition::create() {
	ThreadCondition retval;
	retval.condition = new pthread_cond_t;
	retval.mutex = new pthread_mutex_t;
	pthread_cond_init(retval.condition, NULL);
	pthread_mutex_init(retval.mutex, NULL);
	return retval;
}

void
ThreadCondition::destroy() {
	assert(this->mutex != NULL);
	assert(this->condition != NULL);
	pthread_mutex_destroy(this->mutex);
	pthread_cond_destroy(this->condition);
	delete this->mutex;
	delete this->condition;
	this->mutex = 0;
	this->condition = 0;
}

void
ThreadCondition::lock() {
	assert(this->mutex != NULL);
	assert(this->condition != NULL);
	pthread_mutex_lock(this->mutex);
}

void
ThreadCondition::unlock() {
	assert(this->mutex != NULL);
	assert(this->condition != NULL);
	pthread_mutex_unlock(this->mutex);
}

void
ThreadCondition::wait() {
	assert(this->mutex != NULL);
	assert(this->condition != NULL);
	pthread_cond_wait(this->condition, this->mutex);
}

void
ThreadCondition::signal() {
	assert(this->mutex != NULL);
	assert(this->condition != NULL);
	pthread_mutex_lock(this->mutex);
	pthread_cond_signal(this->condition);
	pthread_mutex_unlock(this->mutex);
}
