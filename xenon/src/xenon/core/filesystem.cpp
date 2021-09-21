#include "filesystem.h"

#include <fstream>

#include "xenon/core/log.h"

namespace xe {

	bool loadTextResource(const std::string& path, std::string& target) {
		XE_LOG_TRACE_F("FILESYSTEM: Loading file: {}", path);
		std::ifstream fileStream(path, std::ios::in);
		
		if (!fileStream.is_open()) {
			XE_LOG_ERROR_F("FILESYSTEM: Failed to open file: {}", path);
			return false;
		}

		std::string line;
		while (getline(fileStream, line)) {
			target.append(line).append("\n");
		}
		return true;
	}

}

