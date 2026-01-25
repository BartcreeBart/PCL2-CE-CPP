#pragma once
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {
	struct VersionInfo {
		std::string Id;
		std::string Type;         
		std::string InheritsFrom;
		std::string Jar;           
		std::string MainClass;
		std::string AssetsIndex;

		std::filesystem::path RootPath; // 版本文件夹
		std::filesystem::path JsonPath; // json 文件

		// 原始 JSON 数据
		nlohmann::json RawData;

		// 缓存标志：是否已经处理完继承关系
		bool IsResolved = false;

		bool IsInherited() const { return !InheritsFrom.empty(); }
	};
	class VersionLocator {
		public:
		// 扫描
		// 返回所有有效版本的列表，已经处理完继承关系
		static std::vector<VersionInfo> GetAllVersions(const std::filesystem::path &versionsRoot);

		// 获取单个版本信息
		// 如果版本存在继承，会尝试在同级目录下寻找父版本并合并
		static std::optional<VersionInfo> GetVersion(const std::filesystem::path &versionsRoot, const std::string &id);

		private:
		// 解析单个 json 文件，不处理继承
		static std::optional<VersionInfo> ParseVersionJson(const std::filesystem::path &jsonPath);

		// 递归处理继承
		// visitingChain 用于检测循环继承
		static void ResolveInheritance(VersionInfo &target, std::map<std::string, VersionInfo> &context, std::vector<std::string>& visitingChain);

		// 合并两个 JSON 对象 (Minecraft 规则)
		static void MergeJson(nlohmann::json &target, const nlohmann::json &source);
	};
}
