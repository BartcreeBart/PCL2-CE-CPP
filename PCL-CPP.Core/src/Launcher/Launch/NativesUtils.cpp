#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "NativesUtils.h"

// miniz 优化预处理指令
#define MINIZ_NO_TIME
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS

#include <miniz/miniz.h>

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Launch {

	/**
	 * @brief 从 Jar 文件中提取所有 .dll 文件到目标目录
	 * @param jarPath Jar 文件路径
	 * @param targetDir 目标提取目录
	 * @param exclude 排除的文件列表
	 * @return 是否全部提取过程完成
	 */
	bool NativesUtils::Extract(const std::filesystem::path &jarPath, const std::filesystem::path &targetDir, const std::vector<std::string> &exclude) {
		if (!std::filesystem::exists(jarPath)) {
			LOG_WARNING("Natives jar not found: {}", jarPath.string());
			return false;
		}

		// 确保目标目录存在
		std::filesystem::create_directories(targetDir);

		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));

		// 初始化 zip 读取器
		if (!mz_zip_reader_init_file(&zip_archive, jarPath.string().c_str(), 0)) {
			LOG_ERROR("Failed to open zip: {}", jarPath.string());
			return false;
		}

		int fileCount = (int) mz_zip_reader_get_num_files(&zip_archive);
		for (int i = 0; i < fileCount; i++) {
			mz_zip_archive_file_stat file_stat;
			if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

			// 跳过目录
			if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) continue;

			std::string filename = file_stat.m_filename;

			// 仅提取 .dll 文件
			if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".dll") {
				continue;
			}

			// 检查排除列表
			bool skipped = false;
			for (const auto &ex : exclude) {
				if (filename.find(ex) != std::string::npos) {
					skipped = true;
					break;
				}
			}
			if (skipped) continue;

			// 执行提取操作
			std::filesystem::path destPath = targetDir / std::filesystem::path(filename).filename();

			if (!mz_zip_reader_extract_to_file(&zip_archive, i, destPath.string().c_str(), 0)) {
				LOG_WARNING("Failed to extract native: {}", filename);
			}
		}

		mz_zip_reader_end(&zip_archive);
		return true;
	}

	/**
	 * @brief 清理 Natives 目录
	 * @param targetDir 要清理的目录路径
	 */
	void NativesUtils::Clean(const std::filesystem::path &targetDir) {
		if (std::filesystem::exists(targetDir)) {
			std::filesystem::remove_all(targetDir);
		}
	}
}
