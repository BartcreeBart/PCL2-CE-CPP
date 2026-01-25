#include "pch.h"
#include "CppUnitTest.h"
#include "App/Logging/AppLogger.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Logging;

namespace PCLCPPTest {
	TEST_CLASS(LoggerTest) {
	public:

	TEST_METHOD(TestLogInitialization) {
		// 准备测试文件路径
		std::filesystem::path logPath = "TestLogs/test_init.log";
		if (std::filesystem::exists(logPath)) {
			std::filesystem::remove(logPath);
		}

		// 初始化日志
		auto &logger = AppLogger::GetInst();
		logger.Init(logPath);

		// 验证文件是否创建
		LOG_INFO("Initialization test");

		// 关闭日志以刷新到磁盘
		logger.Shutdown();

		Assert::IsTrue(std::filesystem::exists(logPath), L"Log file should exist after initialization and writing.");
	}

	TEST_METHOD(TestLogContent) {
		std::filesystem::path logPath = "TestLogs/test_content.log";
		if (std::filesystem::exists(logPath)) {
			std::filesystem::remove(logPath);
		}

		auto &logger = AppLogger::GetInst();
		logger.Init(logPath);

		std::string testMsg = "Unique test message 12345";
		LOG_INFO("{}", testMsg);

		logger.Shutdown();

		// 读取文件验证内容 (文件是 UTF-8)
		std::ifstream file(logPath);
		Assert::IsTrue(file.is_open(), L"Could not open log file for reading.");

		std::string line;
		bool found = false;
		// 简单的查找，因为 ASCII 部分在 UTF-8 中是一样的
		std::string searchMsg = "Unique test message 12345";
		while (std::getline(file, line)) {
			if (line.find(searchMsg) != std::string::npos) {
				found = true;
				// 验证日志级别字符串是否存在
				Assert::IsTrue(line.find("[INFO ]") != std::string::npos, L"Log level INFO not found in log line.");
				break;
			}
		}
		file.close();

		Assert::IsTrue(found, L"Test message not found in log file.");
	}

	TEST_METHOD(TestLogLevels) {
		std::filesystem::path logPath = "TestLogs/test_levels.log";
		if (std::filesystem::exists(logPath)) {
			std::filesystem::remove(logPath);
		}

		auto &logger = AppLogger::GetInst();
		logger.Init(logPath);

		LOG_TRACE("Trace msg");
		LOG_DEBUG("Debug msg");
		LOG_INFO("Info msg");
		LOG_WARNING("Warning msg");
		LOG_ERROR("Error msg");
		LOG_FATAL("Fatal msg");

		logger.Shutdown();

		std::ifstream file(logPath);
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		// 验证所有级别都记录了（默认全记录，除非添加了过滤器逻辑）
		Assert::IsTrue(content.find("[TRACE]") != std::string::npos, L"TRACE level missing");
		Assert::IsTrue(content.find("[DEBUG]") != std::string::npos, L"DEBUG level missing");
		Assert::IsTrue(content.find("[INFO ]") != std::string::npos, L"INFO level missing");
		Assert::IsTrue(content.find("[WARN ]") != std::string::npos, L"WARN level missing");
		Assert::IsTrue(content.find("[ERROR]") != std::string::npos, L"ERROR level missing");
		Assert::IsTrue(content.find("[FATAL]") != std::string::npos, L"FATAL level missing");
	}
	};
}
