# PCL2-CE-CPP

> 🌏 **中文 (Chinese)** | [English](README_EN.md)

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

**Plain Craft Launcher 2 社区版 - C++ 重制**

PCL2-CE-CPP 是一个基于 **C++20** 和 **WinUI 3 (Windows App SDK)** 重新编构的、高性能(必须是) Minecraft 启动器。本项目是 [PCL2 社区版 (PCL2-CE)](https://github.com/PCL-Community/PCL2-CE) （同时也是 [Plain Craft Launcher 2 (PCL2)](https://github.com/Meloong-Git/PCL)）的 C++ 移植版本。

> 🚧 **当前状态**: 活跃开发中 ("正在生产橡胶...")

### 依赖库

本项目目前依赖以下开源库 ( 通过 **vcpkg包管理器** ) ：
*   `nlohmann-json`
*   `yaml-cpp`
*   `miniz`

## 📂 项目结构

*   **`PCL-CPP.Core/`**: 核心逻辑库 (静态库)。
*   **`PCL-CPP.Launcher/`**: 图形界面程序 (WinUI 3)。
*   **`PCL-CPP.Test/`**: 单元测试 (CppUnitTestFramework)。

## 📄 许可证

*   遵循 **Apache License, Version 2.0**。
*   作为 PCL2-CE(PCL2) 的衍生作品，本项目同时还遵守 **Plain Craft Launcher 2 二创指南**。
*   具体请参阅 [LICENSE](LICENSE) 和 [NOTICE](NOTICE)。

---
版权所有 (c) 2024-2026 Bart_Bart11 及贡献者（当然目前还没有）
