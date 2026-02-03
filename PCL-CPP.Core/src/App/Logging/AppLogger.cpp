#include "pch.h"
#include "AppLogger.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <windows.h>

namespace PCL_CPP::Core::Logging {

	/**
	 * @brief 获取日志管理器单例实例
	 * @return AppLogger 实例引用
	 */
	AppLogger &AppLogger::GetInst() {
		static AppLogger instance;
		return instance;
	}

	/**
	 * @brief 析构函数，确保资源被正确释放
	 */
	AppLogger::~AppLogger()noexcept {
		Shutdown();
	}

	/**
	 * @brief 初始化日志系统
	 * @param logFilePath 日志文件保存路径
	 */
	void AppLogger::Init(const std::filesystem::path &logFilePath) {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_initialized)
			return;

		// 确保目标目录存在
		if (logFilePath.has_parent_path()) {
			std::filesystem::create_directories(logFilePath.parent_path());
		}

		// 以追加模式打开日志文件
		m_logFile.open(logFilePath, std::ios::out | std::ios::app);
		if (m_logFile.is_open()) {
			m_initialized = true;
		} else {
			// 如果文件打开失败，输出到标准错误流（使用宽字符以支持 Windows 控制台）
			std::wcerr << L"[FATAL] Failed to open log file: " << logFilePath.c_str() << std::endl;
		}
	}

	/**
	 * @brief 关闭日志系统
	 */
	void AppLogger::Shutdown() noexcept {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_logFile.is_open()) {
			m_logFile.close();
		}
		m_initialized = false;
	}

	/**
	 * @brief 记录一条日志消息
	 * @param level 日志级别
	 * @param message 消息内容
	 * @param location 调用发生的源码位置
	 */
	void AppLogger::Log(LogLevel level, std::string_view message, const std::source_location &location) {
		std::filesystem::path sourcePath(location.file_name());
		
		// 构造日志条目格式：[时间戳] [线程哈希] [级别] [文件名:行号] 消息
		std::string logEntry = std::format(
			"[{}] [{}] [{}] [{}:{}] {}\n",
			GetTimestamp(),
			std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10000,
			LevelToString(level),
			sourcePath.filename().string(),
			location.line(),
			message
		);
		
		std::lock_guard<std::mutex> lock(m_mutex);

		// 调试模式下同时输出到控制台
	#ifdef _DEBUG
		WriteToConsole(level, logEntry);
	#endif

		// 如果初始化成功，则写入文件
		if (m_initialized) {
			WriteToFile(logEntry);
		}
	}

	/**
	 * @brief 将日志写入文件
	 * @param logEntry 格式化后的日志条目
	 */
	void AppLogger::WriteToFile(std::string_view logEntry)noexcept {
		if (m_logFile.is_open()) {
			m_logFile << logEntry;
			m_logFile.flush();
		}
	}

	/**
	 * @brief 将日志输出到控制台
	 * @param level 日志级别（用于着色）
	 * @param logEntry 格式化后的日志条目
	 */
	void AppLogger::WriteToConsole(LogLevel level, std::string_view logEntry)const noexcept {
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		WORD originalAttrs = consoleInfo.wAttributes;

		// 根据级别设置前景色
		switch (level) {
			case LogLevel::Trace:
				SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
				break;
			case LogLevel::Debug:
				SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				break;
			case LogLevel::Info:
				SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				break;
			case LogLevel::Warning:
				SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				break;
			case LogLevel::Error:
				SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
				break;
			case LogLevel::Fatal:
				SetConsoleTextAttribute(hConsole, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
				break;
		}

		// 将 UTF-8 日志条目转换为宽字符，以便在 Windows 控制台正确显示
		if (!logEntry.empty()) {
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, logEntry.data(), (int) logEntry.size(), NULL, 0);
			std::wstring wLogEntry(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, logEntry.data(), (int) logEntry.size(), &wLogEntry[0], size_needed);
			WriteConsoleW(hConsole, wLogEntry.data(), (DWORD) wLogEntry.size(), NULL, NULL);
		}

		// 恢复原始控制台属性
		SetConsoleTextAttribute(hConsole, originalAttrs);
	}

	/**
	 * @brief 获取当前格式化的时间戳
	 * @return 时间戳字符串 (格式: YYYY-MM-DD HH:MM:SS.mmm)
	 */
	std::string AppLogger::GetTimestamp() {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		time_t time = std::chrono::system_clock::to_time_t(now);
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		std::tm tm;
		localtime_s(&tm, &time);

		return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
						   tm.tm_year + 1900,
						   tm.tm_mon + 1,
						   tm.tm_mday,
						   tm.tm_hour,
						   tm.tm_min,
						   tm.tm_sec,
						   ms.count());
	}

	/**
	 * @brief 将日志级别转换为字符串
	 * @param level 日志级别
	 * @return 对应的字符串表示
	 */
	constexpr std::string_view AppLogger::LevelToString(LogLevel level)  noexcept {
		switch (level) {
			case LogLevel::Trace:   return "TRACE";
			case LogLevel::Debug:   return "DEBUG";
			case LogLevel::Info:    return "INFO ";
			case LogLevel::Warning: return "WARN ";
			case LogLevel::Error:   return "ERROR";
			case LogLevel::Fatal:   return "FATAL";
			default:                return "UNKNO";
		}
	}
}
