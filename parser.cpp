#include "parser.h"

#include <regex>

#include <iostream>

bool parse_uniform(const std::string &line, UniformParseResult& results) {
	// Cull all lines not beginning with uniform for performance reasons
	std::size_t start = line.find_first_not_of(" \t");
	if (start == std::string::npos || line.compare(start, 7, "uniform") != 0) {
		return false;
	}

	std::smatch m;
	static const std::regex re(
					 "uniform\\s+"								// uniform
					 "(float|sampler2D|.?samplerBuffer)\\s+"	// type
					 "(\\w+)\\s*"								// uniform name
					 "(?:=\\s*([-\\d\\.]+))?\\s*;\\s*"			// optional = and default value
					 "(?://\\s*(\\w+)\\s*\\((.+)\\))?"			// optional comment at eol
	);
	std::regex_search(line, m, re);
	if (m.empty()) {
		return false;
	} else {
		if (m.size() != 6) {
			return false;
		}
		results.uniform_type = m[1];
		results.uniform_name = m[2];
		results.default_value = m[3];
		results.comment_name = m[4];
		results.comment_value = m[5];
		return true;
	}
}
