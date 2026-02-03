#pragma once
#include "ConfigContainer.h"
#include <memory>
#include <map>

namespace PCL_CPP::Core::Config {
	/**
	 * @brief 配置管理器类
	 * 
	 * @details 
	 * 该类提供了一套高度自动化的配置管理机制：
	 * 1. **单例模式**：全局唯一的访问点，通过 `GetInst()` 获取。
	 * 2. **模板机制 (Template)**：
	 *    - 维护一个 `Template.json`，存储所有可能的配置项及其默认值。
	 *    - **自动补全**：当程序升级新增配置项时，`RecursivePatch` 会自动将缺失项从硬编码默认值同步到模板中。
	 * 3. **多 Profile 支持**：
	 *    - 每个 Profile（如 `Default.json`）仅存储相对于模板的**差异项**（Diff）。
	 *    - **按需加载**：加载时，先读取模板作为基底，再通过 `RecursiveMerge` 覆盖 Profile 中的个性化设置。
	 *    - **差异化保存**：保存时，通过 `ComputeDiff` 过滤掉与模板相同的默认值，极大减小了单个配置文件的大小。
	 * 4. **线程安全**：通过互斥锁保证多线程环境下的配置读写安全。
	 */
	class ConfigManager {
		public:
		/**
		 * @brief 获取配置管理器单例实例
		 * @return ConfigManager 实例引用
		 */
		static ConfigManager &GetInst();

		/**
		 * @brief 初始化配置管理器
		 * @details 
		 * 执行流程：
		 * 1. 确保配置目录存在。
		 * 2. 加载或创建 `Template.json`。
		 * 3. 将硬编码的最新默认值合并到模板中。
		 * 4. 加载默认 Profile。
		 * @param configRoot 配置文件的根目录
		 */
		void Init(const std::filesystem::path &configRoot);

		/**
		 * @brief 获取配置模板容器
		 * @return 包含模板配置的 ConfigContainer 共享指针
		 */
		std::shared_ptr<ConfigContainer> GetTemplate() const;

		/**
		 * @brief 获取当前激活的配置 Profile 容器
		 * @return 包含当前配置的 ConfigContainer 共享指针
		 */
		std::shared_ptr<ConfigContainer> GetActiveProfile() const;

		/**
		 * @brief 加载指定的配置 Profile
		 * @details 
		 * 实现细节：
		 * 1. 读取指定的 JSON 文件。
		 * 2. 深度克隆当前的模板数据。
		 * 3. 将 Profile 数据合并到克隆出的副本中。
		 * @param profileName Profile 名称（例如 "Default"）
		 */
		void LoadProfile(const std::string &profileName);

		/**
		 * @brief 保存当前激活的配置 Profile
		 * @details 
		 * 通过 `ComputeDiff` 计算当前内存中的数据与模板数据的差异，仅将差异部分序列化为文件。
		 */
		void SaveActiveProfile();

		private:
		ConfigManager() = default;

		/**
		 * @brief 获取硬编码的默认配置（版本 0）
		 * @return 默认 JSON 配置引用
		 */
		const nlohmann::json& GetHardcodedDefaults();

		/**
		 * @brief 递归检查并补全配置项
		 * @details 
		 * 遍历 source 的所有键，如果 target 缺失该键，则从 source 拷贝；
		 * 如果类型不匹配，则以 source 为准进行重置。
		 * @param target 目标配置 JSON（将被修改）
		 * @param source 来源配置 JSON
		 */
		void RecursivePatch(nlohmann::json &target, const nlohmann::json &source);

		/**
		 * @brief 计算两个 JSON 对象之间的差异
		 * @details 
		 * 深度对比两个对象，仅保留 target 中与 source 不同（或者 source 中不存在）的键值对。
		 * @param target 目标 JSON 对象
		 * @param source 来源 JSON 对象
		 * @return 包含差异部分的 JSON 对象
		 */
		nlohmann::json ComputeDiff(const nlohmann::json &target, const nlohmann::json &source);

		/**
		 * @brief 递归合并差异到基础配置中
		 * @details 
		 * 将 diff 中的所有键值对递归应用到 base 上。
		 * @param base 基础 JSON 对象（将被修改）
		 * @param diff 包含差异的 JSON 对象
		 */
		void RecursiveMerge(nlohmann::json &base, const nlohmann::json &diff);

		std::filesystem::path m_configRoot; ///< 配置文件根目录
		std::shared_ptr<ConfigContainer> m_templateConfig; ///< 模板配置容器
		std::shared_ptr<ConfigContainer> m_activeProfile; ///< 当前激活的配置 Profile 容器
		std::string m_activeProfileName; ///< 当前激活的 Profile 名称

		mutable std::mutex m_managerMutex; ///< 保证线程安全的互斥锁
	};
}
