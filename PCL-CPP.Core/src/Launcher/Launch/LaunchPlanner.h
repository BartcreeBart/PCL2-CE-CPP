#pragma once
#include "Launcher/Version/VersionLocator.h"
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Launch {

	/**
	 * @brief 启动身份认证信息
	 */
	struct LaunchAuth {
		std::string PlayerName = "Steve"; ///< 玩家名称
		std::string Uuid = "00000000-0000-0000-0000-000000000000"; ///< 玩家 UUID
		std::string AccessToken = "00000000000000000000000000000000"; ///< 访问令牌
		std::string UserType = "Legacy"; ///< 用户类型 (Legacy, Mojang, MSA)
	};

	/**
	 * @brief 启动上下文配置
	 */
	struct LaunchContext {
		// Java 配置
		std::filesystem::path JavaPath = "javaw.exe"; ///< Java 可执行文件路径
		int MaxMemoryMb = 2048; ///< 最大内存 (MB)
		int MinMemoryMb = 512; ///< 最小内存 (MB)

		// 身份认证
		LaunchAuth Auth; ///< 认证信息

		// 窗口配置
		int Width = 854; ///< 窗口宽度
		int Height = 480; ///< 窗口高度
		bool Fullscreen = false; ///< 是否全屏

		// 路径配置
		std::filesystem::path GameRoot; ///< 游戏根目录 (.minecraft 目录)
		std::filesystem::path NativesDir; ///< Natives 库提取目录

		// 功能覆盖
		std::map<std::string, bool> CustomFeatures; ///< 自定义功能开关覆盖
	};

	/**
	 * @brief 进程启动信息
	 */
	struct ProcessStartInfo {
		std::filesystem::path Executable; ///< 可执行文件路径
		std::vector<std::string> Arguments; ///< 启动参数列表
		std::filesystem::path WorkingDirectory; ///< 工作目录

		/**
		 * @brief 获取完整的命令行字符串
		 * @return 命令行字符串
		 */
		std::string ToCommandLine() const {
			std::string cmd = Executable.string();
			for (const auto &arg : Arguments) {
				cmd += " ";
				if (arg.find(' ') != std::string::npos) {
					cmd += "\"" + arg + "\"";
				} else {
					cmd += arg;
				}
			}
			return cmd;
		}
	};

	/**
	 * @brief Maven 标识符工具类
	 */
	class MavenUtils {
		public:
		/**
		 * @brief 将 Maven 标识符转换为文件路径
		 * 
		 * 例如: "com.google.guava:guava:31.0" -> "com/google/guava/guava/31.0/guava-31.0.jar"
		 * 
		 * @param mavenId Maven 标识符
		 * @param extension 文件扩展名 (默认为 jar)
		 * @param classifier 分类器 (可选)
		 * @return 对应的相对文件路径
		 */
		static std::filesystem::path GetPath(const std::string &mavenId, const std::string &extension = "jar", const std::string &classifier = "") {
			std::vector<std::string> parts;
			std::stringstream ss(mavenId);
			std::string item;
			while (std::getline(ss, item, ':')) {
				parts.push_back(item);
			}

			if (parts.size() < 3) return {};

			std::string group = parts[0];
			std::string artifact = parts[1];
			std::string version = parts[2];

			std::replace(group.begin(), group.end(), '.', '/');

			std::string filename = artifact + "-" + version;
			if (!classifier.empty()) {
				filename += "-" + classifier;
			}
			filename += "." + extension;

			return std::filesystem::path(group) / artifact / version / filename;
		}
	};

	/**
	 * @brief 启动规划器类，负责构建启动命令和准备运行环境。
	 * 
	 * @details 
	 * 该类的核心逻辑包括：
	 * 1. **Classpath 构建**：遍历所有依赖库，根据当前系统环境（OS、架构）筛选激活的库，并拼接成完整的 Classpath 字符串。
	 * 2. **参数构建**：支持现代（1.13+，基于 Arguments 对象）和旧版（1.12.2-，基于 minecraftArguments 字符串）两种参数解析方式。
	 * 3. **变量替换**：建立一套占位符映射表（如 `${auth_player_name}`、`${game_directory}`），在生成最终参数时进行全局替换。
	 * 4. **环境准备**：在启动前自动处理 Natives 动态库的提取，确保 Java 能够加载到必要的系统依赖。
	 */
	class LaunchPlanner {
    public:
		/**
		 * @brief 构造函数
		 * @param version 版本信息
		 * @param ctx 启动上下文
		 */
        LaunchPlanner(const Version::VersionInfo &version, const LaunchContext &ctx);

		/**
		 * @brief 执行规划，生成启动信息
		 * @details 
		 * 按照 Java 启动流程，依次构建工作目录、Classpath、JVM 参数、主类和游戏参数。
		 * @return 进程启动信息，包含可执行文件路径及完整参数列表。
		 */
        ProcessStartInfo Plan();
        
		/**
		 * @brief 提取 Natives 动态链接库
		 * @details 
		 * 扫描版本配置中的所有库，筛选出标记为 native 的库，并调用 NativesUtils 将其内部的 .dll 文件提取到指定的临时目录。
		 * @return 是否提取成功
		 */
        bool ExtractNatives();

    private:
        Version::VersionInfo _version; ///< 版本信息
        LaunchContext _ctx; ///< 启动上下文
        std::map<std::string, bool> _features; ///< 生效的功能列表

		/**
		 * @brief 构建 Classpath 字符串
		 * @details 
		 * 实现细节：
		 * - 优先从版本配置的 `libraries` 数组中解析。
		 * - 对于每个库，通过 `Library::IsActive` 检查其规则（Rule）是否匹配当前系统。
		 * - 如果库有 `path` 则直接使用，否则通过 Maven 坐标推导路径。
		 * - 最后将游戏核心 Jar 包追加到末尾。
		 * @return 完整的 Classpath 字符串，以分号分隔
		 */
        std::string BuildClasspath();

		/**
		 * @brief 构建 JVM 启动参数
		 * @details 
		 * 实现细节：
		 * - 注入基础参数（如内存限制 `-Xmx`）。
		 * - 处理 `arguments.jvm` 中的现代参数，支持条件判断（如根据是否为 OSX 启用特定参数）。
		 * - 兼容旧版逻辑，手动注入 `java.library.path` 和 `-cp`。
		 * @param classpath 构建好的 Classpath 字符串
		 * @return JVM 参数列表
		 */
        std::vector<std::string> BuildJvmArgs(const std::string &classpath);

		/**
		 * @brief 构建游戏启动参数
		 * @details 
		 * 实现细节：
		 * - 对于 1.13+ 版本，解析 `arguments.game` 中的复杂参数项。
		 * - 对于 1.12.2 及以下版本，解析 `minecraftArguments` 字符串。
		 * - 所有参数在返回前都会经过 `GetSubstitutions` 映射表进行占位符替换。
		 * @return 游戏参数列表
		 */
        std::vector<std::string> BuildGameArgs();

		/**
		 * @brief 获取参数替换映射表
		 * @details 
		 * 汇总认证信息、路径信息、版本信息以及分辨率等数据，形成 `${key}` 到 `value` 的映射。
		 * @return 替换映射表
		 */
        std::map<std::string, std::string> GetSubstitutions();
    };
}
