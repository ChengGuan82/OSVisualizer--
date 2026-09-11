#include "ui/MainWindow.h"
#include "ui/Charts.h"
#include <QtWidgets>
#include <stdexcept>

namespace {
QLabel* label(const QString& text, const char* name = "") {
    auto* l = new QLabel(text); l->setObjectName(name); l->setWordWrap(true); return l;
}
QPushButton* button(const QString& text, const char* name = "") {
    auto* b = new QPushButton(text); b->setObjectName(name); b->setCursor(Qt::PointingHandCursor); return b;
}
QTableWidget* table(const QStringList& headers, bool editable = false) {
    auto* t = new QTableWidget(0, headers.size()); t->setHorizontalHeaderLabels(headers);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->verticalHeader()->hide(); t->setAlternatingRowColors(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    if (!editable) t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    return t;
}
void cell(QTableWidget* t, int row, int column, const QString& value) {
    auto* item = new QTableWidgetItem(value); item->setTextAlignment(Qt::AlignCenter); t->setItem(row, column, item);
}
QString number(double n) { return QString::number(n, 'f', 2); }
QFrame* card(QVBoxLayout*& layout) {
    auto* frame = new QFrame; frame->setObjectName("card"); layout = new QVBoxLayout(frame);
    layout->setContentsMargins(20, 16, 20, 16); layout->setSpacing(12); return frame;
}
int integer(const QString& text) {
    bool ok = false; int value = text.trimmed().toInt(&ok);
    if (!ok) throw std::invalid_argument("输入必须为整数，请检查空白、文字或超出范围的数值");
    return value;
}
void writeCsv(QWidget* parent, const QString& name, const QString& contents) {
    QString path = QFileDialog::getSaveFileName(parent, "导出当前模拟结果", name, "CSV (*.csv)");
    if (path.isEmpty()) return;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write((QString(QChar(0xFEFF)) + contents).toUtf8()) < 0 || !file.commit())
        QMessageBox::warning(parent, "导出失败", file.errorString());
}
}

MainWindow::MainWindow() {
    cpuRefresh_.setSingleShot(true); cpuRefresh_.setInterval(350);
    pageRefresh_.setSingleShot(true); pageRefresh_.setInterval(350);
    connect(&cpuRefresh_, &QTimer::timeout, this, [this] { runCpu(); });
    connect(&pageRefresh_, &QTimer::timeout, this, [this] { runPages(); });
    setWindowTitle("OS Lab · 操作系统算法可视化平台"); resize(1320, 960); setMinimumSize(1080, 760);
    setStyleSheet(R"(
        QWidget { font-family: 'Microsoft YaHei UI', 'Noto Sans CJK SC', sans-serif; font-size: 13px; color: #20313f; }
        QMainWindow, QStackedWidget { background: #f3f5f5; }
        QFrame#sidebar { background: #172d39; border: none; }
        QLabel#brand { color: #ffffff; font-size: 26px; font-weight: 700; }
        QLabel#sideText { color: #a8c0c8; font-size: 12px; }
        QLabel#sideBadge { color: #8fddc4; font-size: 12px; }
        QListWidget { background: transparent; border: none; color: #b7cbd3; outline: none; }
        QListWidget::item { padding: 15px 12px; margin: 4px 0; border-radius: 7px; }
        QListWidget::item:selected { background: #2a4854; color: white; }
        QLabel#eyebrow { color: #138b80; font-size: 11px; font-weight: 700; }
        QLabel#title { font-size: 28px; font-weight: 700; }
        QLabel#muted { color: #6a7681; }
        QLabel#section { font-size: 15px; font-weight: 700; }
        QLabel#stats { background: #e5f1ed; border-radius: 8px; padding: 14px; color: #176b60; font-size: 16px; font-weight: 600; }
        QLabel#message { color: #986433; }
        QFrame#card { background: white; border: 1px solid #e0e6e7; border-radius: 10px; }
        QPushButton { background: white; border: 1px solid #ccd7da; border-radius: 6px; padding: 8px 14px; }
        QPushButton:hover { background: #eaf2ef; border-color: #138b80; }
        QPushButton#primary { background: #138b80; border-color: #138b80; color: white; font-weight: 600; }
        QPushButton:disabled { color: #a9b1b8; background: #edf0f1; border-color: #e1e5e6; }
        QComboBox, QSpinBox, QLineEdit { background: white; border: 1px solid #ccd7da; border-radius: 5px; padding: 7px; }
        QSpinBox { padding-right: 30px; min-width: 42px; }
        QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 26px; border-left: 1px solid #ccd7da; border-bottom: 1px solid #ccd7da; border-top-right-radius: 5px; background: #f1f6f4; }
        QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 26px; border-left: 1px solid #ccd7da; border-bottom-right-radius: 5px; background: #f1f6f4; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #dcefe8; }
        QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background: #b9ddcf; }
        QSpinBox::up-button:disabled, QSpinBox::down-button:disabled { background: #edf0f1; }
        QSpinBox::up-arrow { image: url(:/icons/arrow-up.svg); width: 12px; height: 8px; }
        QSpinBox::down-arrow { image: url(:/icons/arrow-down.svg); width: 12px; height: 8px; }
        QTableWidget { background: white; alternate-background-color: #f7f9f9; border: 1px solid #e3e8ea; gridline-color: #edf0f1; selection-background-color: #dcefe8; selection-color: #20313f; }
        QHeaderView::section { background: #f1f5f5; color: #60727e; border: none; padding: 8px; font-size: 12px; }
        QScrollArea { border: none; background: transparent; }
        QSlider::groove:horizontal { height: 5px; background: #dce5e4; border-radius: 2px; }
        QSlider::handle:horizontal { background: #138b80; width: 14px; margin: -5px 0; border-radius: 7px; }
    )");
    auto* root = new QWidget; auto* layout = new QHBoxLayout(root); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    auto* sidebar = new QFrame; sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(214);
    auto* side = new QVBoxLayout(sidebar); side->setContentsMargins(22, 30, 22, 24); side->setSpacing(18);
    side->addWidget(label("OS Lab", "brand")); side->addWidget(label("ALGORITHM VISUALIZER", "sideText")); side->addSpacing(28);
    auto* nav = new QListWidget; nav->addItems({"01   CPU 调度", "02   页面置换"}); side->addWidget(nav);
    side->addWidget(label("●  本地算法实验室", "sideBadge"));
    side->addWidget(label("基于实验 5 / 实验 6 演进\nC++17 · Qt 6 · CMake\nv1.0  /  可复现的算法轨迹", "sideText"));
    stack_ = new QStackedWidget; stack_->addWidget(buildCpu()); stack_->addWidget(buildPages());
    layout->addWidget(sidebar); layout->addWidget(stack_, 1); setCentralWidget(root);
    connect(nav, &QListWidget::currentRowChanged, this, [this](int row) { stopPlayback(); stack_->setCurrentIndex(row); });
    nav->setCurrentRow(0);
    timer_.setInterval(700);
    connect(&timer_, &QTimer::timeout, this, [this] {
        if (step_ + 1 < int(pages_.steps.size())) { ++step_; renderStep(); }
        else stopPlayback();
    });
    setCpuExample(); runCpu(); runPages();
}

QWidget* MainWindow::buildCpu() {
    auto* page = new QWidget; auto* outer = new QVBoxLayout(page); outer->setContentsMargins(28, 24, 28, 24); outer->setSpacing(12);
    outer->addWidget(label("PROCESS MANAGEMENT  /  01", "eyebrow"));
    outer->addWidget(label("CPU 调度实验室", "title"));
    outer->addWidget(label("已备好示例，无需初始化。双击单元格编辑；修改后自动更新，切换策略可直接比较。", "muted"));
    QVBoxLayout* content; outer->addWidget(card(content));
    auto* controls = new QHBoxLayout;
    controls->addWidget(label("调度策略")); cpuAlgorithm_ = new QComboBox;
    cpuAlgorithm_->addItems({"FCFS · 先来先服务", "SJF · 短作业优先", "SRTF · 最短剩余时间", "RR · 时间片轮转", "Priority · 非抢占优先级"});
    controls->addWidget(cpuAlgorithm_, 1); controls->addWidget(label("时间片")); quantum_ = new QSpinBox; quantum_->setRange(1, 10000); quantum_->setValue(2); quantum_->setEnabled(false); controls->addWidget(quantum_);
    auto* run = button("立即更新  →", "primary"); controls->addWidget(run); content->addLayout(controls);
    auto* inputHeading = new QHBoxLayout; inputHeading->addWidget(label("01  进程参数", "section")); inputHeading->addStretch();
    auto* add = button("＋ 进程"); auto* remove = button("删除选中"); auto* example = button("载入示例");
    inputHeading->addWidget(add); inputHeading->addWidget(remove); inputHeading->addWidget(example); content->addLayout(inputHeading);
    input_ = table({"PID", "到达时间", "运行时间", "优先级（越大越优先）"}, true); input_->verticalHeader()->setDefaultSectionSize(26); input_->setFixedHeight(150); content->addWidget(input_);
    cpuMessage_ = label("", "message"); content->addWidget(cpuMessage_);
    cpuHelp_ = label("", "muted"); content->addWidget(cpuHelp_);
    cpuStats_ = label("", "stats"); outer->addWidget(cpuStats_);
    QVBoxLayout* result; outer->addWidget(card(result), 1);
    auto* chartHeading = new QHBoxLayout; chartHeading->addWidget(label("02  调度轨迹", "section")); chartHeading->addWidget(label("橙色箭头＝到达 · 等宽片段内按时间定位 · Δt＝时长 · 可滚动", "muted"), 1);
    cpuExport_ = button("导出 CSV"); chartHeading->addWidget(cpuExport_); result->addLayout(chartHeading);
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFixedHeight(220);
    gantt_ = new GanttChart; scroll->setWidget(gantt_); result->addWidget(scroll);
    auto* tabs = new QTabWidget;
    cpuResults_ = table({"PID", "完成时间", "等待时间", "周转时间", "带权周转"}); tabs->addTab(cpuResults_, "进程指标");
    cpuCompare_ = table({"策略", "平均等待", "平均周转", "CPU 利用率"}); tabs->addTab(cpuCompare_, "同组输入 · 全策略对比");
    result->addWidget(tabs, 1);
    connect(run, &QPushButton::clicked, this, [this] { runCpu(); });
    connect(example, &QPushButton::clicked, this, [this] { setCpuExample(); runCpu(); });
    connect(add, &QPushButton::clicked, this, [this] {
        if (input_->rowCount() >= 100) return;
        const QSignalBlocker blocker(input_);
        int pid = 1; QSet<int> used;
        for (int i = 0; i < input_->rowCount(); ++i) if (input_->item(i, 0)) used.insert(input_->item(i, 0)->text().toInt());
        while (used.contains(pid)) ++pid;
        int row = input_->rowCount(); input_->insertRow(row);
        cell(input_, row, 0, QString::number(pid)); cell(input_, row, 1, "0"); cell(input_, row, 2, "3"); cell(input_, row, 3, "1");
        runCpu();
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        auto rows = input_->selectionModel()->selectedRows();
        std::sort(rows.begin(), rows.end(), [](const QModelIndex& a, const QModelIndex& b) { return a.row() > b.row(); });
        for (const auto& row : rows) input_->removeRow(row.row());
        runCpu();
    });
    connect(input_, &QTableWidget::itemChanged, this, [this] { invalidateCpu(); cpuRefresh_.start(); });
    connect(cpuAlgorithm_, &QComboBox::currentIndexChanged, this, [this](int index) { quantum_->setEnabled(index == 3); runCpu(); });
    connect(quantum_, &QSpinBox::valueChanged, this, [this] { runCpu(); });
    connect(cpuExport_, &QPushButton::clicked, this, [this] { exportCpu(); });
    page->setMinimumHeight(900);
    auto* pageScroll = new QScrollArea; pageScroll->setWidgetResizable(true); pageScroll->setWidget(page);
    return pageScroll;
}

QWidget* MainWindow::buildPages() {
    auto* page = new QWidget; auto* outer = new QVBoxLayout(page); outer->setContentsMargins(28, 24, 28, 24); outer->setSpacing(12);
    outer->addWidget(label("MEMORY MANAGEMENT  /  02", "eyebrow")); outer->addWidget(label("页面置换实验室", "title"));
    outer->addWidget(label("示例已就绪，直接点击下一步或播放。修改序列、物理块数或算法后自动重新演示，无需初始化。", "muted"));
    QVBoxLayout* input; outer->addWidget(card(input));
    auto* controls = new QHBoxLayout;
    controls->addWidget(label("置换策略")); pageAlgorithm_ = new QComboBox; pageAlgorithm_->addItems({"FIFO", "LRU", "CLOCK", "OPT"}); pageAlgorithm_->setCurrentIndex(2); controls->addWidget(pageAlgorithm_);
    controls->addWidget(label("物理块")); frameCount_ = new QSpinBox; frameCount_->setRange(1, 16); frameCount_->setValue(3); controls->addWidget(frameCount_); controls->addStretch();
    auto* example = button("恢复示例"); controls->addWidget(example); auto* run = button("从头演示  →", "primary"); controls->addWidget(run); input->addLayout(controls);
    references_ = new QLineEdit("7 0 1 2 0 3 0 4 2 3 0 3 2"); references_->setPlaceholderText("非负页面编号，空格或逗号分隔，最多 500 项"); input->addWidget(references_);
    pageMessage_ = label("", "message"); input->addWidget(pageMessage_);
    pageHelp_ = label("", "muted"); input->addWidget(pageHelp_);
    pageStats_ = label("", "stats"); outer->addWidget(pageStats_);
    QVBoxLayout* visual; outer->addWidget(card(visual));
    auto* heading = new QHBoxLayout; heading->addWidget(label("01  物理页框快照", "section")); heading->addStretch(); pageExport_ = button("导出 CSV"); heading->addWidget(pageExport_); visual->addLayout(heading);
    frames_ = new FrameChart; visual->addWidget(frames_);
    lruOrder_ = label("", "muted"); visual->addWidget(lruOrder_);
    auto* playback = new QHBoxLayout; previous_ = button("← 上一步"); play_ = button("自动播放", "primary"); next_ = button("下一步 →");
    playback->addWidget(previous_); playback->addWidget(play_); playback->addWidget(next_);
    stepSlider_ = new QSlider(Qt::Horizontal); playback->addWidget(stepSlider_, 1); stepLabel_ = label("初始状态", "muted"); playback->addWidget(stepLabel_); visual->addLayout(playback);
    visual->addWidget(label("每一步处理一次访问；拖动或上一步会暂停。播完后再次播放可重播。绿色＝本次访问，橙色＝LRU 淘汰候选；命中不会换出页面。", "muted"));
    QVBoxLayout* history; outer->addWidget(card(history), 1); history->addWidget(label("02  访问轨迹与对比", "section"));
    auto* tabs = new QTabWidget; pageHistory_ = table({}); tabs->addTab(pageHistory_, "已访问页面 · 固定页框");
    pageCompare_ = table({"策略", "访问次数", "缺页次数", "缺页率"}); tabs->addTab(pageCompare_, "完整序列 · 全策略对比"); history->addWidget(tabs);
    connect(run, &QPushButton::clicked, this, [this] { runPages(); });
    connect(example, &QPushButton::clicked, this, [this] {
        const QSignalBlocker a(references_), b(frameCount_);
        references_->setText("7 0 1 2 0 3 0 4 2 3 0 3 2"); frameCount_->setValue(3); runPages();
    });
    connect(references_, &QLineEdit::textChanged, this, [this] { invalidatePages(); pageRefresh_.start(); });
    connect(references_, &QLineEdit::returnPressed, this, [this] { runPages(); });
    connect(frameCount_, &QSpinBox::valueChanged, this, [this] { runPages(); });
    connect(pageAlgorithm_, &QComboBox::currentIndexChanged, this, [this] { runPages(); });
    connect(previous_, &QPushButton::clicked, this, [this] { stopPlayback(); if (step_ >= 0) --step_; renderStep(); });
    connect(next_, &QPushButton::clicked, this, [this] { stopPlayback(); if (step_ + 1 < int(pages_.steps.size())) ++step_; renderStep(); });
    connect(play_, &QPushButton::clicked, this, [this] {
        if (timer_.isActive()) { stopPlayback(); return; }
        if (pages_.steps.empty()) return;
        if (step_ + 1 == int(pages_.steps.size())) { step_ = -1; renderStep(); }
        timer_.start(); play_->setText("暂停");
    });
    connect(stepSlider_, &QSlider::valueChanged, this, [this](int value) { stopPlayback(); step_ = value - 1; renderStep(); });
    connect(pageExport_, &QPushButton::clicked, this, [this] { exportPages(); });
    page->setMinimumHeight(900);
    auto* pageScroll = new QScrollArea; pageScroll->setWidgetResizable(true); pageScroll->setWidget(page);
    return pageScroll;
}

void MainWindow::setCpuExample() {
    const QSignalBlocker blocker(input_); input_->setRowCount(4);
    const int values[4][4] = {{1, 0, 5, 2}, {2, 1, 3, 1}, {3, 2, 4, 3}, {4, 4, 2, 2}};
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) cell(input_, i, j, QString::number(values[i][j]));
    invalidateCpu();
}
std::vector<osv::Process> MainWindow::readProcesses() const {
    std::vector<osv::Process> processes;
    for (int i = 0; i < input_->rowCount(); ++i) {
        int fields[4];
        for (int j = 0; j < 4; ++j) fields[j] = integer(input_->item(i, j) ? input_->item(i, j)->text() : "");
        processes.push_back({fields[0], fields[1], fields[2], fields[3]});
    }
    return processes;
}
void MainWindow::invalidateCpu() {
    schedule_ = {}; cpuResults_->setRowCount(0); cpuCompare_->setRowCount(0); gantt_->setResult(schedule_);
    cpuExport_->setEnabled(false); cpuStats_->setText("正在更新"); cpuMessage_->setText("参数已变更，停止编辑后自动更新。");
}
void MainWindow::runCpu() {
    cpuRefresh_.stop();
    const QStringList explanations = {
        "FCFS：按到达顺序运行至完成；同一时刻到达按表格顺序。时间为整数，不计切换开销。",
        "SJF：每次从已到达进程中选运行时间最短者，非抢占；同值先到先服务。",
        "SRTF：新进程到达时比较剩余时间，必要时抢占；同值按到达时间、表格顺序。",
        "RR：每次最多运行一个时间片；片末先接收新到达进程，再将未完成进程放回队尾。",
        "Priority：数值越大越优先，非抢占；同值按到达时间、表格顺序。"};
    cpuHelp_->setText(explanations[cpuAlgorithm_->currentIndex()]);
    invalidateCpu();
    try {
        const auto input = readProcesses();
        schedule_ = osv::StandardScheduler(static_cast<osv::Scheduling>(cpuAlgorithm_->currentIndex()), quantum_->value()).run(input);
        cpuResults_->setRowCount(int(schedule_.processes.size()));
        for (int i = 0; i < int(schedule_.processes.size()); ++i) {
            const auto& r = schedule_.processes[i];
            const QStringList values = {QString("P%1").arg(r.pid), QString::number(r.completion), QString::number(r.waiting), QString::number(r.turnaround), number(r.weightedTurnaround)};
            for (int j = 0; j < values.size(); ++j) cell(cpuResults_, i, j, values[j]);
        }
        gantt_->setResult(schedule_);
        cpuStats_->setText(QString("平均等待  %1     ·     平均周转  %2     ·     CPU 利用率  %3%").arg(number(schedule_.averageWaiting), number(schedule_.averageTurnaround), number(100 * schedule_.utilization)));
        cpuCompare_->setRowCount(5);
        for (int i = 0; i < 5; ++i) {
            auto r = osv::StandardScheduler(static_cast<osv::Scheduling>(i), quantum_->value()).run(input);
            cell(cpuCompare_, i, 0, cpuAlgorithm_->itemText(i).section(" ·", 0, 0)); cell(cpuCompare_, i, 1, number(r.averageWaiting)); cell(cpuCompare_, i, 2, number(r.averageTurnaround)); cell(cpuCompare_, i, 3, number(r.utilization * 100) + "%");
        }
        cpuExport_->setEnabled(true); cpuMessage_->setText("模拟完成 · 利用率以 t=0 到最后完成时刻为统计区间；不计上下文切换开销。");
    } catch (const std::exception& e) { cpuMessage_->setText(QString("无法模拟：%1").arg(QString::fromUtf8(e.what()))); }
}
void MainWindow::stopPlayback() { timer_.stop(); if (play_) play_->setText("自动播放"); }
void MainWindow::invalidatePages() {
    stopPlayback(); pages_ = {}; step_ = -1; pageCompare_->setRowCount(0); pageHistory_->setColumnCount(0);
    pageExport_->setEnabled(false); pageMessage_->setText("停止输入后自动更新；支持空格、逗号、分号分隔。"); renderStep();
}
void MainWindow::runPages() {
    pageRefresh_.stop();
    const QStringList explanations = {
        "FIFO：缺页时换出最早装入的页面；命中不改变换出顺序。修改参数后回到第 0 步。",
        "LRU：每次访问后将该物理块移到最近使用端；有空闲块先填空，满后缺页才换出队首。",
        "CLOCK：命中置 R=1；缺页扫描时将 R=1 清零，遇 R=0 换入，随后指针前移。",
        "OPT：缺页时换出未来最晚再访问（或不再访问）的页面；使用完整序列作为离线最优基线。"};
    pageHelp_->setText(explanations[pageAlgorithm_->currentIndex()]);
    invalidatePages();
    try {
        std::vector<int> refs;
        const auto tokens = references_->text().split(QRegularExpression("[\\s,，;；]+"), Qt::SkipEmptyParts);
        for (const auto& token : tokens) refs.push_back(integer(token));
        pages_ = osv::StandardReplacement(static_cast<osv::Replacement>(pageAlgorithm_->currentIndex())).run(refs, frameCount_->value());
        pageCompare_->setRowCount(4);
        for (int i = 0; i < 4; ++i) {
            auto r = osv::StandardReplacement(static_cast<osv::Replacement>(i)).run(refs, frameCount_->value());
            cell(pageCompare_, i, 0, pageAlgorithm_->itemText(i)); cell(pageCompare_, i, 1, QString::number(refs.size())); cell(pageCompare_, i, 2, QString::number(r.faults)); cell(pageCompare_, i, 3, number(r.faultRate * 100) + "%");
        }
        pageHistory_->setRowCount(frameCount_->value() + 2);
        QStringList rows; for (int i = 0; i < frameCount_->value(); ++i) rows << QString("Frame %1").arg(i + 1); rows << "结果" << "淘汰页";
        pageHistory_->setVerticalHeaderLabels(rows); pageHistory_->verticalHeader()->show();
        pageHistory_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed); pageHistory_->horizontalHeader()->setDefaultSectionSize(66);
        pageExport_->setEnabled(true); renderStep();
    } catch (const std::exception& e) { pageMessage_->setText(QString("无法模拟：%1").arg(QString::fromUtf8(e.what()))); }
}
void MainWindow::renderStep() {
    const bool has = !pages_.steps.empty(); const int count = frameCount_->value();
    frames_->setStep(step_ < 0 ? nullptr : &pages_.steps[step_], count, static_cast<osv::Replacement>(pageAlgorithm_->currentIndex()));
    lruOrder_->setVisible(pageAlgorithm_->currentIndex() == 1);
    if (pageAlgorithm_->currentIndex() == 1) {
        QStringList order;
        if (step_ >= 0) for (int f : pages_.steps[step_].lruOrder)
            order << QString("Frame %1〔页 %2〕").arg(f + 1).arg(pages_.steps[step_].frames[f]);
        QString candidate = "存在空闲块，下次缺页先填空，不换出页面。";
        if (step_ >= 0 && pages_.steps[step_].nextVictim >= 0) {
            int f = pages_.steps[step_].nextVictim;
            candidate = QString("若下一次访问缺页，将换出 Frame %1〔页 %2〕；若命中，只更新顺序。").arg(f + 1).arg(pages_.steps[step_].frames[f]);
        }
        lruOrder_->setText(QString("LRU 顺序（最久未访问 → 最近访问）：%1\n%2").arg(order.isEmpty() ? "尚未访问" : order.join(" → "), candidate));
    }
    previous_->setEnabled(has && step_ >= 0); next_->setEnabled(has && step_ + 1 < int(pages_.steps.size())); play_->setEnabled(has);
    { const QSignalBlocker blocker(stepSlider_); stepSlider_->setRange(0, int(pages_.steps.size())); stepSlider_->setValue(step_ + 1); stepSlider_->setEnabled(has); }
    stepLabel_->setText(QString("%1 / %2 步").arg(step_ + 1).arg(pages_.steps.size()));
    int faults = 0;
    pageHistory_->setColumnCount(step_ + 1);
    for (int i = 0; i <= step_; ++i) {
        const auto& s = pages_.steps[i]; faults += s.fault;
        pageHistory_->setHorizontalHeaderItem(i, new QTableWidgetItem(QString("%1: %2").arg(i + 1).arg(s.requestedPage)));
        for (int f = 0; f < count; ++f) {
            cell(pageHistory_, f, i, s.frames[f] < 0 ? "—" : QString::number(s.frames[f]));
            if (f == s.touchedFrame) pageHistory_->item(f, i)->setBackground(QColor(s.fault ? "#f8ead8" : "#e2f3ed"));
        }
        cell(pageHistory_, count, i, s.fault ? "缺页" : "命中"); cell(pageHistory_, count + 1, i, s.replacedPage < 0 ? "—" : QString::number(s.replacedPage));
    }
    if (step_ >= 0) pageHistory_->scrollToItem(pageHistory_->item(0, step_));
    pageStats_->setText(has ? QString("已访问  %1     ·     当前缺页  %2     ·     当前缺页率  %3%     /     全程缺页  %4").arg(step_ + 1).arg(faults).arg(number(step_ < 0 ? 0 : faults * 100.0 / (step_ + 1))).arg(pages_.faults) : "等待生成轨迹");
    if (!has) return;
    if (step_ < 0) pageMessage_->setText("轨迹已就绪：当前为第 0 步，页框为空。直接点击下一步或自动播放；无需设置初始内存。");
    else {
        const auto& s = pages_.steps[step_];
        QString message = QString("访问页 %1 → %2；Frame %3；%4").arg(s.requestedPage).arg(s.fault ? "缺页" : "命中").arg(s.touchedFrame + 1).arg(s.replacedPage < 0 ? "没有淘汰页面" : QString("淘汰页 %1").arg(s.replacedPage));
        if (pageAlgorithm_->currentIndex() == 2) {
            QStringList cleared; for (int f : s.clearedFrames) cleared << QString::number(f + 1);
            message += QString("；访问位清零：%1；下次从 Frame %2 扫描").arg(cleared.isEmpty() ? "无" : cleared.join(", ")).arg(s.hand + 1);
        }
        pageMessage_->setText(message);
    }
    if (step_ + 1 == int(pages_.steps.size())) stopPlayback();
}

void MainWindow::exportCpu() {
    if (schedule_.processes.empty()) return;
    QString text = QString("algorithm,quantum\n%1,%2\n\npid,arrival,burst,priority\n").arg(cpuAlgorithm_->currentText()).arg(quantum_->value());
    for (const auto& p : readProcesses()) text += QString("%1,%2,%3,%4\n").arg(p.pid).arg(p.arrival).arg(p.burst).arg(p.priority);
    text += "\npid,completion,waiting,turnaround,weighted_turnaround\n";
    for (const auto& r : schedule_.processes) text += QString("%1,%2,%3,%4,%5\n").arg(r.pid).arg(r.completion).arg(r.waiting).arg(r.turnaround).arg(number(r.weightedTurnaround));
    text += "\npid,start,end\n"; for (const auto& s : schedule_.timeline) text += QString("%1,%2,%3\n").arg(s.pid).arg(s.start).arg(s.end);
    text += QString("\naverage_waiting,average_turnaround,utilization_percent\n%1,%2,%3\n").arg(number(schedule_.averageWaiting), number(schedule_.averageTurnaround), number(schedule_.utilization * 100));
    writeCsv(this, "cpu-simulation.csv", text);
}
void MainWindow::exportPages() {
    if (pages_.steps.empty()) return;
    QString text = QString("algorithm,frames\n%1,%2\n\nstep,page,fault,evicted,next_hand,lru_next_victim,lru_order_oldest_to_newest").arg(pageAlgorithm_->currentText()).arg(frameCount_->value());
    for (int i = 0; i < frameCount_->value(); ++i) text += QString(",frame_%1,reference_bit_%1").arg(i + 1);
    text += "\n"; int i = 0;
    for (const auto& s : pages_.steps) {
        QStringList order; for (int f : s.lruOrder) order << QString::number(f + 1);
        text += QString("%1,%2,%3,%4,%5,%6,%7").arg(++i).arg(s.requestedPage).arg(s.fault ? 1 : 0).arg(s.replacedPage).arg(s.hand + 1).arg(s.nextVictim < 0 ? -1 : s.nextVictim + 1).arg(order.join(" > "));
        for (int f = 0; f < frameCount_->value(); ++f) text += QString(",%1,%2").arg(s.frames[f]).arg(s.referenceBits[f]);
        text += "\n";
    }
    text += QString("\nfaults,fault_rate_percent\n%1,%2\n").arg(pages_.faults).arg(number(pages_.faultRate * 100));
    writeCsv(this, "page-simulation.csv", text);
}

bool MainWindow::smokeTest(const QString& directory) {
    // Exercise the same slots/widgets as interactive use, with no external service.
    if (schedule_.processes.size() != 4 || pages_.steps.size() != 13 || !next_->isEnabled()) return false;
    for (int i = 0; i < 5; ++i) { cpuAlgorithm_->setCurrentIndex(i); if (schedule_.processes.size() != 4) return false; }
    cpuAlgorithm_->setCurrentIndex(3); runCpu();
    if (!directory.isEmpty()) {
        if (!QDir().mkpath(directory)) return false;
        stack_->setCurrentIndex(0); QApplication::processEvents();
        if (!grab().save(directory + "/cpu.png")) return false;
    }
    input_->item(0, 2)->setText("0"); runCpu(); if (!schedule_.processes.empty() || cpuExport_->isEnabled()) return false;
    setCpuExample(); runCpu();
    for (int i = 0; i < 4; ++i) { pageAlgorithm_->setCurrentIndex(i); if (pages_.steps.size() != 13) return false; }
    pageAlgorithm_->setCurrentIndex(2); runPages(); next_->click(); if (step_ != 0) return false;
    previous_->click(); if (step_ != -1) return false;
    play_->click(); if (!timer_.isActive()) return false;
    QMetaObject::invokeMethod(&timer_, "timeout", Qt::DirectConnection); if (step_ != 0) return false;
    stopPlayback(); stepSlider_->setValue(8); if (step_ != 7) return false;
    if (!directory.isEmpty()) {
        findChild<QListWidget*>()->setCurrentRow(1); QApplication::processEvents();
        if (!grab().save(directory + "/paging.png")) return false;
    }
    pageAlgorithm_->setCurrentIndex(1); stepSlider_->setValue(4);
    if (pages_.steps[step_].nextVictim != 1) return false;
    next_->click(); if (pages_.steps[step_].nextVictim != 2) return false;
    previous_->click(); if (pages_.steps[step_].nextVictim != 1) return false;
    stepSlider_->setValue(8);
    if (!directory.isEmpty()) {
        QApplication::processEvents();
        if (!grab().save(directory + "/lru.png")) return false;
    }
    references_->setText("1 x 2"); runPages(); if (!pages_.steps.empty() || play_->isEnabled()) return false;
    references_->setText("1 2 3"); runPages(); frameCount_->setValue(1);
    if (pages_.steps.size() != 3 || !pageExport_->isEnabled() || step_ != -1) return false;
    references_->setText("4 5 4");
    if (!pages_.steps.empty() || !pageRefresh_.isActive()) return false;
    QMetaObject::invokeMethod(&pageRefresh_, "timeout", Qt::DirectConnection);
    if (pages_.steps.size() != 3 || pages_.steps[0].requestedPage != 4) return false;
    return true;
}
