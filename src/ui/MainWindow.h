#pragma once
#include <QMainWindow>
#include <QTimer>
#include "core/Algorithms.h"
class QTableWidget;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QLabel;
class QPushButton;
class QSlider;
class QStackedWidget;
class GanttChart;
class FrameChart;
class BankerWidget;
class DiskSchedulerWidget;

class MainWindow final : public QMainWindow {
public:
    MainWindow();
    bool smokeTest(const QString& screenshotDirectory = {});
private:
    QWidget* buildCpu();
    QWidget* buildPages();
    void runCpu();
    void runPages();
    void renderStep();
    void stopPlayback();
    void invalidateCpu();
    void invalidatePages();
    void exportCpu();
    void exportPages();
    void setCpuExample();
    std::vector<osv::Process> readProcesses() const;
    QStackedWidget* stack_{};
    BankerWidget* banker_{};
    DiskSchedulerWidget* disk_{};
    QTableWidget *input_{}, *cpuResults_{}, *cpuCompare_{}, *pageHistory_{}, *pageCompare_{};
    QComboBox *cpuAlgorithm_{}, *pageAlgorithm_{};
    QSpinBox *quantum_{}, *frameCount_{};
    QLineEdit* references_{};
    QLabel *cpuStats_{}, *cpuMessage_{}, *pageStats_{}, *pageMessage_{}, *stepLabel_{};
    QLabel *cpuHelp_{}, *pageHelp_{}, *lruOrder_{};
    QPushButton *play_{}, *previous_{}, *next_{}, *cpuExport_{}, *pageExport_{};
    QSlider* stepSlider_{};
    GanttChart* gantt_{};
    FrameChart* frames_{};
    QTimer timer_;
    QTimer cpuRefresh_, pageRefresh_;
    osv::ScheduleResult schedule_;
    osv::PageResult pages_;
    int step_ = -1;
};
