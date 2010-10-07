#include "program_options.h"

#include <iostream>
#include <fstream>

namespace po = boost::program_options;

boost::shared_ptr<ProgramOptions> program_options;

ProgramOptions::ProgramOptions(const int argc, char* argv[]) :
	desc("Options") {
		
	int opt_i;
	double opt_d;
	std::string opt_s;
	this->desc.add_options()
		("help", "Display this information")
		("config",
		 po::value<std::string>(&opt_s)->default_value("./vor.cfg"),
		 "Full path to config file")
		("source_url",
		 po::value<std::string>(&opt_s)->default_value("http://www.nexopia.com/"),
		 "Source URL to pull data from")
		("port", po::value<int>(&opt_i)->default_value(6974),
		 "Listen for incoming connections on this port")
		("min_threads", po::value<int>(&opt_i)->default_value(8),
		 "Minimum number of threads during initial data retrieval")
		("max_threads", po::value<int>(&opt_i)->default_value(16),
		 "Maximum number of threads during initial data retrieval")
		("reload_hour", po::value<int>(&opt_i)->default_value(3),
		 "Hour of day to reload data (localtime, 24 hour clock)")
		("reload_frequency", po::value<int>(&opt_i)->default_value(10),
		 "How often (every x minutes) to reload newuser/online data")
		("verbose,v", po::value<int>(&opt_i)->default_value(1),
		 "Verbosity level, 0-3")
		("min_userid_mult", po::value<double>(&opt_d)->default_value(1.0),
		 "minimum userid as multiple of maximum userid (debugging only)")
	;
	
	try {
		po::store(po::parse_command_line(
			argc, argv, this->desc), this->vm);
		po::notify(this->vm);
	
		std::ifstream ifs(this->vm["config"].as<std::string>().c_str());
		po::store(po::parse_config_file(ifs, this->desc), this->vm);
	} catch (...) {
		std::cerr << "Unable to parse options:" << std::endl;
		for (int i = 0; i < argc; ++i) {
			std::cerr << argv[i] << " ";
		}
		std::cerr << std::endl;
		exit(1);
	}
	

	if (this->vm.count("help")) {
		help();
		exit(0);
	}
}

ProgramOptions::~ProgramOptions() {
}

void
ProgramOptions::help() const {
	std::cout << this->desc << std::endl;
}

std::string
ProgramOptions::source_url() const {
	return this->vm["source_url"].as<std::string>();
}

int
ProgramOptions::port() const {
	return this->vm["port"].as<int>();
}

int
ProgramOptions::min_threads() const {
	return this->vm["min_threads"].as<int>();
}

int
ProgramOptions::max_threads() const {
	return this->vm["max_threads"].as<int>();
}

double
ProgramOptions::min_userid_mult() const {
	return this->vm["min_userid_mult"].as<double>();
}

int
ProgramOptions::reload_hour() const {
	return this->vm["reload_hour"].as<int>();
}

int
ProgramOptions::reload_frequency() const {
	return this->vm["reload_frequency"].as<int>() * 60; // conv to minutes
}

int
ProgramOptions::verbose() const {
	return this->vm["verbose"].as<int>();
}
