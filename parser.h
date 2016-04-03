#pragma once

#include <string>
#include <sstream>

struct UniformParseResult {
	std::string uniform_name;
	std::string uniform_type;
	std::string default_value;
	std::string comment_name;
	std::string comment_value;
};

// If find a float in line fill ud and return true
// Return false if not found
//uniform float xxx = 0.01; // ui(0.004, 3.443)
//uniform sampler2D color_map; // path(../../red.png)
bool parse_uniform(const std::string &line, UniformParseResult& results);

template<class T>
bool parse_value(const std::string &str, T &value) {
	T d;
	std::istringstream ss(str);

	ss >> d;
	if (ss.bad() || ss.fail()) {
		return false;
	}
	value = d;
	return true;
}
