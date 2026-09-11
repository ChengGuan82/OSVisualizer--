#include "ui/Charts.h"
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainterPath>
#include <map>
#include <cmath>

namespace {
QColor processColor(int pid) {
    static const QColor colors[] = {"#138b80", "#5779ce", "#c99547", "#a377af", "#ce7968", "#679aad"};
    return pid < 0 ? QColor("#b3bbc2") : colors[pid % 6];
}
}
GanttChart::GanttChart(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(130); setAutoFillBackground(true);
    auto colors = palette(); colors.setColor(QPalette::Window, Qt::white); setPalette(colors);
}
void GanttChart::setResult(const osv::ScheduleResult& result) {
    slices_ = result.timeline;
    arrivals_ = result.arrivals;
    // Equal-width cells preserve tiny slices; every exact duration is labeled.
    setMinimumWidth(std::max(650, int(slices_.size()) * 96 + 48));
    layoutArrivals();
    update();
}
double GanttChart::timePosition(int time) const {
    const double cell = double(width() - 48) / slices_.size();
    auto it = std::upper_bound(slices_.begin(), slices_.end(), time,
        [](int t, const osv::Slice& s) { return t < s.end; });
    if (it == slices_.end()) return width() - 24;
    return 24 + (std::distance(slices_.begin(), it) + double(time - it->start) / (it->end - it->start)) * cell;
}
void GanttChart::layoutArrivals() {
    labels_.clear();
    if (slices_.empty()) { setMinimumHeight(160); return; }
    std::map<int, QStringList> grouped;
    for (const auto& process : arrivals_) grouped[process.arrival] << QString("P%1").arg(process.pid);
    std::vector<double> laneEnds;
    int bottom = 150;
    for (const auto& [time, pids] : grouped) {
        // Split large simultaneous-arrival groups into separate non-overlapping labels.
        for (int start = 0; start < pids.size(); start += 4) {
            QString text = QString("t=%1  %2 到达").arg(time).arg(pids.mid(start, 4).join(" / "));
            double x = timePosition(time);
            double w = fontMetrics().horizontalAdvance(text) + 16;
            double left = std::clamp(x - w / 2, 8.0, std::max(8.0, width() - w - 8));
            size_t lane = 0;
            while (lane < laneEnds.size() && left < laneEnds[lane] + 12) ++lane;
            if (lane == laneEnds.size()) laneEnds.push_back(0);
            laneEnds[lane] = left + w;
            QRectF box(left, 122 + lane * 29, w, 24);
            labels_.push_back({text, x, box});
            bottom = std::max(bottom, int(box.bottom()) + 12);
        }
    }
    setMinimumHeight(bottom);
}
void GanttChart::resizeEvent(QResizeEvent* event) { QWidget::resizeEvent(event); layoutArrivals(); }
void GanttChart::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor("#6a7681"));
    if (slices_.empty()) { p.drawText(rect(), Qt::AlignCenter, "运行模拟后，在这里查看执行轨迹"); return; }
    const double cell = double(width() - 48) / slices_.size();
    int first = std::max(0, int((event->rect().left() - 24) / cell));
    int last = std::min(int(slices_.size()), int(event->rect().right() / cell) + 1);
    for (int i = first; i < last; ++i) {
        const auto& s = slices_[i];
        QRectF r(24 + i * cell, 24, cell - 4, 60);
        p.setPen(Qt::NoPen); p.setBrush(processColor(s.pid)); p.drawRoundedRect(r, 8, 8);
        p.setPen(Qt::white);
        p.drawText(r.adjusted(0, 4, 0, -25), Qt::AlignCenter, s.pid < 0 ? "空闲" : QString("P%1").arg(s.pid));
        p.drawText(r.adjusted(0, 30, 0, -2), Qt::AlignCenter, QString("Δt = %1").arg(s.end - s.start));
        p.setPen(QColor("#6a7681"));
        p.drawText(QRectF(r.x(), 92, cell, 20), Qt::AlignLeft, QString::number(s.start));
    }
    p.drawText(QRectF(width() - 62, 92, 40, 20), Qt::AlignRight, QString::number(slices_.back().end));
    for (const auto& marker : labels_) {
        if (!marker.box.intersects(event->rect()) && (marker.x < event->rect().left() || marker.x > event->rect().right())) continue;
        p.setPen(QPen(QColor("#ac7130"), 1, Qt::DashLine));
        p.drawLine(QPointF(marker.x, 85), QPointF(marker.x, marker.box.top()));
        p.setPen(Qt::NoPen); p.setBrush(QColor("#ac7130"));
        p.drawPolygon(QPolygonF{QPointF(marker.x, 84), QPointF(marker.x - 4, 91), QPointF(marker.x + 4, 91)});
    }
    for (const auto& marker : labels_) {
        if (!marker.box.intersects(event->rect())) continue;
        p.setPen(Qt::NoPen); p.setBrush(QColor("#fbefdd")); p.drawRoundedRect(marker.box, 5, 5);
        p.setPen(QColor("#8b5d25")); p.drawText(marker.box, Qt::AlignCenter, marker.text);
    }
}
FrameChart::FrameChart(QWidget* parent) : QWidget(parent) { setMinimumHeight(230); }
void FrameChart::setStep(const osv::PageStep* step, int count, osv::Replacement strategy) {
    active_ = step != nullptr; count_ = count; clock_ = strategy == osv::Replacement::CLOCK; lru_ = strategy == osv::Replacement::LRU;
    setMinimumHeight(clock_ ? (count > 8 ? 335 : 185) : (count > 8 ? 245 : 155));
    if (step) step_ = *step;
    update();
}
void FrameChart::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const int columns = std::min(count_, 8);
    double cell = std::min(clock_ ? 128.0 : 108.0, double(width() - (clock_ ? 64 : 40)) / columns);
    auto frameRect = [&](int i) {
        return QRectF((clock_ ? 32 : 20) + (i % columns) * cell,
                      24 + (i / columns) * (clock_ ? 146 : 106), cell - (clock_ ? 28 : 10), 76);
    };
    if (clock_) {
        // Draw behind the frames: numbered slots form one directed cyclic list.
        // Separate the row transition from the outer last-to-first return lane.
        const QColor ringColor("#7ca69f");
        const double right = frameRect(columns - 1).right();
        for (int i = 0; i < count_; ++i) {
            const QRectF from = frameRect(i), to = frameRect((i + 1) % count_);
            QPainterPath link(QPointF(from.right(), from.center().y()));
            if (i == count_ - 1) {
                link.lineTo(right + 18, from.center().y());
                link.lineTo(right + 18, 8);
                link.lineTo(10, 8);
                link.lineTo(10, to.center().y());
            } else if ((i + 1) % columns == 0) {
                const double betweenRows = from.top() + 120;
                link.lineTo(right + 8, from.center().y());
                link.lineTo(right + 8, betweenRows);
                link.lineTo(20, betweenRows);
                link.lineTo(20, to.center().y());
            }
            link.lineTo(to.left(), to.center().y());
            p.setPen(QPen(ringColor, 1.6, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush); p.drawPath(link);
            p.setPen(Qt::NoPen); p.setBrush(ringColor);
            const QPointF tip(to.left() - 2, to.center().y());
            p.drawPolygon(QPolygonF{tip, tip + QPointF(-6, -4), tip + QPointF(-6, 4)});
        }
        p.setPen(QColor("#6a7681"));
        const int captionY = count_ > 8 ? 288 : 142;
        p.drawText(QRectF(20, captionY, width() - 40, 24), Qt::AlignLeft | Qt::AlignVCenter,
                   "虚线箭头：Frame 1 → Frame 2 → … → Frame 1；指针沿环循环扫描");
    }
    for (int i = 0; i < count_; ++i) {
        QRectF r = frameRect(i);
        bool touched = active_ && i == step_.touchedFrame;
        bool victim = lru_ && active_ && i == step_.nextVictim;
        p.setPen(QPen(victim ? QColor("#ac7130") : touched ? QColor("#138b80") : QColor("#dce3e6"), touched || victim ? 2 : 1));
        p.setBrush(victim ? QColor("#fbefdd") : touched ? QColor("#e2f3ed") : QColor("#f5f7f8")); p.drawRoundedRect(r, 9, 9);
        p.setPen(QColor("#6a7681")); p.drawText(r.adjusted(0, 5, 0, -46), Qt::AlignCenter, QString("FRAME %1").arg(i + 1));
        QFont f = font(); f.setPixelSize(23); f.setBold(true); p.setFont(f); p.setPen(QColor("#20313f"));
        p.drawText(r.adjusted(0, 24, 0, -13), Qt::AlignCenter, active_ && step_.frames[i] >= 0 ? QString::number(step_.frames[i]) : "—");
        p.setFont(font());
        if (clock_) p.drawText(r.adjusted(0, 57, 0, 0), Qt::AlignCenter, QString("R = %1").arg(active_ ? step_.referenceBits[i] : 0));
        if (clock_ && i == (active_ ? step_.hand : 0)) {
            p.setPen(QColor("#138b80")); p.drawText(r.adjusted(-8, 78, 8, 26), Qt::AlignCenter, "↑ 下次扫描");
        }
        if (lru_) {
            int freeFrame = active_ ? int(std::distance(step_.frames.begin(), std::find(step_.frames.begin(), step_.frames.end(), -1))) : 0;
            if (victim || i == freeFrame) {
                p.setPen(QColor("#ac7130"));
                p.drawText(r.adjusted(-8, 78, 8, 26), Qt::AlignCenter, victim ? "↑ 缺页时换出" : "↑ 空闲优先");
            }
        }
    }
}
