#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>

namespace PCL_CPP::Core::Logging {
	/**
	 * @brief 日志级别枚举
	 */
	enum class LogLevel: uint8_t {
		Trace = 0,   ///< 追踪：最详细的调试信息
		Debug = 1,   ///< 调试：常规调试信息
		Info = 2,    ///< 信息：程序运行中的关键节点信息
		Warning = 3, ///< 警告：可能导致问题的非致命性错误
		Error = 4,   ///< 错误：影响功能的严重问题
		Fatal = 5    ///< 致命：导致程序无法继续运行的错误
	};

	/**
	 * @brief 应用程序日志管理类
	 * 
	 * @details 
	 * 该类实现了高性能且线程安全的全局日志系统：
	 * 1. **迈耶斯单例 (Meyers' Singleton)**：确保全局唯一的日志实例，且在首次访问时自动初始化。
	 * 2. **多通道输出**：
	 *    - **文件通道**：始终记录所有级别的日志，便于事后排查。
	 *    - **控制台通道**：仅在 `_DEBUG` 模式下启用，支持根据日志级别自动着色。
	 * 3. **线程安全**：内部通过 `std::mutex` 互斥锁同步，支持多个线程并发打印日志。
	 * 4. **上下文感知**：利用 `std::source_location` 自动捕获日志调用的文件名、函数名和行号。
	 * 5. **宽字符处理**：在输出到 Windows 控制台时，自动处理 UTF-8 到宽字符（UTF-16）的转换，避免乱码。
	 */
	class AppLogger {
		public:
		/**
		 * @brief 获取日志管理器单例实例
		 * @return AppLogger 实例引用
		 */
		static AppLogger &GetInst();

		// 禁止拷贝构造和赋值操作
		AppLogger(const AppLogger &) = delete;
		AppLogger &operator=(const AppLogger &) = delete;

		/**
		 * @brief 初始化日志系统
		 * @details 
		 * 负责创建必要的日志目录并以追加模式（Append Mode）打开日志文件。
		 * @param logFilePath 日志文件保存路径
		 */
		void Init(const std::filesystem::path &logFilePath);

		/**
		 * @brief 关闭日志系统
		 */
		void Shutdown() noexcept;

		/**
		 * @brief 记录一条日志消息
		 * @details 
		 * 实现逻辑：
		 * 1. 格式化日志头部（时间戳、线程 ID、级别、位置）。
		 * 2. 加锁后依次调用文件写入和控制台写入逻辑。
		 * @param level 日志级别
		 * @param message 日志消息内容
		 * @param location 调用发生的源码位置
		 */
		void Log(LogLevel level, std::string_view message,
				 const std::source_location &location = std::source_location::current());

		/**
		 * @brief 格式化并记录日志消息
		 * @tparam Args 格式化参数类型
		 * @param level 日志级别
		 * @param location 调用发生的源码位置
		 * @param fmt 格式化字符串
		 * @param args 格式化参数
		 */
		template <typename... Args>
		void FLog(LogLevel level, const std::source_location &location, std::format_string<Args...> fmt,
				  Args &&...args) {
			try {
				std::string message = std::format(fmt, std::forward<Args>(args)...);
				Log(level, message, location);
			} catch (const std::exception &e) {
				Log(LogLevel::Error, std::format("Log formatting error: {}", e.what()), location);
			}
		}

		private:
		AppLogger() = default; ///< 私有构造函数，实现单例
		~AppLogger() noexcept; ///< 私有析构函数

		/**
		 * @brief 将日志写入文件
		 * @param logEntry 格式化后的日志条目
		 */
		void WriteToFile(std::string_view logEntry) noexcept;

		/**
		 * @brief 将日志输出到控制台
		 * @details 
		 * 使用 Win32 API `SetConsoleTextAttribute` 实现着色，并通过 `WriteConsoleW` 处理宽字符输出。
		 * @param level 日志级别（用于着色）
		 * @param logEntry 格式化后的日志条目
		 */
		void WriteToConsole(LogLevel level, std::string_view logEntry) const noexcept;

		/**
		 * @brief 获取当前格式化的时间戳
		 * @return 时间戳字符串 (格式: YYYY-MM-DD HH:MM:SS.mmm)
		 */
		static std::string GetTimestamp();

		/**
		 * @brief 将日志级别转换为字符串
		 * @param level 日志级别
		 * @return 对应的字符串表示
		 */
		static constexpr std::string_view LevelToString(LogLevel level) noexcept;

		std::mutex m_mutex; ///< 保证日志写入线程安全的互斥锁
		std::ofstream m_logFile; ///< 日志文件流
		bool m_initialized = false; ///< 日志系统是否已初始化的标志
	};
}

// 辅助宏
#define LOG_TRACE(...)                                                                                                 \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Trace,                           \
                                                        std::source_location::current(), __VA_ARGS__)
#define LOG_DEBUG(...)                                                                                                 \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Debug,                           \
                                                        std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...)                                                                                                  \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Info,                            \
                                                        std::source_location::current(), __VA_ARGS__)
#define LOG_WARNING(...)                                                                                               \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Warning,                         \
                                                        std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...)                                                                                                 \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Error,                           \
                                                        std::source_location::current(), __VA_ARGS__)
#define LOG_FATAL(...)                                                                                                 \
    PCL_CPP::Core::Logging::AppLogger::GetInst().FLog(PCL_CPP::Core::Logging::LogLevel::Fatal,                           \
                                                        std::source_location::current(), __VA_ARGS__)
