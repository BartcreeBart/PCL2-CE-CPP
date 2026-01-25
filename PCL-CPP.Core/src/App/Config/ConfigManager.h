#pragma once
#include "ConfigContainer.h"
#include <memory>
#include <map>

namespace PCL_CPP::Core::Config {
	class ConfigManager {
		public:
		static ConfigManager &GetInst();

		// 初始化：设置配置目录，加载/修复 Template
		void Init(const std::filesystem::path &configRoot);

		// 获取 Template 配置容器
		std::shared_ptr<ConfigContainer> GetTemplate() const;

		// 获取当前激活的配置
		std::shared_ptr<ConfigContainer> GetActiveProfile() const;

		// 加载指定 Profile (如果不存在则创建默认)
		void LoadProfile(const std::string &profileName);

		// 保存当前 Profile (只保存差异)
		void SaveActiveProfile();

		private:
		ConfigManager() = default;

		// 获取硬编码的默认配置 (Version 0)
		const nlohmann::json& GetHardcodedDefaults();

		// 递归检查并补全配置
		void RecursivePatch(nlohmann::json &target, const nlohmann::json &source);

		// 计算差异
		nlohmann::json ComputeDiff(const nlohmann::json &target, const nlohmann::json &source);

		// 递归合并
		void RecursiveMerge(nlohmann::json &base, const nlohmann::json &diff);

		std::filesystem::path m_configRoot;
		std::shared_ptr<ConfigContainer> m_templateConfig;
		std::shared_ptr<ConfigContainer> m_activeProfile;
		std::string m_activeProfileName;

		mutable std::mutex m_managerMutex;
	};
}
