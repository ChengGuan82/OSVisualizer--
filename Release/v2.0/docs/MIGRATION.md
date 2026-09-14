# 从课程实验到可视化平台

## v1 原始来源

本项目对用户自己的实验代码进行结构性重构。不是直接嵌入原 GUI，也不将原有实现中的所有策略都宣称为本版功能。

| 来源（相对于工作区） | 本版复用与调整 |
| --- | --- |
| `exp/OS-exp5/GUI/process.cpp` | 保留 FCFS 到达排序、SJF 就绪集合选择、Priority 大数优先与 RR 入队顺序；统一为策略接口，补充每次执行片段和完成指标，排序改为稳定排序，空闲直接跳转事件。 |
| `exp/OS-exp5/GUI/process.h` | 将含 QString 的进程对象改为纯 C++ 整数数据模型，算法输入与输出分离；v1 采用整数时间。 |
| `exp/OS-exp6/GUI/page_replacement_alg.cpp` | 保留 FIFO 淘汰最早装入页、LRU 最近使用、OPT 最远未来和 CLOCK 二次机会思路；以完整快照取代字符串报告和输出参数。 |
| `exp/OS-exp6/GUI/page_replacement_alg.h` | 多个函数统一为 PageReplacement 接口；新增被替换页、访问位、扫描指针与访问位清零记录。 |

新增 SRTF、策略对比、CSV、确定性测试、界面回放和统一导航。HRN、MLFQ、LFU、改进 CLOCK 等原实验中的其他策略不在当前发布范围。

## 原实现中需要调整的地方

1. **CLOCK 随机初始位**：原实现对空页框设置随机访问位，导致行为不稳定；本版空页框全为 0，避免无依据的二次机会。
2. **CLOCK 替换后的指针**：原实现在写入后立即 break，未统一推进指针；本版替换后前移一格，并将“下次扫描位置”写入快照。
3. **FIFO/LRU 的显示位置**：原实现直接用队列顺序/最近使用顺序作为页框表格，使物理页框随命中或淘汰移动；本版使用固定槽位，替换元数据单独存储。
4. **模拟数据与显示耦合**：原进程模型依赖 QString，页面结果包含报告字符串；本版纯 STL 数据可在没有 Qt 时构建和测试。
5. **可复现性与合法输入**：本版明确平局规则、时间片边界、非法输入拒绝规则，并防止修改参数后继续回放旧轨迹。

## 来源指纹（本次读取时 SHA-256）

```text
exp/OS-exp5/GUI/process.cpp
D5DA1D8828AC5EC7F4770EBC401E65C981AC0BA13395E573B49EBADB5254D572

exp/OS-exp6/GUI/page_replacement_alg.cpp
8A7335857E244ADFBF54240F4D57167F57B639CF2B1B40FCF2B6309A8A036DDB
```

这些指纹帮助识别重构起点，不是性能或正确性证明。原实验文件保持原样。

## v2 迁移：以现有实验为基础

| 现有实验 | 生产实现 | 保留与修改 |
| --- | --- | --- |
| `exp/OS-exp3/banker_alg.cpp` 的 `isSafe` | `src/core/Banker.cpp` | 保留 Work=Available、按进程编号循环扫描、Need 逐资源比较和释放 Allocation 的逻辑；把输出改为 Need/安全序列/每次检查快照；一轮无进展即结束，新增维度和数量校验。 |
| `exp/OS-exp7/disk_schedule.cpp` 的 FCFS/SSTF/SCAN/CSCAN | `src/core/Disk.cpp` | 保留输入顺序、最近距离选择、左右分组和边界回绕逻辑；增加左向扫描、请求身份、总移动量和每步轨迹。 |
| `exp/OS-exp7/GUI/DiskScheduler.cpp` | `src/ui/DiskSchedulerWidget.cpp` | 沿用输入→选择策略→显示结果的交互组织；原随机请求改为可编辑序列，QString 结果改为核心模型；磁头路径图和回放为新增功能，原 GUI 未提供路径图。 |
| v1 CPU/页面界面和算法 | 原文件继续维护 | 保留 SRTF、OPT、对比、到达标记、LRU 队列与 CLOCK 虚线环；按 v2 文档把 Priority 从大数优先改为小数优先，并补测试。 |

银行家原控制台还包含资源请求和回滚，本版按方案仅迁移安全性检测，不声称已经迁移请求分配功能。原磁盘代码包含 LOOK/CLOOK/NStepSCAN/FSCAN，这些不在 v2 范围。

本次是迁移重构，不是直接运行旧控制台程序。原函数与图形显示、字符串输出耦合的部分改成结构化结果，以支持测试和回放。

### 原实现直接参与回归

`tests/legacy/banker_original.cpp` 与 `tests/legacy/disk_original.cpp` 是原文件的字节级副本，测试编译时只重命名 main 入口。它们链接进测试程序，不链接进 GUI。

- 银行家：200 组合法输入同时对比原 `isSafe` 和独立穷举安全序列结果。
- 磁盘：默认示例的四种策略与原实现对照；200 组请求的 SSTF 服务顺序与原实现一致。
- 原 SCAN/C-SCAN 即使无剩余请求也会继续输出边界点；v2 明确在最后请求完成时停止，只在仍需服务另一侧请求时走边界/回绕。这是有意修正，不要求该边界条件与旧版相同。
- 最大磁道号与原 `range` 换算：`maxTrack = range - 1`；C-SCAN 回绕计入移动量。

原文件与测试副本 SHA-256 一致：

```text
banker_alg.cpp / tests/legacy/banker_original.cpp
8FD5992477D6AA14241B97FEC18E5CA9FD1B9EBEEC8DA3F3E8E42C23315FC7DB

disk_schedule.cpp / tests/legacy/disk_original.cpp
E4FCA043DD982045C115DA25EAABB3B94062BDD4169BBB16A1343BEB20093D0F
```
