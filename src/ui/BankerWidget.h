#pragma once
#include "core/Banker.h"
#include "ui/SimulationUi.h"
class BankerFlow;
class BankerWidget final : public QWidget {
public:
    explicit BankerWidget(QWidget* parent=nullptr);
    bool smokeTest();
    void showExampleStep(int index);
private:
    void resizeMatrices();
    void rebuildVisuals();
    void renderVisuals(int step);
    BankerFlow* flow_;
    QGridLayout* cards_;
    QHBoxLayout* bars_;
    std::vector<QLabel*> texts_;
    std::vector<QProgressBar*> gauges_;
    std::vector<QVariantAnimation*> animations_;
    void loadExample();
    void invalidate();
    void calculate();
    void render(int step);
    osv::BankerInput input()const;
    QSpinBox *processCount_,*resourceCount_;
    QLineEdit* available_;
    QComboBox* example_;
    QTableWidget *allocation_,*max_,*need_,*work_,*history_;
    QLabel *status_,*message_,*sequence_;
    osvui::ReplayBar* replay_;
    QTimer refresh_;
    osv::BankerInput input_;
    osv::BankerResult result_;
    bool valid_=false;
};
