#pragma once

#include "pch.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>

namespace PCL_CPP::Core::Logging {
	// 日志级别
	enum class LogLevel: uint8_t {
		Trace = 0,  // 脆丝
		Debug = 1,  // 第八个
		Info = 2,   // 银佛
		Warning = 3,// 挖泥
		Error = 4,  // 鹅肉
		Fatal = 5   // 发臭
	};
	class AppLogger {
		public:
		// 迈耶斯单例
		static AppLogger &GetInst();

		// 禁止拷贝和赋值（迈耶斯死星？）
		AppLogger(const AppLogger &) = delete;
		AppLogger &operator=(const AppLogger &) = delete;

		// RAII 红警2设计模式(Resource Acquisition Is Initialization)
		void Init(const std::filesystem::path &logFilePath);

		// 关闭日志系统，实际析构过程
		void Shutdown() noexcept;

		// 打印日志
		void Log(LogLevel level, std::string_view message,
				 const std::source_location &location = std::source_location::current());

		// 格式化打印
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
		AppLogger() = default;// 就放这吧
		~AppLogger() noexcept;// 也放这吧

		void WriteToFile(std::string_view logEntry)noexcept;
		void WriteToConsole(LogLevel level, std::string_view logEntry)const noexcept;
		static std::string GetTimestamp();
		static constexpr std::string_view LevelToString(LogLevel level) noexcept;

		std::mutex m_mutex;
		std::ofstream m_logFile;
		bool m_initialized = false;
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
