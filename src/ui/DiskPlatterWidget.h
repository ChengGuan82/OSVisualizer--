#pragma once
#include "core/Disk.h"
#include <QtWidgets>
#include <algorithm>
#include <cmath>

// A schematic surface: track zero is outermost; animation never changes the trace.
class DiskPlatterWidget final : public QWidget {
public:
    DiskPlatterWidget(){
        setMinimumSize(360,290);
        timer_.setInterval(16);
        connect(&timer_,&QTimer::timeout,this,[this]{
            const double elapsed=clock_.elapsed();
            double t=std::clamp(elapsed/550.0,0.0,1.0);
            track_=from_+(target_-from_)*t*t*(3-2*t);
            angle_=std::fmod(startAngle_+elapsed*0.09,360.0);
            if(t>=1&&!playing_)timer_.stop();
            update();
        });
    }
    void display(const osv::DiskInput& input,const osv::DiskResult& result,int step){
        maximum_=input.maxTrack;
        target_=step?result.moves.at(step-1).to:input.head;
        kind_=step?result.moves.at(step-1).kind:osv::DiskMoveKind::Request;
        caption_=step?QString("第 %1 步：%2 → %3 · %4").arg(step).arg(result.moves[step-1].from).arg(target_)
            .arg(kind_==osv::DiskMoveKind::Request?"访问请求":kind_==osv::DiskMoveKind::Boundary?"到达边界":"回绕移动"):QString("初始磁头：磁道 %1").arg(input.head);
        from_=track_;startAngle_=angle_;clock_.restart();
        if(step&&isVisible())timer_.start();else settle();
        update();
    }
    void setPlaying(bool playing){
        playing_=playing;
        if(playing&&isVisible()){from_=track_;startAngle_=angle_;clock_.restart();timer_.start();}
        else if(!playing)settle();
    }
    void settle(){timer_.stop();track_=target_;update();}
    int targetTrack()const{return target_;}
protected:
    void hideEvent(QHideEvent* e)override{settle();QWidget::hideEvent(e);}
    void showEvent(QShowEvent* e)override{QWidget::showEvent(e);if(playing_)setPlaying(true);}
    void paintEvent(QPaintEvent*)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(),QColor("#f5f9fa"));
        double r=std::min((height()-76)/2.0,(width()-125)/2.0);
        QPointF c(width()*0.43,height()/2.0+4);
        auto radius=[&](double track){return r*(0.91-0.58*track/std::max(1,maximum_));};
        QRadialGradient metal(c,r);metal.setColorAt(0,QColor("#eef5f7"));metal.setColorAt(0.6,QColor("#d5e3e8"));metal.setColorAt(1,QColor("#b5cbd4"));
        p.setPen(QPen(QColor("#829eac"),2));p.setBrush(metal);p.drawEllipse(c,r,r);
        p.save();p.translate(c);p.rotate(-angle_);
        p.setPen(QPen(QColor("#ffffff"),1));
        for(int i=0;i<12;++i){p.rotate(30);p.drawLine(QPointF(r*0.27,0),QPointF(r*0.98,0));}
        p.setPen(QPen(QColor("#8baeb9"),4));p.drawArc(QRectF(-r*0.96,-r*0.96,r*1.92,r*1.92),10*16,45*16);
        p.setPen(Qt::NoPen);p.setBrush(QColor("#8baeb9"));
        const double a=55*3.141592653589793/180;QPointF tip(r*.96*std::cos(a),-r*.96*std::sin(a));
        p.drawPolygon(QPolygonF{tip,tip+QPointF(9,3),tip+QPointF(2,10)});p.restore();
        p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor("#a0b7c1"),1));
        int rings=std::min(maximum_+1,12);
        for(int i=0;i<rings;++i){double rr=radius(double(i)*maximum_/std::max(1,rings-1));p.drawEllipse(c,rr,rr);}
        QColor active(kind_==osv::DiskMoveKind::Request?"#138b80":"#bf873d");
        p.setPen(QPen(active,2,Qt::DashLine));double dest=radius(target_);p.drawEllipse(c,dest,dest);
        double rr=radius(track_);p.setPen(QPen(active,3));p.drawEllipse(c,rr,rr);
        // Track labels travel with the platter; their text stays upright for readability.
        p.save();
        QFont trackFont=p.font();trackFont.setPixelSize(12);p.setFont(trackFont);
        const double labelAngles[]={180,140,215,105,255};
        int previous=-1;
        for(int i=0;i<5;++i){
            int number=std::min(maximum_,int((static_cast<long long>(maximum_)+1)*i/4));
            if(number==previous)continue;
            previous=number;
            double rad=radius(number),theta=(labelAngles[i]+angle_)*3.141592653589793/180;
            p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor("#7896a3"),1));p.drawEllipse(c,rad,rad);
            QPointF at=c+QPointF(rad*std::cos(theta),-rad*std::sin(theta));
            QString text=QString::number(number);
            double w=p.fontMetrics().horizontalAdvance(text)+10;
            QRectF box(at.x()-w/2,at.y()-9,w,18);
            p.setPen(Qt::NoPen);p.setBrush(QColor("#f5f9fa"));p.drawRoundedRect(box,3,3);
            p.setPen(QColor("#344e5e"));p.drawText(box,Qt::AlignCenter,text);
        }
        QPointF targetLabel=c+QPointF(dest*0.7071,dest*0.7071);
        QString targetText=QString("目标 %1").arg(target_);
        double targetWidth=p.fontMetrics().horizontalAdvance(targetText)+12;
        QRectF targetBox(targetLabel.x()-targetWidth/2,targetLabel.y()-10,targetWidth,20);
        p.setPen(Qt::NoPen);p.setBrush(active);p.drawRoundedRect(targetBox,4,4);
        p.setPen(Qt::white);p.drawText(targetBox,Qt::AlignCenter,targetText);
        p.restore();
        p.setPen(QPen(QColor("#829eac"),2));p.setBrush(QColor("#f4f8fa"));p.drawEllipse(c,r*.24,r*.24);
        p.setBrush(QColor("#6c8796"));p.drawEllipse(c,6,6);
        QPointF head=c+QPointF(rr,0),pivot=c+QPointF(r+42,r*.48);
        p.setPen(QPen(QColor("#6c7f8a"),14,Qt::SolidLine,Qt::RoundCap));p.drawLine(pivot,head);
        p.setPen(QPen(QColor("#c4d1d8"),8,Qt::SolidLine,Qt::RoundCap));p.drawLine(pivot,head);
        p.setPen(QPen(QColor("#344e5e"),2));p.setBrush(QColor("#dbe5e9"));p.drawEllipse(pivot,12,12);
        p.setBrush(active);p.drawRoundedRect(QRectF(head.x()-7,head.y()-5,14,10),2,2);
        p.setPen(QColor("#20313f"));p.drawText(QRectF(12,8,width()-24,24),Qt::AlignCenter,caption_);
        p.drawText(QRectF(c.x()-35,c.y()-20,70,18),Qt::AlignCenter,"主轴");
        p.drawText(QPointF(pivot.x()-14,pivot.y()+30),"磁臂");
        p.drawText(QPointF(head.x()+9,head.y()-12),"磁头");
        p.drawText(QPointF(c.x()-r+4,c.y()-r+15),"盘片 / 同心磁道");
        p.drawText(QRectF(8,height()-32,width()-16,24),Qt::AlignCenter,QString("外圈 0 → 内圈 %1   ·   高亮目标磁道 %2").arg(maximum_).arg(target_));
    }
private:
    QTimer timer_;QElapsedTimer clock_;double track_=53,from_=53,angle_=0,startAngle_=0;
    int maximum_=199,target_=53;bool playing_=false;QString caption_;
    osv::DiskMoveKind kind_=osv::DiskMoveKind::Request;
};
