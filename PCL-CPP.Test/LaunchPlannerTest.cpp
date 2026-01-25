#include "pch.h"
#include "CppUnitTest.h"
#include "Launcher/Launch/LaunchPlanner.h"
#include "Launcher/Version/VersionLocator.h"
#include <filesystem>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Launcher::Version;
using namespace PCL_CPP::Core::Launcher::Launch;

namespace PCLCPPTest {
	TEST_CLASS(LaunchPlannerTest) {
	public:
	std::filesystem::path testRoot = "TestLaunch";

	TEST_METHOD_INITIALIZE(Setup) {
		if (std::filesystem::exists(testRoot)) std::filesystem::remove_all(testRoot);
		std::filesystem::create_directories(testRoot);

		// Use real assets from the test project directory
		std::filesystem::path assetsDir = TEST_ASSETS_DIR;

		// 1.18.2 (Vanilla)
		std::filesystem::create_directories(testRoot / "versions" / "1.18.2");
		std::filesystem::copy_file(assetsDir / "1.18.2.json", testRoot / "versions/1.18.2/1.18.2.json");

		// 1.18.2-OptiFine (Standalone)
		std::filesystem::create_directories(testRoot / "versions" / "1.18.2-OptiFine");
		std::filesystem::copy_file(assetsDir / "1.18.2-OptiFine.json", testRoot / "versions/1.18.2-OptiFine/1.18.2-OptiFine.json");
	}

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
		ProcessStartInfo info = planner.Plan();

		// Executable
		Assert::AreEqual(std::string("C:/Java/bin/javaw.exe"), info.Executable.string());

		// JVM Args
		bool foundXmx = false;
		bool foundCp = false;
		for (const auto &arg : info.Arguments) {
			if (arg == "-Xmx2048m") foundXmx = true;
			// Vanilla 1.18.2 has many libs, check for a common one like oshi-core
			if (arg.find("oshi-core") != std::string::npos) foundCp = true;
		}
		Assert::IsTrue(foundXmx, L"Should have Xmx argument");
		Assert::IsTrue(foundCp, L"Should have library in classpath");

		// Game Args (Vanilla 1.18.2 uses --username)
		bool foundUsername = false;
		for (size_t i = 0; i < info.Arguments.size(); i++) {
			if (info.Arguments[i] == "--username" && i + 1 < info.Arguments.size()) {
				if (info.Arguments[i + 1] == "Steve") foundUsername = true;
			}
		}
		Assert::IsTrue(foundUsername, L"Should have substituted username");
	}

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

		// OptiFine inherits from 1.18.2, so it should have similar args plus OptiFine libs

		// Check Classpath for OptiFine lib (usually just "optifine:OptiFine:1.18.2_HD_U_H7")
		bool foundOptiFineLib = false;
		for (const auto &arg : info.Arguments) {
			// Classpath argument follows -cp
			if (arg.find("OptiFine") != std::string::npos) foundOptiFineLib = true;
		}
		Assert::IsTrue(foundOptiFineLib, L"Should have OptiFine library in classpath");

		// Check Resolution args (enabled by feature)
		bool foundWidth = false;
		for (size_t i = 0; i < info.Arguments.size(); i++) {
			if (info.Arguments[i] == "--width" && i + 1 < info.Arguments.size()) {
				if (info.Arguments[i + 1] == "1280") foundWidth = true;
			}
		}
		Assert::IsTrue(foundWidth, L"Should have resolution arguments");
	}
	};
}
