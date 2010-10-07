#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <string>

class Utility {
public:
	// Return true if the given character is not 'a'-'z','A'-'Z'
	static bool not_alpha(const char c);

	// Return true if the given character is not 'a'-'z','A'-'Z', ' '
	static bool not_alpha_space(const char c);

	// Strip all non-alphabetic characters and convert to lower-case.
	static std::string strip_string(const std::string& in, bool keep_space = false);

	// Convert to lower-case
	static std::string downcase(const std::string& in);
	
	// Remove all whitespace
	static std::string strip_whitespace(const std::string& in);
};

#endif
