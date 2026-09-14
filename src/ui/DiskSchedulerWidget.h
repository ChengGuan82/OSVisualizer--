#pragma once
#include "core/Disk.h"
#include "ui/SimulationUi.h"
class DiskPathChart;
class DiskPlatterWidget;
class DiskSchedulerWidget final : public QWidget {
public:
    explicit DiskSchedulerWidget(QWidget* parent=nullptr);
    bool smokeTest();
    void showExampleStep(int step);
private:
    void loadExample();
    void calculate();
    void invalidate();
    void render(int step);
    QSpinBox *head_,*maximum_;
    QLineEdit* requests_;
    QComboBox *algorithm_,*direction_;
    QLabel *status_,*message_,*sequence_;
    QTableWidget *comparison_,*moves_;
    DiskPathChart* chart_;
    DiskPlatterWidget* platter_;
    QScrollArea* pathScroll_;
    osvui::ReplayBar* replay_;
    QTimer refresh_;
    osv::DiskInput input_;
    osv::DiskResult result_;
    bool valid_=false;
};
