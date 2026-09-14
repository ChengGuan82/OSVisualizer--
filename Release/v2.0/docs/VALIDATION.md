# 本次交付验证

验证日期：2026-09-11。环境：Windows x64、Qt 6.9.0 MinGW 套件、GCC 13.1、Release 构建。

| 验证项目 | 结果 |
| --- | --- |
| `scripts/build.ps1` 完整构建与发布运行库部署 | 通过 |
| CTest `core_tests` | 通过，标准算例、边界及 200 组固定种子随机输入 |
| CTest `gui_smoke` | 通过，5 种调度、4 种置换、前进/回退/播放/拖动、错误输入与旧结果清理 |
| 到达标记与 LRU 增量更新 | 到达时间保留测试、LRU 空闲/命中/换入/单块候选测试通过；界面自动更新、LRU 回退恢复检查通过 |
| `OSV_BUILD_GUI=OFF` 独立核心构建及 CTest | 通过，无 Qt 链接依赖 |
| 发布包清除 Qt/编译器 PATH 后使用 offscreen 插件运行 | 退出码 0 |
| 发布包清除 Qt/编译器 PATH 后使用原生 Windows 插件运行 | 退出码 0 |
| 实际 Qt 窗口截图 | 已生成并逐张检查中文、数据与图形布局 |

构建过程中修复了两个与本机环境有关的问题：Windows PowerShell 5.1 读取无 BOM 中文脚本、独立测试目录缺少 Qt offscreen 插件。构建脚本使用 UTF-8 BOM，并显式部署测试插件。

Qt 工具检测提示 Vulkan 头文件与部分 DirectX 编译器 DLL 未找到。本项目使用 Qt Widgets / QPainter，无 Vulkan 或 Qt Quick 功能；已完成原生 Windows 插件的实际启动验证。

本记录不代表在其他 Windows 电脑或 Linux 上做过测试，不代表性能基准，也不将断言数量作为独立用例数量。
