#include "pch.h"
#include "Arguments.h"
#include <regex>

namespace PCL_CPP::Core::Launcher::Version {

	ArgumentPart ArgumentPart::Parse(const nlohmann::json &j) {
		ArgumentPart part;
		if (j.is_string()) {
			part.Values.push_back(j.get<std::string>());
		} else if (j.is_object()) {
			// Value
			if (j.contains("value")) {
				const auto &val = j["value"];
				if (val.is_string()) {
					part.Values.push_back(val.get<std::string>());
				} else if (val.is_array()) {
					for (const auto &item : val) {
						part.Values.push_back(item.get<std::string>());
					}
				}
			}
			// Rules
			if (j.contains("rules")) {
				for (const auto &r : j["rules"]) {
					part.Rules.push_back(Rule::Parse(r));
				}
			}
		}
		return part;
	}

	bool ArgumentPart::IsActive(const std::map<std::string, bool> &features) const {
		if (Rules.empty()) return true;

		bool isAllowed = false;
		for (const auto &rule : Rules) {
			if (rule.IsMatch(features)) {
				isAllowed = (rule.Action == Rule::ActionType::Allow);
			}
		}
		return isAllowed;
	}

	Arguments Arguments::Parse(const nlohmann::json &j) {
		Arguments args;

		if (j.contains("game")) {
			for (const auto &item : j["game"]) {
				args.Game.push_back(ArgumentPart::Parse(item));
			}
		}

		if (j.contains("jvm")) {
			for (const auto &item : j["jvm"]) {
				args.Jvm.push_back(ArgumentPart::Parse(item));
			}
		}

		return args;
	}

	static std::string DoSubstitution(std::string str, const std::map<std::string, std::string> &substitutions) {
		for (const auto &[key, val] : substitutions) {
			std::string placeholder = "${" + key + "}";
			size_t pos = 0;
			while ((pos = str.find(placeholder, pos)) != std::string::npos) {
				str.replace(pos, placeholder.length(), val);
				pos += val.length();
			}
		}
		return str;
	}

	std::vector<std::string> Arguments::GetGameArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features) const {
		std::vector<std::string> result;
		for (const auto &part : Game) {
			if (part.IsActive(features)) {
				for (const auto &val : part.Values) {
					result.push_back(DoSubstitution(val, substitutions));
				}
			}
		}
		return result;
	}

	std::vector<std::string> Arguments::GetJvmArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features) const {
		std::vector<std::string> result;
		for (const auto &part : Jvm) {
			if (part.IsActive(features)) {
				for (const auto &val : part.Values) {
					result.push_back(DoSubstitution(val, substitutions));
				}
			}
		}
		return result;
	}
}
