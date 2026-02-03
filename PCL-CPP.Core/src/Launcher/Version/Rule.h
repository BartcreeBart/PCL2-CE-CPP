#pragma once
#include <string>
#include <map>
#include <optional>
#include <regex>
#include <nlohmann/json.hpp>

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 系统环境信息工具类
	 * 
	 * 用于获取当前运行环境的架构、操作系统版本等信息。
	 */
    class SystemInfo {
    public:
		/**
		 * @brief 获取当前系统架构
		 * @return 架构字符串 (例如 "x64", "x86")
		 */
        static const std::string& GetArch();

		/**
		 * @brief 获取当前操作系统版本
		 * @return 版本字符串
		 */
        static const std::string& GetOsVersion();
    };

	/**
	 * @brief 规则类，用于判断库或参数在当前环境下是否适用。
	 */
	class Rule {
		public:
		/**
		 * @brief 动作类型
		 */
		enum class ActionType {
			Allow,   ///< 允许
			Disallow ///< 禁止
		};

		ActionType Action = ActionType::Allow; ///< 规则动作

		// 操作系统匹配条件
		std::optional<std::string> OsName;    ///< 操作系统名称
		std::optional<std::string> OsVersion; ///< 操作系统版本正则
		std::optional<std::string> OsArch;    ///< 系统架构

		// 功能匹配条件
		std::map<std::string, bool> Features; ///< 功能开关要求

		/**
		 * @brief 从 JSON 对象解析规则
		 * @param j JSON 数据
		 * @return 解析出的 Rule 对象
		 */
		static Rule Parse(const nlohmann::json &j);

		/**
		 * @brief 判断该规则是否匹配当前环境
		 * @param features 当前启用的功能开关
		 * @return 如果匹配则返回 true
		 */
		bool IsMatch(const std::map<std::string, bool> &features = {}) const;
	};
}
