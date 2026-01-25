#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "VersionLocator.h"
#include <fstream>
#include <set>

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

	std::vector<VersionInfo> VersionLocator::GetAllVersions(const std::filesystem::path &versionsRoot) {
		std::vector<VersionInfo> results;
		std::map<std::string, VersionInfo> versionMap;

		if (!std::filesystem::exists(versionsRoot)) {
			LOG_WARNING("Versions root not found: {}", versionsRoot.string());
			return results;
		}

		// 扫描
		for (const auto &entry : std::filesystem::directory_iterator(versionsRoot)) {
			if (entry.is_directory()) {
				std::string dirName = entry.path().filename().string();
				std::filesystem::path jsonPath = entry.path() / (dirName + ".json");

				if (std::filesystem::exists(jsonPath)) {
					auto info = ParseVersionJson(jsonPath);
					if (info) {
						versionMap[info->Id] = *info;
					}
				}
			}
		}

		LOG_INFO("Found {} potential versions.", versionMap.size());

		// 解析
		std::vector<std::string> visitingChain;

		for (auto &[id, info] : versionMap) {
			visitingChain.clear();
			ResolveInheritance(info, versionMap, visitingChain);
		}

		results.reserve(versionMap.size());
		for (const auto &[id, info] : versionMap) {
			results.push_back(info);
		}

		return results;
	}

	std::optional<VersionInfo> VersionLocator::GetVersion(const std::filesystem::path &versionsRoot, const std::string &id) {
		

		std::filesystem::path targetDir = versionsRoot / id;
		std::filesystem::path jsonPath = targetDir / (id + ".json");

		auto info = ParseVersionJson(jsonPath);
		if (!info) return std::nullopt;

		if (!info->IsInherited()) return info;

		std::map<std::string, VersionInfo> tempContext;
		tempContext[info->Id] = *info;

		std::string currentId = info->Id;
		std::set<std::string> loaded; 
		loaded.insert(currentId);

		while (true) {
			VersionInfo &currentInfo = tempContext[currentId];
			if (!currentInfo.IsInherited()) break;

			std::string parentId = currentInfo.InheritsFrom;
			if (loaded.contains(parentId)) {
				LOG_ERROR("Circular inheritance detected for version {}", id);
				break;
			}

			// 尝试加载父版本
			std::filesystem::path parentJson = versionsRoot / parentId / (parentId + ".json");
			auto parentInfo = ParseVersionJson(parentJson);

			if (parentInfo) {
				tempContext[parentId] = *parentInfo;
				loaded.insert(parentId);
				currentId = parentId; 
			} else {
				LOG_WARNING("Parent version {} not found for {}", parentId, currentInfo.Id);
				break; // 断链
			}
		}

		std::vector<std::string> visitingChain;
		ResolveInheritance(tempContext[id], tempContext, visitingChain);

		return tempContext[id];
	}

	std::optional<VersionInfo> VersionLocator::ParseVersionJson(const std::filesystem::path &jsonPath) {
		try {
			std::ifstream file(jsonPath);
			if (!file.is_open()) return std::nullopt;

			nlohmann::json j;
			file >> j;

			VersionInfo info;
			info.Id = j.value("id", "Unknown");
			info.Type = j.value("type", "release");
			info.InheritsFrom = j.value("inheritsFrom", "");
			info.Jar = j.value("jar", "");
			info.MainClass = j.value("mainClass", "");
			info.AssetsIndex = j.value("assets", "");
			info.RootPath = jsonPath.parent_path();
			info.JsonPath = jsonPath;
			info.RawData = j;

			if (info.Jar.empty() && info.InheritsFrom.empty()) {
				info.Jar = info.Id;
			}

			return info;
		} catch (const std::exception &e) {
			LOG_ERROR("Failed to parse version json {}: {}", jsonPath.string(), e.what());
			return std::nullopt;
		}
	}

	void VersionLocator::ResolveInheritance(VersionInfo &target, std::map<std::string, VersionInfo> &context, std::vector<std::string>& visitingChain) {
		// 无需继承或已解决
		if (!target.IsInherited() || target.IsResolved) {
			target.IsResolved = true;
			return;
		}

		// 循环检测
		for (const auto& v : visitingChain) {
			if (v == target.Id) {
				LOG_ERROR("Circular inheritance detected: {} in chain", target.Id);
				return;
			}
		}
		visitingChain.push_back(target.Id);

		// 找父
		auto it = context.find(target.InheritsFrom);
		if (it == context.end()) {
			LOG_WARNING("Version {} inherits from {}, but parent not found in context.", target.Id, target.InheritsFrom);
			target.IsResolved = true;
			visitingChain.pop_back();
			return;
		}

		VersionInfo& parent = it->second;

		// 递归解决父版本
		ResolveInheritance(parent, context, visitingChain);

		// 将父版本数据合并到当前版本
		MergeJson(target.RawData, parent.RawData);

		if (target.Jar.empty()) target.Jar = parent.Jar;
		if (target.MainClass.empty()) target.MainClass = parent.MainClass;
		if (target.AssetsIndex.empty()) target.AssetsIndex = parent.AssetsIndex;

		target.IsResolved = true;
		visitingChain.pop_back();
	}

	void VersionLocator::MergeJson(nlohmann::json &target, const nlohmann::json &source){

		nlohmann::json result = source; 

		for (auto &[key, val] : target.items()) {
			if (key == "libraries") {
				if (result.contains("libraries") && result["libraries"].is_array() && val.is_array()) {
					for (const auto &lib : val) {
						result["libraries"].push_back(lib);
					}
				} else {
					result[key] = val;
				}
			} else if (key == "minecraftArguments") {
				result[key] = val;
			} else if (key == "arguments") {
				if (result.contains("arguments") && result["arguments"].is_object() && val.is_object()) {
					if (val.contains("game")) {
						if (!result["arguments"].contains("game") || !result["arguments"]["game"].is_array()) {
							result["arguments"]["game"] = val["game"];
						} else {
							nlohmann::json &gameArgs = result["arguments"]["game"];
							for (auto &arg : val["game"]) gameArgs.push_back(arg);
						}
					}
					if (val.contains("jvm")) {
						if (!result["arguments"].contains("jvm") || !result["arguments"]["jvm"].is_array()) {
							result["arguments"]["jvm"] = val["jvm"];
						} else {
							nlohmann::json &jvmArgs = result["arguments"]["jvm"];
							for (auto &arg : val["jvm"]) jvmArgs.push_back(arg);
						}
					}
				} else {
					result[key] = val;
				}
			} else {
				result[key] = val;
			}
		}

		target = result;
	}
}
