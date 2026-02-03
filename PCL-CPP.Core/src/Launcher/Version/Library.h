#pragma once
#include "Rule.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {
	/**
	 * @brief 文件信息结构体
	 */
	struct FileInfo {
		std::string Path; ///< 文件的相对路径
		std::string Sha1; ///< 文件的 SHA-1 校验值
		size_t Size;      ///< 文件大小（字节）
		std::string Url;  ///< 下载链接
	};

	/**
	 * @brief 提取规则结构体（主要用于 Natives）
	 */
	struct ExtractRule {
		std::vector<std::string> Exclude; ///< 提取时排除的文件列表
	};

	/**
	 * @brief 依赖库类，代表 Minecraft 运行所需的一个 Jar 依赖或 Native 库。
	 */
	class Library {
		public:
		std::string Name; ///< 库的名称 (格式为 group:artifact:version)

		// 下载信息
		std::optional<FileInfo> Artifact; ///< 主文件下载信息
		std::map<std::string, FileInfo> Classifiers; ///< 各分类器对应的下载信息

		// Natives 映射 (OS 名称 -> 分类器键)
		std::map<std::string, std::string> Natives;

		// 提取规则 (用于 Natives)
		std::optional<ExtractRule> Extract;

		// 启用规则
		std::vector<Rule> Rules;

		/**
		 * @brief 从 JSON 对象解析 Library
		 * @param j JSON 数据
		 * @return 解析出的 Library 对象
		 */
		static Library Parse(const nlohmann::json &j);

		/**
		 * @brief 检查该库在当前环境下是否应被激活
		 * @param features 当前启用的功能开关
		 * @return 如果应激活则返回 true
		 */
		bool IsActive(const std::map<std::string, bool> &features = {}) const;

		/**
		 * @brief 获取适用于当前环境的文件信息
		 * @param features 当前启用的功能开关
		 * @return 适用的文件信息，如果不适用则返回 std::nullopt
		 */
		std::optional<FileInfo> GetApplicableFile(const std::map<std::string, bool> &features = {}) const;

		/**
		 * @brief 检查该库是否为 Native 库
		 * @return 如果是 Native 库则返回 true
		 */
		bool IsNative() const;
	};
}
