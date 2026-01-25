#pragma once
#include <string>
#include <map>
#include <optional>
#include <regex>
#include <nlohmann/json.hpp>

namespace PCL_CPP::Core::Launcher::Version {

    // 运行环境信息缓存
    class SystemInfo {
    public:
        // 结果会被缓存，不会重复获取
        static const std::string& GetArch();
        static const std::string& GetOsVersion();
    };

	class Rule {
		public:
		enum class ActionType {
			Allow,
			Disallow
		};

		ActionType Action = ActionType::Allow;

		// OS
		std::optional<std::string> OsName;
		std::optional<std::string> OsVersion;
		std::optional<std::string> OsArch;

		// Feature
		std::map<std::string, bool> Features;

		// 解析单个规则对象
		static Rule Parse(const nlohmann::json &j);

		// 判断该规则是否匹配当前环境
		// features: 当前启用的额外功能特性
		bool IsMatch(const std::map<std::string, bool> &features = {}) const;
	};
}
