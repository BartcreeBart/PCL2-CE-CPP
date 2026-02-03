#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "Launcher/Version/Arguments.h"
#include "Launcher/Version/Library.h"
#include "Launcher/Version/VersionLocator.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Launcher::Version;
using namespace PCL_CPP::Core::Logging;

namespace PCLCPPTest {
	TEST_CLASS(VersionTest) {
	public:
	std::filesystem::path testRoot = "TestVersions";

	TEST_METHOD_INITIALIZE(Setup) {
		if (std::filesystem::exists(testRoot)) std::filesystem::remove_all(testRoot);
		std::filesystem::create_directories(testRoot);
		std::filesystem::path assetsDir = TEST_ASSETS_DIR;

		// 1.18.2 (Vanilla)
		std::filesystem::create_directories(testRoot / "1.18.2");
		std::filesystem::copy_file(assetsDir / "1.18.2.json", testRoot / "1.18.2/1.18.2.json");

		// 1.18.2-OptiFine
		std::filesystem::create_directories(testRoot / "1.18.2-OptiFine");
		std::filesystem::copy_file(assetsDir / "1.18.2-OptiFine.json", testRoot / "1.18.2-OptiFine/1.18.2-OptiFine.json");
	}

	TEST_METHOD(TestDiscovery) {
		auto versions = VersionLocator::GetAllVersions(testRoot);
		Assert::AreEqual((size_t) 2, versions.size(), L"Should find 2 versions from Assets");

		bool foundVanilla = false;
		bool foundOptiFine = false;

		for (const auto &v : versions) {
			if (v.Id == "1.18.2") foundVanilla = true;
			if (v.Id == "1.18.2-OptiFine") foundOptiFine = true;
		}

		Assert::IsTrue(foundVanilla, L"Vanilla version not found");
		Assert::IsTrue(foundOptiFine, L"OptiFine version not found");
	}

	TEST_METHOD(TestRealVersionParsing) {
		// Test 1.18.2-OptiFine parsing
		auto optifine = VersionLocator::GetVersion(testRoot, "1.18.2-OptiFine");
		Assert::IsTrue(optifine.has_value(), L"GetVersion failed for OptiFine");

		// Verify basic fields
		Assert::AreEqual(std::string("1.18.2-OptiFine"), optifine->Id);
		Assert::AreEqual(std::string("net.minecraft.launchwrapper.Launch"), optifine->MainClass);
		Assert::AreEqual(std::string("release"), optifine->Type);

		// Verify libraries
		const auto &libs = optifine->RawData["libraries"];
		Assert::IsTrue(libs.size() > 10, L"Should have many libraries");

		// Verify arguments
		const auto &gameArgs = optifine->RawData["arguments"]["game"];
		Assert::IsTrue(gameArgs.size() > 0, L"Should have game arguments");
	}

	TEST_METHOD(TestInheritanceLogic_Synthetic) {
		std::filesystem::path inheritRoot = testRoot / "InheritanceTest";
		std::filesystem::create_directories(inheritRoot);

		// Parent
		std::filesystem::create_directories(inheritRoot / "Parent");
		nlohmann::json parent = {
			{"id", "Parent"},
			{"mainClass", "ParentMain"},
			{"libraries", { {{"name", "libP"}} }}
		};
		std::ofstream(inheritRoot / "Parent/Parent.json") << parent.dump();

		// Child
		std::filesystem::create_directories(inheritRoot / "Child");
		nlohmann::json child = {
			{"id", "Child"},
			{"inheritsFrom", "Parent"},
			{"libraries", { {{"name", "libC"}} }}
		};
		std::ofstream(inheritRoot / "Child/Child.json") << child.dump();

		auto v = VersionLocator::GetVersion(inheritRoot, "Child");
		Assert::IsTrue(v.has_value());
		Assert::AreEqual(std::string("ParentMain"), v->MainClass, L"Should inherit MainClass");
		Assert::AreEqual((size_t) 2, v->RawData["libraries"].size(), L"Should merge libraries");
	}

	TEST_METHOD(TestLibraryFiltering) {
		auto optifine = VersionLocator::GetVersion(testRoot, "1.18.2-OptiFine");
		Assert::IsTrue(optifine.has_value());

		std::map<std::string, bool> features;

		int activeCount = 0;
		bool foundWindowsNative = false;

		const auto &libs = optifine->RawData["libraries"];
		for (const auto &libJson : libs) {
			Library lib = Library::Parse(libJson);
			if (lib.IsActive(features)) {
				activeCount++;

				if (lib.IsNative()) {
					// Check if it's windows native
					if (lib.Natives.count("windows")) {
						foundWindowsNative = true;
						auto file = lib.GetApplicableFile(features);
						Assert::IsTrue(file.has_value(), L"Native file should be resolvable");
					}
				}
			}
		}

		Assert::IsTrue(activeCount > 0, L"Should have active libraries");
		Assert::IsTrue(foundWindowsNative, L"Should find windows natives");
	}

	TEST_METHOD(TestArgumentParsing) {
		auto optifine = VersionLocator::GetVersion(testRoot, "1.18.2-OptiFine");
		Assert::IsTrue(optifine.has_value());

		Arguments args = Arguments::Parse(optifine->RawData["arguments"]);

		// Features override
		std::map<std::string, bool> features;
		features["has_custom_resolution"] = true;

		std::map<std::string, std::string> subs;
		subs["version_name"] = "1.18.2-OptiFine";
		subs["auth_player_name"] = "Player";

		auto gameArgs = args.GetGameArgs(subs, features);

		// Check for --version 1.18.2-OptiFine
		bool foundVer = false;
		for (size_t i = 0; i < gameArgs.size(); i++) {
			if (gameArgs[i] == "--version" && i + 1 < gameArgs.size()) {
				if (gameArgs[i + 1] == "1.18.2-OptiFine") foundVer = true;
			}
		}
		Assert::IsTrue(foundVer, L"Should find substituted version argument");

		// Check for resolution (enabled by feature)
		bool foundRes = false;
		for (const auto &arg : gameArgs) {
			if (arg == "--width") foundRes = true;
		}
		Assert::IsTrue(foundRes, L"Should find resolution argument due to feature enabled");
	}
	};
}
