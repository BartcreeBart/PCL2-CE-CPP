#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "Rule.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 获取当前 Windows 系统的版本号
	 * @return 版本号字符串 (例如 "10.0")
	 */
    static std::string FetchWindowsVersion() {
        // 使用 RtlGetVersion 获取真实版本号，避免被兼容性模式欺骗
        typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
        HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
        if (hMod) {
            RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
            if (fxPtr != nullptr) {
                RTL_OSVERSIONINFOW rovi = { 0 };
                rovi.dwOSVersionInfoSize = sizeof(rovi);
                if (fxPtr(&rovi) == 0) {
                    return std::to_string(rovi.dwMajorVersion) + "." + std::to_string(rovi.dwMinorVersion);
                }
            }
        }
        // 回退默认值
        return "10.0"; 
    }

	/**
	 * @brief 获取当前系统的架构信息
	 * @return 架构字符串 ("x64", "x86", "arm64")
	 */
    static std::string FetchSystemArchitecture() {
        // 检查是否运行在 WOW64 环境下
        BOOL isWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &isWow64);
        
        if (isWow64) {
            return "x64";
        } else {
            SYSTEM_INFO si;
            GetNativeSystemInfo(&si);

            if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                return "x64";
            } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                return "x86";
            } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
                return "arm64";
            }
        }
        return "x86"; // 默认回退到 x86
    }

	/**
	 * @brief 获取当前系统架构
	 * @return 架构字符串 (例如 "x64", "x86")
	 */
    const std::string& SystemInfo::GetArch() {
        static std::string arch = FetchSystemArchitecture();
        return arch;
    }

	/**
	 * @brief 获取当前操作系统版本
	 * @return 版本字符串
	 */
    const std::string& SystemInfo::GetOsVersion() {
        static std::string ver = FetchWindowsVersion();
        return ver;
    }

	/**
	 * @brief 从 JSON 对象解析规则
	 * @param j JSON 数据
	 * @return 解析出的 Rule 对象
	 */
    Rule Rule::Parse(const nlohmann::json& j) {
        Rule rule;

        // 解析动作类型 (action)
        std::string actionStr = j.value("action", "allow");
        rule.Action = (actionStr == "allow") ? ActionType::Allow : ActionType::Disallow;

        // 解析操作系统限制 (os)
        if (j.contains("os")) {
            const auto& os = j["os"];
            if (os.contains("name")) rule.OsName = os["name"].get<std::string>();
            if (os.contains("version")) rule.OsVersion = os["version"].get<std::string>();
            if (os.contains("arch")) rule.OsArch = os["arch"].get<std::string>();
        }

        // 解析功能开关限制 (features)
        if (j.contains("features")) {
            for (auto& [key, val] : j["features"].items()) {
                if (val.is_boolean()) {
                    rule.Features[key] = val.get<bool>();
                }
            }
        }

        return rule;
    }

	/**
	 * @brief 判断该规则是否匹配当前环境
	 * @param currentFeatures 当前启用的功能开关
	 * @return 如果匹配则返回 true
	 */
    bool Rule::IsMatch(const std::map<std::string, bool>& currentFeatures) const {
        // 如果指定了系统名称且不是 Windows，则不匹配（本项目仅支持 Windows）
        if (OsName.has_value() && OsName.value() != "windows") return false;

        // 检查架构是否匹配
        if (OsArch.has_value()) {
            if (OsArch.value() != SystemInfo::GetArch()) return false;
        }

        // 检查操作系统版本（支持正则表达式）
        if (OsVersion.has_value()) {
            try {
                std::regex re(OsVersion.value());
                if (!std::regex_search(SystemInfo::GetOsVersion(), re)) return false;
            }
            catch (...) {
                LOG_WARNING("Invalid regex in rule: {}", OsVersion.value());
                return false;
            }
        }

        // 检查功能开关是否满足要求
		for (const auto &[feat, requiredVal] : Features) {
			auto it = currentFeatures.find(feat);
			bool actualVal = (it != currentFeatures.end()) ? it->second : false;
			if (actualVal != requiredVal) return false;
		}

		return true;
	}
}
