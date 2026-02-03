#include "pch.h"
#include "ProcessRunner.h"
#include "App/Logging/AppLogger.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Launch {

    /**
     * @brief 对 Windows 命令行参数进行转义处理
     * @param arg 原始参数
     * @return 转义后的参数
     */
    std::string ProcessRunner::EscapeArg(const std::string& arg) {
        // 如果不包含空格或引号，直接返回
        if (arg.find(' ') == std::string::npos && arg.find('"') == std::string::npos) {
            return arg;
        }
        
        std::string escaped = "\"";
        for (size_t i = 0; i < arg.length(); i++) {
            char c = arg[i];
            if (c == '"') {
                // 转义双引号
                escaped += "\\\""; 
            } else if (c == '\\') {
                // 反斜杠在某些情况下需要处理，这里简单处理
                escaped += c;
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }

    /**
     * @brief 启动指定的进程
     * @param startInfo 进程启动配置信息
     * @return 如果进程启动成功则返回 true
     */
    bool ProcessRunner::Start(const ProcessStartInfo& startInfo) {
        // 构造命令行字符串
        std::string cmdLine = "\"" + startInfo.Executable.string() + "\"";
        for (const auto& arg : startInfo.Arguments) {
            cmdLine += " " + EscapeArg(arg);
        }

        LOG_INFO("Starting process: {}", cmdLine);
        LOG_INFO("Working directory: {}", startInfo.WorkingDirectory.string());

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // CreateProcessW 需要可修改的字符串
        std::wstring wCmdLine(cmdLine.begin(), cmdLine.end()); // 基础转换，假定参数兼容 ASCII/UTF8
        std::wstring wWorkDir = startInfo.WorkingDirectory.wstring();

        // 启动进程
        if (!CreateProcessW(
            NULL,           // 不使用模块名
            &wCmdLine[0],   // 命令行
            NULL,           // 进程句柄不可继承
            NULL,           // 线程句柄不可继承
            FALSE,          // 句柄继承标志
            0,              // 无创建标志
            NULL,           // 使用父进程的环境块
            wWorkDir.c_str(), // 工作目录 
            &si,            // STARTUPINFO 指针
            &pi             // PROCESS_INFORMATION 指针
        )) {
            LOG_ERROR("CreateProcess failed ({})", GetLastError());
            return false;
        }

        LOG_INFO("Process started. PID: {}", pi.dwProcessId);

        // 关闭句柄（除非需要等待进程结束，否则通常不需要保留）
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }
}
