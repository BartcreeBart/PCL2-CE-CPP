#pragma once
#include "Rule.h"
#include <map>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {

	struct ArgumentPart {
		std::vector<std::string> Values;
		std::vector<Rule> Rules;

		static ArgumentPart Parse(const nlohmann::json &j);
		bool IsActive(const std::map<std::string, bool> &features) const;
	};

	class Arguments {
		public:
		std::vector<ArgumentPart> Game;
		std::vector<ArgumentPart> Jvm;

		static Arguments Parse(const nlohmann::json &j);

		std::vector<std::string> GetGameArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features = {}) const;
		std::vector<std::string> GetJvmArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features = {}) const;
	};
}
