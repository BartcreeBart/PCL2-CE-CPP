#include "pch.h"
#include "AppLogger.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <windows.h>

namespace PCL_CPP::Core::Logging {

	AppLogger &AppLogger::GetInst() {
		static AppLogger instance;
		return instance;
	}

	AppLogger::~AppLogger()noexcept {
		// 析构函数不应抛出异常:-)   那Shutdown会抛出异常么？？（算了看到这条注释的人修吧）
		Shutdown();
	}

	void AppLogger::Init(const std::filesystem::path &logFilePath) {
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_initialized)
			return;

		// 确保目录存在
		if (logFilePath.has_parent_path()) {
			std::filesystem::create_directories(logFilePath.parent_path());
		}

		// 打开文件
		m_logFile.open(logFilePath, std::ios::out | std::ios::app);
		if (m_logFile.is_open()) {
			m_initialized = true;
		} else {
			// 如果文件打开失败，输出到标准错误 (宽字符防止输出乱码)
			std::wcerr << L"[FATAL] Failed to open log file: " << logFilePath.c_str() << std::endl;
		}
	}

	void AppLogger::Shutdown() noexcept {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_logFile.is_open()) {
			m_logFile.close();
		}
		m_initialized = false;
	}

	void AppLogger::Log(LogLevel level, std::string_view message, const std::source_location &location) {
		// 吃饭
		std::filesystem::path sourcePath(location.file_name());
		// 消化
		std::string logEntry = std::format(
			"[{}] [{}] [{}] [{}:{}] {}\n",
			GetTimestamp(),
			std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10000,
			LevelToString(level),
			sourcePath.filename().string(),
			location.line(),
			message
		);
		std::lock_guard<std::mutex> lock(m_mutex);// 锁门

		// 控制台 (Debug模式)
	#ifdef _DEBUG
		WriteToConsole(level, logEntry);
	#endif

		// 文件 (始终写入)
		if (m_initialized) {
			WriteToFile(logEntry);
		}
	}

	void AppLogger::WriteToFile(std::string_view logEntry)noexcept {
		if (m_logFile.is_open()) {
			// 冲马桶
			m_logFile << logEntry;
			m_logFile.flush();
		}
	}

	void AppLogger::WriteToConsole(LogLevel level, std::string_view logEntry)const noexcept {
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
		WORD originalAttrs = consoleInfo.wAttributes;

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
				SetConsoleTextAttribute(hConsole, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);// 红底白（你觉得不是就不是吧）字
				break;
		}

		// 防止输出乱码
		if (!logEntry.empty()) {
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, logEntry.data(), (int) logEntry.size(), NULL, 0);
			std::wstring wLogEntry(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, logEntry.data(), (int) logEntry.size(), &wLogEntry[0], size_needed);
			WriteConsoleW(hConsole, wLogEntry.data(), (DWORD) wLogEntry.size(), NULL, NULL);
		}

		SetConsoleTextAttribute(hConsole, originalAttrs);
	}

	std::string AppLogger::GetTimestamp() {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		time_t time = std::chrono::system_clock::to_time_t(now);
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		std::tm tm;
		localtime_s(&tm, &time);

		// 使用 snprintf/stringstream 或者手动拼接，比 std::format 处理 time_t 更稳健(是么？)
		// 但既然引入了 std::format，我们可以利用它格式化整数的特性（据说这样用开销最小？）
		return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
						   tm.tm_year + 1900,
						   tm.tm_mon + 1,
						   tm.tm_mday,
						   tm.tm_hour,
						   tm.tm_min,
						   tm.tm_sec,
						   ms.count());
	}
	constexpr std::string_view AppLogger::LevelToString(LogLevel level)  noexcept {//为什么这里不用加static？
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
