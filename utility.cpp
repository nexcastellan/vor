#include "utility.h"

#include <algorithm>
#include <cctype>

bool
Utility::not_alpha(const char c) {
	return !::isalpha(c);
}

bool
Utility::not_alpha_space(const char c) {
	if (c == ' ')
		return false;
	return not_alpha(c);
}

std::string
Utility::strip_string(const std::string& in, bool keep_space) {
	std::string out(in);
	// Strip out non-alphabetic characters
	if (keep_space) {
		out.erase(std::remove_if(out.begin(), out.end(),
		 	Utility::not_alpha_space), out.end());
	} else {
		out.erase(std::remove_if(out.begin(), out.end(), Utility::not_alpha),
	 		out.end());
	}

	return downcase(out);
}

std::string
Utility::downcase(const std::string& in) {
	std::string out(in);
	
	// Convert to lower-case
	std::transform(out.begin(), out.end(), out.begin(), ::tolower);
	
	return out;
}

std::string
Utility::strip_whitespace(const std::string& in) {
	std::string out(in);
	
	out.erase(std::remove_if(out.begin(), out.end(),
		::isspace), out.end());

	return out;	
}

