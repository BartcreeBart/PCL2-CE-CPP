#pragma once
#include "Rule.h"
#include <map>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 参数项结构体，包含一个或多个参数值及其启用规则。
	 */
	struct ArgumentPart {
		std::vector<std::string> Values; ///< 参数值列表
		std::vector<Rule> Rules;         ///< 启用该参数项需满足的规则

		/**
		 * @brief 从 JSON 对象解析参数项
		 * @param j JSON 数据
		 * @return 解析出的 ArgumentPart 对象
		 */
		static ArgumentPart Parse(const nlohmann::json &j);

		/**
		 * @brief 检查该参数项在当前环境下是否激活
		 * @param features 当前启用的功能开关
		 * @return 如果应激活则返回 true
		 */
		bool IsActive(const std::map<std::string, bool> &features) const;
	};

	/**
	 * @brief 启动参数管理类，负责处理游戏参数和 JVM 参数。
	 */
	class Arguments {
		public:
		std::vector<ArgumentPart> Game; ///< 游戏启动参数列表
		std::vector<ArgumentPart> Jvm;  ///< JVM 启动参数列表

		/**
		 * @brief 从 JSON 对象解析参数配置
		 * @param j JSON 数据
		 * @return 解析出的 Arguments 对象
		 */
		static Arguments Parse(const nlohmann::json &j);

		/**
		 * @brief 获取处理后的游戏启动参数列表
		 * @param substitutions 参数替换映射表 (例如 "${version_name}" -> "1.18.2")
		 * @param features 当前启用的功能开关
		 * @return 替换后的完整游戏参数列表
		 */
		std::vector<std::string> GetGameArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features = {}) const;

		/**
		 * @brief 获取处理后的 JVM 启动参数列表
		 * @param substitutions 参数替换映射表
		 * @param features 当前启用的功能开关
		 * @return 替换后的完整 JVM 参数列表
		 */
		std::vector<std::string> GetJvmArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features = {}) const;
	};
}
