#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <windows.h>
#include "LaunchPlanner.h" // For ProcessStartInfo

namespace PCL_CPP::Core::Launcher::Launch {
	/**
	 * @brief 进程运行器类
	 * 
	 * @details 
	 * 该类封装了底层的 Windows 进程拉起逻辑：
	 * 1. **命令行构造**：将可执行文件路径与参数列表拼接，并对包含空格或特殊字符的参数进行转义处理。
	 * 2. **CreateProcessW 调用**：利用 Win32 API 启动 Java 进程。
	 *    - **宽字符支持**：支持 Unicode 路径和参数。
	 *    - **句柄管理**：启动后自动关闭进程和线程句柄（除非有特殊等待需求），防止资源泄漏。
	 */
    class ProcessRunner {
    public:
        /**
         * @brief 启动游戏进程
         * @details 
         * 执行流程：
         * 1. 拼接并转义命令行字符串。
         * 2. 初始化 `STARTUPINFOW` 和 `PROCESS_INFORMATION` 结构体。
         * 3. 调用 `CreateProcessW`。
         * 4. 记录启动结果及进程 ID (PID)。
         * @param startInfo 进程启动信息
         * @return 如果进程启动成功则返回 true
         */
        static bool Start(const ProcessStartInfo& startInfo);

    private:
        /**
         * @brief 对 Windows 命令行参数进行转义处理
         * @details 
         * 遵循 Windows 命令行解析规则：对包含空格或双引号的参数添加外层引号，并对内部的双引号进行反斜杠转义。
         * @param arg 原始参数
         * @return 转义后的参数
         */
        static std::string EscapeArg(const std::string& arg);
    };
}
