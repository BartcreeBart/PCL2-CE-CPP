#include "pch.h"
#include "Launcher/Launch/LaunchPlanner.h"
#include "Launcher/Version/VersionLocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Launcher::Version;
using namespace PCL_CPP::Core::Launcher::Launch;

namespace PCLCPPTest {
	TEST_CLASS(LaunchPlannerTest) {
	public:
	std::filesystem::path testRoot = "TestLaunch";

	/**
	 * @brief 测试初始化：准备测试所需的虚拟目录结构和资源文件
	 */
	TEST_METHOD_INITIALIZE(Setup) {
		if (std::filesystem::exists(testRoot)) std::filesystem::remove_all(testRoot);
		std::filesystem::create_directories(testRoot);

		// 从测试项目目录获取真实的测试资源
		std::filesystem::path assetsDir = TEST_ASSETS_DIR;

		// 准备 1.18.2 (Vanilla) 版本的测试环境
		std::filesystem::create_directories(testRoot / "versions" / "1.18.2");
		std::filesystem::copy_file(assetsDir / "1.18.2.json", testRoot / "versions/1.18.2/1.18.2.json");

		// 准备 1.18.2-OptiFine (继承版本) 版本的测试环境
		std::filesystem::create_directories(testRoot / "versions" / "1.18.2-OptiFine");
		std::filesystem::copy_file(assetsDir / "1.18.2-OptiFine.json", testRoot / "versions/1.18.2-OptiFine/1.18.2-OptiFine.json");
	}

	/**
	 * @brief 测试原版 1.18.2 的启动规划生成
	 */
	TEST_METHOD(TestPlanGeneration_Vanilla) {
        auto version = VersionLocator::GetVersion(testRoot / "versions", "1.18.2");
        Assert::IsTrue(version.has_value());

        LaunchContext ctx;
        ctx.JavaPath = "C:/Java/bin/javaw.exe";
        ctx.GameRoot = testRoot; 
        ctx.NativesDir = testRoot / "natives";
        ctx.Auth.PlayerName = "Steve";
        ctx.MaxMemoryMb = 2048;

        LaunchPlanner planner(*version, ctx);
        
        // 测试 Natives 提取逻辑 (模拟场景 - 虽然缺失真实 Jar，但验证逻辑运行)
        planner.ExtractNatives(); 

        ProcessStartInfo info = planner.Plan();

        // 验证可执行文件路径
        Assert::AreEqual(std::string("C:/Java/bin/javaw.exe"), info.Executable.string());

		// 验证 JVM 参数
		bool foundXmx = false;
		bool foundCp = false;
		for (const auto &arg : info.Arguments) {
			if (arg == "-Xmx2048m") foundXmx = true;
			// 原版 1.18.2 包含多个依赖库，检查 common 库
			if (arg.find("oshi-core") != std::string::npos) foundCp = true;
		}
		Assert::IsTrue(foundXmx, L"应包含 Xmx 参数");
		Assert::IsTrue(foundCp, L"Classpath 应包含依赖库");

		// 验证游戏参数 (原版 1.18.2 使用 --username)
		bool foundUsername = false;
		for (size_t i = 0; i < info.Arguments.size(); i++) {
			if (info.Arguments[i] == "--username" && i + 1 < info.Arguments.size()) {
				if (info.Arguments[i + 1] == "Steve") foundUsername = true;
			}
		}
		Assert::IsTrue(foundUsername, L"用户名应已替换成功");
	}

	/**
	 * @brief 测试继承版 1.18.2-OptiFine 的启动规划生成
	 */
	TEST_METHOD(TestPlanGeneration_OptiFine) {
		auto version = VersionLocator::GetVersion(testRoot / "versions", "1.18.2-OptiFine");
		Assert::IsTrue(version.has_value());

		LaunchContext ctx;
		ctx.JavaPath = "C:/Java/bin/javaw.exe";
		ctx.GameRoot = testRoot;
		ctx.NativesDir = testRoot / "natives";
		ctx.Auth.PlayerName = "Alex";
		ctx.MaxMemoryMb = 4096;
		ctx.CustomFeatures["has_custom_resolution"] = true;
		ctx.Width = 1280;
		ctx.Height = 720;

		LaunchPlanner planner(*version, ctx);
		ProcessStartInfo info = planner.Plan();

		// OptiFine 继承自 1.18.2，应包含父版本库和自身的 OptiFine 库

		// 验证 Classpath 是否包含 OptiFine 库
		bool foundOptiFineLib = false;
		for (const auto &arg : info.Arguments) {
			if (arg.find("OptiFine") != std::string::npos) foundOptiFineLib = true;
		}
		Assert::IsTrue(foundOptiFineLib, L"Classpath 应包含 OptiFine 库");

		// 验证分辨率参数 (由 feature 启用)
		bool foundWidth = false;
		for (size_t i = 0; i < info.Arguments.size(); i++) {
			if (info.Arguments[i] == "--width" && i + 1 < info.Arguments.size()) {
				if (info.Arguments[i + 1] == "1280") foundWidth = true;
			}
		}
		Assert::IsTrue(foundWidth, L"应包含分辨率参数");
	}
	};
}
