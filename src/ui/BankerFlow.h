#pragma once
#include <QtWidgets>

// A schematic transfer lane. Each token is a resource quantity, not one unit.
class BankerFlow final : public QWidget {
public:
    BankerFlow() {
        setMinimumHeight(100);
        motion_.setDuration(650);
        motion_.setStartValue(0.0);motion_.setEndValue(1.0);
        motion_.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&motion_, &QVariantAnimation::valueChanged, this,
                [this](const QVariant& value){progress_=value.toReal();update();});
    }
    void clear(){motion_.stop();tokens_.clear();process_=-1;update();}
    void display(int process, int phase, bool possible, const QStringList& tokens) {
        motion_.stop();process_=process;phase_=phase;possible_=possible;
        tokens_=tokens;progress_=0;
        setMinimumHeight(std::max(100,42+int(tokens.size())*24));
        if(possible && phase>=3)motion_.start();
        update();
    }
protected:
    void hideEvent(QHideEvent* event) override {motion_.stop();QWidget::hideEvent(event);}
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        const QRectF pool(8,12,120,height()-24), process(width()-128,12,120,height()-24);
        p.setPen(Qt::NoPen);p.setBrush(QColor("#e4f3ef"));p.drawRoundedRect(pool,10,10);
        p.setBrush(QColor("#edf1f7"));p.drawRoundedRect(process,10,10);
        p.setPen(QColor("#244451"));p.drawText(pool,Qt::AlignCenter,"系统资源池");
        p.drawText(process,Qt::AlignCenter,process_<0?"等待进程":QString("P%1").arg(process_));
        const qreal left=140,right=width()-140;
        if(process_<0){p.drawText(QRectF(left,0,right-left,height()),Qt::AlignCenter,"选中 → 比较 → 借出 → 完成归还");return;}
        if(!possible_ || phase_<3){
            p.drawText(QRectF(left,0,right-left,height()),Qt::AlignCenter,
                phase_==1?"① 选中进程":phase_==2?"② 逐项比较剩余需求与可用资源":"资源不足 · 跳过，资源池不变");return;
        }
        p.drawText(QRectF(left,0,right-left,24),Qt::AlignCenter,phase_==3?"③ 借出剩余需求 Need →":"④ ← 归还全部持有量 Allocation + Need");
        if(tokens_.isEmpty()){p.drawText(QRectF(left,24,right-left,height()-24),Qt::AlignCenter,"本阶段转移量为 0");return;}
        for(int j=0;j<tokens_.size();++j){
            qreal y=36+j*24;
            p.setPen(QPen(QColor("#c4d9d5"),2));p.drawLine(QPointF(left,y),QPointF(right,y));
            const qreal t=phase_==3?progress_:1-progress_;
            const qreal x=left+(right-left-128)*t;
            QRectF badge(x,y-10,128,21);p.setPen(Qt::NoPen);p.setBrush(QColor("#168f80"));p.drawRoundedRect(badge,6,6);
            p.setPen(Qt::white);p.drawText(badge,Qt::AlignCenter,tokens_[j]);
        }
    }
private:
    QVariantAnimation motion_;
    QStringList tokens_;
    int process_=-1,phase_=0;
    bool possible_=false;
    qreal progress_=0;
};
