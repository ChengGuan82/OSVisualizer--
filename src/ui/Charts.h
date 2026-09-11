#pragma once
#include <QWidget>
#include "core/Algorithms.h"

class GanttChart final : public QWidget {
public:
    explicit GanttChart(QWidget* parent = nullptr);
    void setResult(const osv::ScheduleResult& result);
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    void layoutArrivals();
    double timePosition(int time) const;
    struct ArrivalLabel { QString text; double x; QRectF box; };
    std::vector<osv::Slice> slices_;
    std::vector<osv::Process> arrivals_;
    std::vector<ArrivalLabel> labels_;
};

class FrameChart final : public QWidget {
public:
    explicit FrameChart(QWidget* parent = nullptr);
    void setStep(const osv::PageStep* step, int count, osv::Replacement strategy);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    osv::PageStep step_{};
    int count_ = 3;
    bool active_ = false, clock_ = false, lru_ = false;
};
