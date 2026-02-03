#include "pch.h"
#include "Arguments.h"
#include <regex>

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 从 JSON 对象解析参数项
	 * @param j JSON 数据
	 * @return 解析出的 ArgumentPart 对象
	 */
	ArgumentPart ArgumentPart::Parse(const nlohmann::json &j) {
		ArgumentPart part;
		if (j.is_string()) {
			part.Values.push_back(j.get<std::string>());
		} else if (j.is_object()) {
			// 解析参数值 (value)
			if (j.contains("value")) {
				const auto &val = j["value"];
				if (val.is_string()) {
					part.Values.push_back(val.get<std::string>());
				} else if (val.is_array()) {
					for (const auto &item : val) {
						part.Values.push_back(item.get<std::string>());
					}
				}
			}
			// 解析启用规则 (rules)
			if (j.contains("rules")) {
				for (const auto &r : j["rules"]) {
					part.Rules.push_back(Rule::Parse(r));
				}
			}
		}
		return part;
	}

	/**
	 * @brief 检查参数项在当前环境下是否激活
	 * @param features 当前启用的功能开关
	 * @return 如果应激活则返回 true
	 */
	bool ArgumentPart::IsActive(const std::map<std::string, bool> &features) const {
		if (Rules.empty()) return true;

		bool isAllowed = false;
		for (const auto &rule : Rules) {
			if (rule.IsMatch(features)) {
				isAllowed = (rule.Action == Rule::ActionType::Allow);
			}
		}
		return isAllowed;
	}

	/**
	 * @brief 从 JSON 对象解析参数配置
	 * @param j JSON 数据
	 * @return 解析出的 Arguments 对象
	 */
	Arguments Arguments::Parse(const nlohmann::json &j) {
		Arguments args;

		if (j.contains("game")) {
			for (const auto &item : j["game"]) {
				args.Game.push_back(ArgumentPart::Parse(item));
			}
		}

		if (j.contains("jvm")) {
			for (const auto &item : j["jvm"]) {
				args.Jvm.push_back(ArgumentPart::Parse(item));
			}
		}

		return args;
	}

	/**
	 * @brief 执行参数占位符替换
	 * @param str 包含占位符的字符串
	 * @param substitutions 替换映射表
	 * @return 替换后的字符串
	 */
	static std::string DoSubstitution(std::string str, const std::map<std::string, std::string> &substitutions) {
		for (const auto &[key, val] : substitutions) {
			std::string placeholder = "${" + key + "}";
			size_t pos = 0;
			while ((pos = str.find(placeholder, pos)) != std::string::npos) {
				str.replace(pos, placeholder.length(), val);
				pos += val.length();
			}
		}
		return str;
	}

	/**
	 * @brief 获取处理后的游戏启动参数列表
	 * @param substitutions 参数替换映射表 (例如 "${version_name}" -> "1.18.2")
	 * @param features 当前启用的功能开关
	 * @return 替换后的完整游戏参数列表
	 */
	std::vector<std::string> Arguments::GetGameArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features) const {
		std::vector<std::string> result;
		for (const auto &part : Game) {
			if (part.IsActive(features)) {
				for (const auto &val : part.Values) {
					result.push_back(DoSubstitution(val, substitutions));
				}
			}
		}
		return result;
	}

	/**
	 * @brief 获取处理后的 JVM 启动参数列表
	 * @param substitutions 参数替换映射表
	 * @param features 当前启用的功能开关
	 * @return 替换后的完整 JVM 参数列表
	 */
	std::vector<std::string> Arguments::GetJvmArgs(const std::map<std::string, std::string> &substitutions, const std::map<std::string, bool> &features) const {
		std::vector<std::string> result;
		for (const auto &part : Jvm) {
			if (part.IsActive(features)) {
				for (const auto &val : part.Values) {
					result.push_back(DoSubstitution(val, substitutions));
				}
			}
		}
		return result;
	}
}
