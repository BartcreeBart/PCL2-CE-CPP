#pragma once
#include "Rule.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {
	struct FileInfo {
		std::string Path;
		std::string Sha1;
		size_t Size;
		std::string Url;
	};

	struct ExtractRule {
		std::vector<std::string> Exclude;
	};

	class Library {
		public:
		std::string Name; // group:artifact:version

		// Downloads
		std::optional<FileInfo> Artifact;
		std::map<std::string, FileInfo> Classifiers;

		// Natives mapping (OS Name -> Classifier Key)
		std::map<std::string, std::string> Natives;

		// Extract rules (for natives)
		std::optional<ExtractRule> Extract;

		// Rules
		std::vector<Rule> Rules;

		static Library Parse(const nlohmann::json &j);

		// check library
		bool IsActive(const std::map<std::string, bool> &features = {}) const;

		std::optional<FileInfo> GetApplicableFile(const std::map<std::string, bool> &features = {}) const;

		bool IsNative() const;
	};
}
