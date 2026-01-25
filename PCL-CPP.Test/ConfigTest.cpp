#include "pch.h"
#include "CppUnitTest.h"
#include "App/Config/ConfigManager.h"
#include <filesystem>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Config;

namespace PCLCPPTest {
	TEST_CLASS(ConfigTest) {
	public:
	TEST_METHOD(TestDefaultTemplateCreation) {
		std::filesystem::path configRoot = "TestConfigs_Defaults";
		if (std::filesystem::exists(configRoot)) std::filesystem::remove_all(configRoot);

		// 初始化，应该自动创建 Template.json
		ConfigManager::GetInst().Init(configRoot);

		Assert::IsTrue(std::filesystem::exists(configRoot / "Template.json"), L"Template.json should be created.");

		// 验证默认值
		auto tmpl = ConfigManager::GetInst().GetTemplate();
		std::string lang = tmpl->Get<std::string>("General/Language", "unknown");
		Assert::AreEqual(std::string("zh-CN"), lang, L"Default language should be zh-CN");

		int ver = tmpl->Get<int>("Version", -1);
		Assert::AreEqual(0, ver, L"Default version should be 0");
	}

	TEST_METHOD(TestProfileLoadingAndDiff) {
		std::filesystem::path configRoot = "TestConfigs_Profiles";
		if (std::filesystem::exists(configRoot)) std::filesystem::remove_all(configRoot);

		auto &mgr = ConfigManager::GetInst();
		mgr.Init(configRoot);

		// 加载一个新 Profile
		mgr.LoadProfile("UserProfile1");
		auto profile = mgr.GetActiveProfile();

		// 修改一个值
		profile->Set<std::string>("General/Language", "en-US");

		// 保存
		mgr.SaveActiveProfile();

		// 验证文件存在
		std::filesystem::path profilePath = configRoot / "UserProfile1.json";
		Assert::IsTrue(std::filesystem::exists(profilePath));

		// 验证文件内容只包含差异 (en-US)
		std::ifstream file(profilePath);
		nlohmann::json savedJson;
		file >> savedJson;

		// 应该包含修改过的项
		Assert::IsTrue(savedJson["General"].contains("Language"));
		Assert::AreEqual(std::string("en-US"), savedJson["General"]["Language"].get<std::string>());

		// 不应该包含未修改的项 (如 Version, ProfileName 等，如果它们在默认值里有的话)
		// 理论上 diff 不应该包含 Version，除非它是根节点。
		Assert::IsFalse(savedJson.contains("Version"), L"Unchanged root key should not be in diff file.");
	}

	TEST_METHOD(TestTemplatePatching) {
		std::filesystem::path configRoot = "TestConfigs_Patch";
		if (std::filesystem::exists(configRoot)) std::filesystem::remove_all(configRoot);
		std::filesystem::create_directories(configRoot);

		// 手动创建一个残缺的 Template.json
		{
			nlohmann::json brokenTemplate = {
				{"Version", 0},
				// 缺少 General/Language
				{"ProfileName", "Broken Profile"}
			};
			std::ofstream file(configRoot / "Template.json");
			file << brokenTemplate.dump();
		}

		// 初始化，应该自动修复
		ConfigManager::GetInst().Init(configRoot);

		auto tmpl = ConfigManager::GetInst().GetTemplate();

		// 验证缺失的键是否补全
		std::string lang = tmpl->Get<std::string>("General/Language", "missing");
		Assert::AreEqual(std::string("zh-CN"), lang, L"Missing key should be patched.");

		// 验证已有的键是否保留
		std::string name = tmpl->Get<std::string>("ProfileName", "missing");
		Assert::AreEqual(std::string("Broken Profile"), name, L"Existing key should be preserved.");
	}
	};
}
