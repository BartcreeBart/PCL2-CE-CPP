#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "LaunchPlanner.h"
#include "Launcher/Version/Arguments.h"
#include "Launcher/Version/Library.h"
#include "Launcher/Launch/NativesUtils.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Launch {

	/**
	 * @brief 构造函数，初始化启动规划器
	 * @param version 版本信息
	 * @param ctx 启动上下文
	 */
	LaunchPlanner::LaunchPlanner(const Version::VersionInfo &version, const LaunchContext &ctx)
		: _version(version), _ctx(ctx) {

		// 初始化功能开关
		_features = _ctx.CustomFeatures;
		// 设置默认功能
		if (_features.find("is_demo_user") == _features.end()) _features["is_demo_user"] = false;
		if (_features.find("has_custom_resolution") == _features.end()) _features["has_custom_resolution"] = true;
	}

	/**
	 * @brief 执行规划，生成启动所需的命令行信息
	 * @return 进程启动信息
	 */
	ProcessStartInfo LaunchPlanner::Plan() {
		ProcessStartInfo info;
		info.Executable = _ctx.JavaPath;
		info.WorkingDirectory = _ctx.GameRoot; // 游戏通常在 .minecraft 目录下运行

		// 构建 Classpath
		std::string cp = BuildClasspath();

		// 构建 JVM 参数
		auto jvmArgs = BuildJvmArgs(cp);
		info.Arguments.insert(info.Arguments.end(), jvmArgs.begin(), jvmArgs.end());

		// 添加主类
		info.Arguments.push_back(_version.MainClass);

		// 构建游戏参数
		auto gameArgs = BuildGameArgs();
		info.Arguments.insert(info.Arguments.end(), gameArgs.begin(), gameArgs.end());

		return info;
	}

	/**
	 * @brief 构建 Classpath 字符串
	 * @return 完整的 Classpath 字符串，以分号分隔
	 */
	std::string LaunchPlanner::BuildClasspath() {
		std::string cp;
		std::string separator = ";"; 

		auto librariesDir = _ctx.GameRoot / "libraries";

		// 处理依赖库
		const auto &libsJson = _version.RawData["libraries"];
		for (const auto &j : libsJson) {
			auto lib = Version::Library::Parse(j);

			// 仅考虑当前环境下激活的库
			if (!lib.IsActive(_features)) continue;

			// Classpath 中不包含 Native 库（它们由 java.library.path 处理）
			if (lib.IsNative()) continue;

			// 获取库文件路径
			auto fileInfo = lib.GetApplicableFile(_features);
			std::filesystem::path libPath;

			if (fileInfo && !fileInfo->Path.empty()) {
				libPath = librariesDir / fileInfo->Path;
			} else {
				// 如果没有显式路径，则根据 Maven 坐标构造
				libPath = librariesDir / MavenUtils::GetPath(lib.Name);
			}

			if (!cp.empty()) cp += separator;
			cp += libPath.string();
		}

		// 添加 Minecraft 核心 Jar 文件
		std::filesystem::path clientJar;
		if (!_version.Jar.empty()) {
			clientJar = _ctx.GameRoot / "versions" / _version.Jar / (_version.Jar + ".jar");
		} else {
			clientJar = _version.RootPath / (_version.Jar + ".jar");
		}

		if (!cp.empty()) cp += separator;
		cp += clientJar.string();

		return cp;
	}

	/**
	 * @brief 获取参数替换映射表
	 * @return 包含所有预定义变量替换的映射
	 */
	std::map<std::string, std::string> LaunchPlanner::GetSubstitutions() {
		std::map<std::string, std::string> subs;

		// 身份认证信息
		subs["auth_player_name"] = _ctx.Auth.PlayerName;
		subs["auth_uuid"] = _ctx.Auth.Uuid;
		subs["auth_access_token"] = _ctx.Auth.AccessToken;
		subs["user_type"] = _ctx.Auth.UserType;

		// 版本信息
		subs["version_name"] = _version.Id;
		subs["version_type"] = _version.Type;
		subs["assets_index_name"] = _version.AssetsIndex;

		// 路径信息
		subs["game_directory"] = _ctx.GameRoot.string();
		subs["assets_root"] = (_ctx.GameRoot / "assets").string();
		subs["natives_directory"] = _ctx.NativesDir.string();
		subs["launcher_name"] = "PCL2-CE-CPP";
		subs["launcher_version"] = "0.0.1";

		// 分辨率设置
		subs["resolution_width"] = std::to_string(_ctx.Width);
		subs["resolution_height"] = std::to_string(_ctx.Height);

		return subs;
	}

	/**
	 * @brief 构建 JVM 启动参数
	 * @param classpath 构建好的 Classpath 字符串
	 * @return JVM 参数列表
	 */
	std::vector<std::string> LaunchPlanner::BuildJvmArgs(const std::string &classpath) {
		std::vector<std::string> args;

		// 基础 JVM 参数（内存设置）
		args.push_back("-Xmx" + std::to_string(_ctx.MaxMemoryMb) + "m");

		// 处理版本特定的 JVM 参数
		if (_version.RawData.contains("arguments") && _version.RawData["arguments"].contains("jvm")) {
			auto argParser = Version::Arguments::Parse(_version.RawData["arguments"]);
			auto subs = GetSubstitutions();
			subs["classpath"] = classpath; // 注入 Classpath 变量

			auto dynamicArgs = argParser.GetJvmArgs(subs, _features);
			args.insert(args.end(), dynamicArgs.begin(), dynamicArgs.end());
		} else {
			// 兼容旧版（通常是 1.13 以下版本）
			// 如果 arguments.jvm 缺失，必须提供默认的旧版参数
			args.push_back("-Djava.library.path=" + _ctx.NativesDir.string());
			args.push_back("-cp");
			args.push_back(classpath);
		}

		return args;
	}

	/**
	 * @brief 构建游戏启动参数
	 * @return 游戏参数列表
	 */
	std::vector<std::string> LaunchPlanner::BuildGameArgs() {
		auto subs = GetSubstitutions();
		std::vector<std::string> args;

		if (_version.RawData.contains("arguments") && _version.RawData["arguments"].contains("game")) {
			// 现代版本 (1.13+)
			auto argParser = Version::Arguments::Parse(_version.RawData["arguments"]);
			args = argParser.GetGameArgs(subs, _features);
		} else if (_version.RawData.contains("minecraftArguments")) {
			// 旧版 (1.7.10 - 1.12.2)
			// 使用简单的空格分割并手动替换变量
			std::string raw = _version.RawData["minecraftArguments"].get<std::string>();

			std::stringstream ss(raw);
			std::string segment;
			while (std::getline(ss, segment, ' ')) {
				if (!segment.empty()) {
					for (const auto &[key, val] : subs) {
						std::string placeholder = "${" + key + "}";
						size_t pos = 0;
						while ((pos = segment.find(placeholder, pos)) != std::string::npos) {
							segment.replace(pos, placeholder.length(), val);
							pos += val.length();
						}
					}
					args.push_back(segment);
				}
			}
		}

		return args;
	}

	/**
	 * @brief 提取当前版本所需的 Native 库
	 * @return 是否全部提取成功
	 */
    bool LaunchPlanner::ExtractNatives() {
        const auto& libsJson = _version.RawData["libraries"];
        auto librariesDir = _ctx.GameRoot / "libraries";

        for (const auto& j : libsJson) {
            auto lib = Version::Library::Parse(j);
            
            if (!lib.IsActive(_features)) continue;
            if (!lib.IsNative()) continue;

            auto fileInfo = lib.GetApplicableFile(_features);
            std::filesystem::path jarPath;

            if (fileInfo && !fileInfo->Path.empty()) {
                jarPath = librariesDir / fileInfo->Path;
            } else {
                // 如果 Native 路径缺失，尝试回退到默认路径构造
                jarPath = librariesDir / MavenUtils::GetPath(lib.Name, "jar", ""); 
            }

            // 处理提取排除规则
            std::vector<std::string> excludes;
            if (lib.Extract.has_value()) {
                excludes = lib.Extract->Exclude;
            }

            if (!NativesUtils::Extract(jarPath, _ctx.NativesDir, excludes)) {
                LOG_WARNING("Failed to extract native library: {}", lib.Name);
                // 这里暂时不中断流程，尝试继续启动
            }
        }
        return true;
    }
}
