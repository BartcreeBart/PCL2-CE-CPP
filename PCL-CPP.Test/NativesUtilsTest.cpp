#include "pch.h"
#include "Launcher/Launch/NativesUtils.h"

#define MINIZ_NO_TIME
#define MINIZ_NO_ZLIB_APIS
#include <miniz/miniz.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PCL_CPP::Core::Launcher::Launch;

namespace PCLCPPTest {
	TEST_CLASS(NativesUtilsTest) {
	public:
	std::filesystem::path testRoot = "TestNatives";

	TEST_METHOD_INITIALIZE(Setup) {
		if (std::filesystem::exists(testRoot)) std::filesystem::remove_all(testRoot);
		std::filesystem::create_directories(testRoot);
	}

	TEST_METHOD(TestExtraction) {
		// Create a dummy zip file
		std::filesystem::path zipPath = testRoot / "test.jar";
		std::filesystem::path extractDir = testRoot / "natives";

		// Create zip with miniz
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		Assert::IsTrue(mz_zip_writer_init_file(&zip_archive, zipPath.string().c_str(), 0), L"Failed to create zip");

		// Add a dll
		const char *dllData = "dummy dll content";
		mz_zip_writer_add_mem(&zip_archive, "test.dll", dllData, strlen(dllData), MZ_DEFAULT_COMPRESSION);

		// Add a text file (should be ignored)
		const char *txtData = "dummy text";
		mz_zip_writer_add_mem(&zip_archive, "readme.txt", txtData, strlen(txtData), MZ_DEFAULT_COMPRESSION);

		// Add a nested dll
		mz_zip_writer_add_mem(&zip_archive, "META-INF/nested.dll", dllData, strlen(dllData), MZ_DEFAULT_COMPRESSION);

		mz_zip_writer_finalize_archive(&zip_archive);
		mz_zip_writer_end(&zip_archive);

		// Extract
		Assert::IsTrue(NativesUtils::Extract(zipPath, extractDir));

		// Verify
		Assert::IsTrue(std::filesystem::exists(extractDir / "test.dll"), L"test.dll not extracted");
		Assert::IsTrue(std::filesystem::exists(extractDir / "nested.dll"), L"nested.dll not extracted (flattened)");
		Assert::IsFalse(std::filesystem::exists(extractDir / "readme.txt"), L"readme.txt should be ignored");
	}

	TEST_METHOD(TestExclusion) {
		// Create a dummy zip file
		std::filesystem::path zipPath = testRoot / "test_exclude.jar";
		std::filesystem::path extractDir = testRoot / "natives_exclude";

		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));
		mz_zip_writer_init_file(&zip_archive, zipPath.string().c_str(), 0);

		const char *dllData = "dummy";
		mz_zip_writer_add_mem(&zip_archive, "keep.dll", dllData, 5, MZ_DEFAULT_COMPRESSION);
		mz_zip_writer_add_mem(&zip_archive, "exclude_me.dll", dllData, 5, MZ_DEFAULT_COMPRESSION);

		mz_zip_writer_finalize_archive(&zip_archive);
		mz_zip_writer_end(&zip_archive);

		std::vector<std::string> excludes = {"exclude"};
		Assert::IsTrue(NativesUtils::Extract(zipPath, extractDir, excludes));

		Assert::IsTrue(std::filesystem::exists(extractDir / "keep.dll"));
		Assert::IsFalse(std::filesystem::exists(extractDir / "exclude_me.dll"));
	}
	};
}
