#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "Rule.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

    static std::string FetchWindowsVersion() {
        // 使用 RtlGetVersion 获取版本号
        // 需要动态加载 ntdll.dll
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
        // 古老的系统嘛不管了
        return "10.0"; 
    }

    static std::string FetchSystemArchitecture() {
        // WOW64 (32位程序运行在64位系统上)
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
                return "arm64"; // 可能么
            }
        }
        return "x86"; // 默认回退
    }

    const std::string& SystemInfo::GetArch() {
        static std::string arch = FetchSystemArchitecture();
        return arch;
    }

    const std::string& SystemInfo::GetOsVersion() {
        static std::string ver = FetchWindowsVersion();
        return ver;
    }

    Rule Rule::Parse(const nlohmann::json& j) {
        Rule rule;

        // Action
        std::string actionStr = j.value("action", "allow");
        rule.Action = (actionStr == "allow") ? ActionType::Allow : ActionType::Disallow;

        // OS
        if (j.contains("os")) {
            const auto& os = j["os"];
            if (os.contains("name")) rule.OsName = os["name"].get<std::string>();
            if (os.contains("version")) rule.OsVersion = os["version"].get<std::string>();
            if (os.contains("arch")) rule.OsArch = os["arch"].get<std::string>();
        }

        // Features
        if (j.contains("features")) {
            for (auto& [key, val] : j["features"].items()) {
                if (val.is_boolean()) {
                    rule.Features[key] = val.get<bool>();
                }
            }
        }

        return rule;
    }

    bool Rule::IsMatch(const std::map<std::string, bool>& currentFeatures) const {
        if (OsName.has_value() && OsName.value() != "windows") return false;

        // Arch
        if (OsArch.has_value()) {
            if (OsArch.value() != SystemInfo::GetArch()) return false;
        }

        // OS Version
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

		for (const auto &[feat, requiredVal] : Features) {
			auto it = currentFeatures.find(feat);
			bool actualVal = (it != currentFeatures.end()) ? it->second : false;
			if (actualVal != requiredVal) return false;
		}

		return true;
	}
}
