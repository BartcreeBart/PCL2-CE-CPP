#pragma once
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {
	/**
	 * @brief 游戏版本信息结构体
	 */
	struct VersionInfo {
		std::string Id;           ///< 版本 ID (例如 "1.18.2")
		std::string Type;         ///< 版本类型 (例如 "release", "snapshot")
		std::string InheritsFrom; ///< 继承自的父版本 ID
		std::string Jar;          ///< 核心 Jar 文件名
		std::string MainClass;    ///< 游戏主类
		std::string AssetsIndex;  ///< 资源索引名称

		std::filesystem::path RootPath; ///< 版本根目录
		std::filesystem::path JsonPath; ///< 版本 JSON 文件路径

		nlohmann::json RawData; ///< 原始 JSON 配置数据

		bool IsResolved = false; ///< 是否已处理完继承关系

		/**
		 * @brief 检查当前版本是否为继承版本
		 * @return 如果有继承父版本则返回 true
		 */
		bool IsInherited() const { return !InheritsFrom.empty(); }
	};

	/**
	 * @brief 版本定位器类，负责扫描和解析 Minecraft 版本。
	 * 
	 * @details 
	 * 该类实现了 Minecraft 的多版本管理逻辑：
	 * 1. **版本发现**：通过扫描 `.minecraft/versions` 目录，识别符合 `目录名/目录名.json` 结构的有效版本。
	 * 2. **继承处理 (Inheritance)**：支持版本间的配置继承（如 OptiFine 继承原版）。
	 *    - **递归解析**：如果版本 A 继承自 B，则先解析 B，再将 A 的配置合并到 B 之上。
	 *    - **循环检测**：通过维护访问链，防止 A->B->A 这样的死循环继承。
	 * 3. **配置合并 (JSON Merge)**：
	 *    - **数组追加**：对于 `libraries` 等字段，子版本会将其依赖追加到父版本列表中。
	 *    - **字段覆盖**：对于 `mainClass`、`jar` 等单一属性，子版本会覆盖父版本的值。
	 *    - **现代参数合并**：对于 `arguments` 对象中的 `game` 和 `jvm` 列表，采用深度合并策略。
	 */
	class VersionLocator {
		public:
		/**
		 * @brief 扫描并获取所有有效版本
		 * @details 
		 * 遍历版本根目录，对每个子目录尝试解析其 JSON。解析后会全局解决继承关系，
		 * 确保返回的每个 `VersionInfo` 都包含了完整的、合并后的配置。
		 * @param versionsRoot .minecraft/versions 目录路径
		 * @return 已处理完继承关系的有效版本列表
		 */
		static std::vector<VersionInfo> GetAllVersions(const std::filesystem::path &versionsRoot);

		/**
		 * @brief 获取指定 ID 的版本信息
		 * 
		 * @details 
		 * 如果该版本声明了 `inheritsFrom`，该函数会：
		 * 1. 在同级目录下查找父版本的 JSON。
		 * 2. 递归加载整条继承链上的所有版本。
		 * 3. 按照从父到子的顺序执行 `MergeJson`。
		 * 
		 * @param versionsRoot .minecraft/versions 目录路径
		 * @param id 版本 ID
		 * @return 成功则返回版本信息，否则返回 std::nullopt
		 */
		static std::optional<VersionInfo> GetVersion(const std::filesystem::path &versionsRoot, const std::string &id);

		private:
		/**
		 * @brief 解析单个 JSON 文件（不处理继承）
		 * @details 
		 * 负责基础字段的映射（Id, Type, Jar 等）以及原始 JSON 数据的存储。
		 * @param jsonPath JSON 文件路径
		 * @return 解析出的版本信息
		 */
		static std::optional<VersionInfo> ParseVersionJson(const std::filesystem::path &jsonPath);

		/**
		 * @brief 递归处理版本继承关系
		 * @details 
		 * 核心算法逻辑：
		 * - 检查 `IsResolved` 标志以避免重复处理。
		 * - 使用 `visitingChain` 进行深度优先搜索中的环检测。
		 * - 找到父版本后，递归调用自身，随后调用 `MergeJson` 将结果合并到当前对象。
		 * @param target 目标版本信息
		 * @param context 可用的版本上下文映射
		 * @param visitingChain 用于循环继承检测的链
		 */
		static void ResolveInheritance(VersionInfo &target, std::map<std::string, VersionInfo> &context, std::vector<std::string>& visitingChain);

		/**
		 * @brief 合并两个 JSON 对象（遵循 Minecraft 解析规则）
		 * @details 
		 * 合并逻辑：
		 * - **Libraries**：将 source 的库列表作为基础，追加 target 的库。
		 * - **Arguments**：对 `game` 和 `jvm` 数组执行追加操作。
		 * - **其他字段**：target 的值覆盖 source。
		 * @param target 目标 JSON 对象（将被修改）
		 * @param source 来源 JSON 对象
		 */
		static void MergeJson(nlohmann::json &target, const nlohmann::json &source);
	};
}
