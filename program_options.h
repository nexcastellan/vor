#ifndef _PROGRAM_OPTIONS_H_
#define _PROGRAM_OPTIONS_H_

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <string>

class ProgramOptions {
public:
	ProgramOptions(const int argc, char *argv[]);
	~ProgramOptions();
	
	// See --help for details on what these mean
	std::string source_url() const;
	int port() const;
	int min_threads() const;
	int max_threads() const;
	double min_userid_mult() const;
	int reload_hour() const;
	int reload_frequency() const;
	int verbose() const;
	
private:
	// Display help message
	void help() const;
	
private:
	boost::program_options::options_description desc;
	boost::program_options::variables_map vm;
};

// Global configuration options
extern boost::shared_ptr<ProgramOptions> program_options;

#endif
