#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "VersionLocator.h"
#include <fstream>
#include <set>

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 扫描并获取所有有效版本
	 * @param versionsRoot .minecraft/versions 目录路径
	 * @return 已处理完继承关系的有效版本列表
	 */
	std::vector<VersionInfo> VersionLocator::GetAllVersions(const std::filesystem::path &versionsRoot) {
		std::vector<VersionInfo> results;
		std::map<std::string, VersionInfo> versionMap;

		if (!std::filesystem::exists(versionsRoot)) {
			LOG_WARNING("Versions root not found: {}", versionsRoot.string());
			return results;
		}

		// 第一步：扫描目录并初步解析所有 JSON 文件
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

		// 第二步：递归处理版本继承关系
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

	/**
	 * @brief 获取指定 ID 的版本信息
	 * @param versionsRoot .minecraft/versions 目录路径
	 * @param id 版本 ID
	 * @return 成功则返回版本信息，否则返回 std::nullopt
	 */
	std::optional<VersionInfo> VersionLocator::GetVersion(const std::filesystem::path &versionsRoot, const std::string &id) {
		std::filesystem::path targetDir = versionsRoot / id;
		std::filesystem::path jsonPath = targetDir / (id + ".json");

		auto info = ParseVersionJson(jsonPath);
		if (!info) return std::nullopt;

		// 如果没有继承关系，直接返回
		if (!info->IsInherited()) return info;

		// 构建临时上下文以处理继承
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

			// 尝试在同级目录下寻找并加载父版本
			std::filesystem::path parentJson = versionsRoot / parentId / (parentId + ".json");
			auto parentInfo = ParseVersionJson(parentJson);

			if (parentInfo) {
				tempContext[parentId] = *parentInfo;
				loaded.insert(parentId);
				currentId = parentId; 
			} else {
				LOG_WARNING("Parent version {} not found for {}", parentId, currentInfo.Id);
				break; // 继承链断裂
			}
		}

		std::vector<std::string> visitingChain;
		ResolveInheritance(tempContext[id], tempContext, visitingChain);

		return tempContext[id];
	}

	/**
	 * @brief 解析单个 JSON 文件（不处理继承）
	 * @param jsonPath JSON 文件路径
	 * @return 解析出的版本信息
	 */
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

			// 如果未指定 Jar 文件名且没有继承关系，则默认与 ID 相同
			if (info.Jar.empty() && info.InheritsFrom.empty()) {
				info.Jar = info.Id;
			}

			return info;
		} catch (const std::exception &e) {
			LOG_ERROR("Failed to parse version json {}: {}", jsonPath.string(), e.what());
			return std::nullopt;
		}
	}

	/**
	 * @brief 递归处理版本继承关系
	 * @param target 目标版本信息
	 * @param context 可用的版本上下文映射
	 * @param visitingChain 用于循环继承检测的链
	 */
	void VersionLocator::ResolveInheritance(VersionInfo &target, std::map<std::string, VersionInfo> &context, std::vector<std::string>& visitingChain) {
		// 无需继承或已处理完毕
		if (!target.IsInherited() || target.IsResolved) {
			target.IsResolved = true;
			return;
		}

		// 循环继承检测
		for (const auto& v : visitingChain) {
			if (v == target.Id) {
				LOG_ERROR("Circular inheritance detected: {} in chain", target.Id);
				return;
			}
		}
		visitingChain.push_back(target.Id);

		// 查找父版本
		auto it = context.find(target.InheritsFrom);
		if (it == context.end()) {
			LOG_WARNING("Version {} inherits from {}, but parent not found in context.", target.Id, target.InheritsFrom);
			target.IsResolved = true;
			visitingChain.pop_back();
			return;
		}

		VersionInfo& parent = it->second;

		// 递归先处理父版本的继承
		ResolveInheritance(parent, context, visitingChain);

		// 将父版本的数据合并到当前版本
		MergeJson(target.RawData, parent.RawData);

		// 补全缺失的关键信息
		if (target.Jar.empty()) target.Jar = parent.Jar;
		if (target.MainClass.empty()) target.MainClass = parent.MainClass;
		if (target.AssetsIndex.empty()) target.AssetsIndex = parent.AssetsIndex;

		target.IsResolved = true;
		visitingChain.pop_back();
	}

	/**
	 * @brief 合并两个 JSON 对象（遵循 Minecraft 解析规则）
	 * @param target 目标 JSON 对象（将被修改）
	 * @param source 来源 JSON 对象
	 */
	void VersionLocator::MergeJson(nlohmann::json &target, const nlohmann::json &source){

		nlohmann::json result = source; 

		for (auto &[key, val] : target.items()) {
			if (key == "libraries") {
				// 库列表采取追加方式合并
				if (result.contains("libraries") && result["libraries"].is_array() && val.is_array()) {
					for (const auto &lib : val) {
						result["libraries"].push_back(lib);
					}
				} else {
					result[key] = val;
				}
			} else if (key == "minecraftArguments") {
				// 旧版参数直接覆盖
				result[key] = val;
			} else if (key == "arguments") {
				// 现代版参数对象递归合并
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
				// 其他字段直接覆盖
				result[key] = val;
			}
		}

		target = result;
	}
}
