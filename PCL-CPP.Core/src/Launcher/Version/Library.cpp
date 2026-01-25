#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "Library.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

	static std::optional<FileInfo> ParseFileInfo(const nlohmann::json &j) {
		if (!j.is_object()) return std::nullopt;
		FileInfo info;
		info.Path = j.value("path", "");
		info.Sha1 = j.value("sha1", "");
		info.Size = j.value("size", 0);
		info.Url = j.value("url", "");
		return info;
	}

	Library Library::Parse(const nlohmann::json &j) {
		Library lib;
		lib.Name = j.value("name", "");

		// Downloads
		if (j.contains("downloads")) {
			const auto &dl = j["downloads"];
			if (dl.contains("artifact")) {
				lib.Artifact = ParseFileInfo(dl["artifact"]);
			}
			if (dl.contains("classifiers")) {
				for (auto &[key, val] : dl["classifiers"].items()) {
					auto info = ParseFileInfo(val);
					if (info) lib.Classifiers[key] = *info;
				}
			}
		}

		// Natives
		if (j.contains("natives")) {
			for (auto &[os, classifier] : j["natives"].items()) {
				lib.Natives[os] = classifier.get<std::string>();
			}
		}

		// Extract
		if (j.contains("extract")) {
			ExtractRule ex;
			if (j["extract"].contains("exclude")) {
				for (const auto &item : j["extract"]["exclude"]) {
					ex.Exclude.push_back(item.get<std::string>());
				}
			}
			lib.Extract = ex;
		}

		// Rules
		if (j.contains("rules")) {
			for (const auto &r : j["rules"]) {
				lib.Rules.push_back(Rule::Parse(r));
			}
		}

		return lib;
	}

	bool Library::IsActive(const std::map<std::string, bool> &features) const {
		if (Rules.empty()) return true;

		bool isAllowed = false;

		for (const auto &rule : Rules) {
			if (rule.IsMatch(features)) {
				isAllowed = (rule.Action == Rule::ActionType::Allow);
			}
		}
		return isAllowed;
	}

	bool Library::IsNative() const {
		return Natives.count("windows") > 0;
	}

	std::optional<FileInfo> Library::GetApplicableFile(const std::map<std::string, bool> &features) const {
		if (!IsActive(features)) return std::nullopt;

		if (IsNative()) {
			std::string classifierKey = Natives.at("windows");

			// Handle substitution ${arch}
			size_t pos = classifierKey.find("${arch}");
			if (pos != std::string::npos) {
				std::string bitness = (SystemInfo::GetArch() == "x86") ? "32" : "64";
				classifierKey.replace(pos, 7, bitness);
			}

			auto it = Classifiers.find(classifierKey);
			if (it != Classifiers.end()) {
				return it->second;
			} else {
				return std::nullopt;
			}
		} else {
			if (Artifact.has_value()) return Artifact.value();
			return std::nullopt;
		}
	}
}
