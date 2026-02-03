#pragma once
#include <string>
#include <filesystem>
#include <vector>

namespace PCL_CPP::Core::Launcher::Launch {
	/**
	 * @brief Natives 库工具类
	 * 
	 * @details 
	 * 该类负责处理 Minecraft 运行所需的 Native 库（.dll）：
	 * 1. **Zip 解压**：集成 `miniz` 库，直接从 Jar 包（本质是 Zip 格式）中读取并解压文件。
	 * 2. **按需提取**：仅提取后缀为 `.dll` 的文件，并支持根据排除列表（Exclude Rules）跳过特定文件（如 manifest 文件）。
	 * 3. **目录清理**：提供清理功能，确保每次启动时的 Natives 目录都是干净且最新的。
	 */
    class NativesUtils {
    public:
        /**
         * @brief 从 Jar 文件中提取所有 .dll 文件到目标目录
         * @details 
         * 实现细节：
         * 1. 使用 `mz_zip_reader_init_file` 初始化解压器。
         * 2. 遍历压缩包内所有文件，过滤出 `.dll` 文件。
         * 3. 检查文件名是否命中 `exclude` 中的任意关键词。
         * 4. 使用 `mz_zip_reader_extract_to_file` 执行解压。
         * @param jarPath Jar 文件路径
         * @param targetDir 目标提取目录
         * @param exclude 排除的文件列表
         * @return 是否全部提取过程完成
         */
        static bool Extract(const std::filesystem::path& jarPath, const std::filesystem::path& targetDir, const std::vector<std::string>& exclude = {});

        /**
         * @brief 清理 Natives 目录
         * @details 直接删除指定目录下的所有内容。
         * @param targetDir 要清理的目录路径
         */
        static void Clean(const std::filesystem::path& targetDir);
    };
}
