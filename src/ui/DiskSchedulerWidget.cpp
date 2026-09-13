#include "ui/DiskSchedulerWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <cmath>
using namespace osvui;

class DiskPathChart final : public QWidget {
public:
    void display(const osv::DiskInput& input,const osv::DiskResult& result,int step){
        input_=input;result_=result;step_=step;setMinimumHeight(std::max(210,80+int(result.moves.size()+1)*30));update();
    }
protected:
    void paintEvent(QPaintEvent* event)override{
        QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
        auto x=[&](int track){return 68.0+double(track)*(width()-160)/std::max(1,input_.maxTrack);};
        p.setPen(QColor("#879aa3"));
        for(int k=0;k<=4;++k){int track=int(static_cast<long long>(input_.maxTrack)*k/4);double at=x(track);p.drawLine(QPointF(at,30),QPointF(at,height()-12));p.drawText(QRectF(at-30,4,60,20),Qt::AlignCenter,QString::number(track));}
        p.setPen(QColor("#20313f"));p.drawText(4,51,"起点");
        p.setBrush(QColor("#138b80"));p.drawEllipse(QPointF(x(input_.head),45),5,5);
        p.drawText(QPointF(x(input_.head)+9,49),QString::number(input_.head));
        for(int i=0;i<int(result_.moves.size());++i){
            const auto& m=result_.moves[i];QPointF a(x(m.from),45+i*30),b(x(m.to),75+i*30);
            if(b.y()<event->rect().top()||a.y()>event->rect().bottom())continue;
            bool active=i<step_;QColor color=active?(m.kind==osv::DiskMoveKind::Request?QColor("#138b80"):QColor("#bf873d")):QColor("#d9e0e2");
            p.setPen(QPen(color,active?2.5:1,m.kind==osv::DiskMoveKind::Wrap?Qt::DashLine:Qt::SolidLine));p.drawLine(a,b);
            QPointF v=b-a;double length=std::hypot(v.x(),v.y());QPointF u=v/length,normal(-u.y(),u.x());
            p.setPen(Qt::NoPen);p.setBrush(color);p.drawPolygon(QPolygonF{b,b-u*8+normal*4,b-u*8-normal*4});p.drawEllipse(b,3,3);
            p.setPen(active?QColor("#20313f"):QColor("#aab7bd"));p.drawText(QPointF(4,b.y()+4),QString::number(i+1));
            QString suffix=m.kind==osv::DiskMoveKind::Request?"":m.kind==osv::DiskMoveKind::Boundary?" 边界":" 回绕";
            p.drawText(QPointF(b.x()+8,b.y()+4),QString::number(m.to)+suffix);
        }
    }
private:
    osv::DiskInput input_;osv::DiskResult result_;int step_=0;
};

DiskSchedulerWidget::DiskSchedulerWidget(QWidget* parent):QWidget(parent){
    // Migrate exp/OS-exp7/GUI/DiskScheduler.cpp's input → strategy → result flow.
    // Replace hidden random requests and QString algorithm results with editable
    // requests and pure-C++ trace objects shared by the path, table and comparison.
    auto* root=new QVBoxLayout(this);root->setContentsMargins(28,24,28,24);root->setSpacing(12);
    root->addWidget(label("DISK SCHEDULING  /  04","eyebrow"));root->addWidget(label("磁盘调度实验室","title"));
    root->addWidget(label("基于实验 7 迁移：同一请求序列比较四种策略，逐步观察磁头移动。默认示例可直接运行。"));
    QVBoxLayout* inputs;root->addWidget(card(inputs));auto* row=new QHBoxLayout;
    head_=new QSpinBox;head_->setRange(0,1000000);maximum_=new QSpinBox;maximum_->setRange(1,1000000);
    algorithm_=new QComboBox;algorithm_->addItems({"FCFS","SSTF","SCAN","C-SCAN"});
    direction_=new QComboBox;direction_->addItems({"向右 / 增大","向左 / 减小"});
    row->addWidget(label("磁头"));row->addWidget(head_);row->addWidget(label("最大磁道号"));row->addWidget(maximum_);
    row->addWidget(algorithm_);row->addWidget(direction_);auto* load=button("载入示例");row->addWidget(load);inputs->addLayout(row);
    requests_=new QLineEdit;requests_->setPlaceholderText("磁道请求，空格或逗号分隔；留空表示无请求");inputs->addWidget(requests_);
    inputs->addWidget(label("磁道范围为 0～最大磁道号。SCAN 到边界再反向；C-SCAN 回绕也计入移动量。最后一个请求完成后停止；重复请求分别服务。"));
    status_=label("","stats");root->addWidget(status_);message_=label("");root->addWidget(message_);
    QVBoxLayout* visual;root->addWidget(card(visual),1);visual->addWidget(label("磁头路径 · 横轴为磁道，纵轴为移动步骤","section"));
    auto* split=new QHBoxLayout;auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setMinimumHeight(290);scroll->setMaximumHeight(360);
    chart_=new DiskPathChart;scroll->setWidget(chart_);split->addWidget(scroll,3);
    moves_=table({"步骤","起点 → 终点","距离","类型"});moves_->setMinimumHeight(290);moves_->setMaximumHeight(360);split->addWidget(moves_,2);visual->addLayout(split);
    replay_=new ReplayBar;visual->addWidget(replay_);sequence_=label("");visual->addWidget(sequence_);
    visual->addWidget(label("绿色为已执行请求，橙色为边界移动，橙色虚线为回绕；灰色为未执行路径。上一步或拖动进度会暂停；点击表格行可跳转。"));
    QVBoxLayout* compare;root->addWidget(card(compare));compare->addWidget(label("同组输入 · 四策略对比（完整结果）","section"));
    comparison_=table({"算法","总移动量","每请求平均移动量"});comparison_->setFixedHeight(166);compare->addWidget(comparison_);
    refresh_.setSingleShot(true);refresh_.setInterval(350);connect(&refresh_,&QTimer::timeout,this,[this]{calculate();});
    connect(requests_,&QLineEdit::textChanged,this,[this]{invalidate();refresh_.start();});
    for(auto* spin:{head_,maximum_})connect(spin,&QSpinBox::valueChanged,this,[this]{calculate();});
    connect(algorithm_,&QComboBox::currentIndexChanged,this,[this](int i){direction_->setEnabled(i>=2);calculate();});
    connect(direction_,&QComboBox::currentIndexChanged,this,[this]{calculate();});
    connect(load,&QPushButton::clicked,this,[this]{loadExample();});
    connect(moves_,&QTableWidget::cellClicked,this,[this](int row,int){replay_->seek(row+1);});
    replay_->changed=[this](int step){render(step);};loadExample();
}
void DiskSchedulerWidget::loadExample(){
    QSignalBlocker a(head_),b(maximum_),c(requests_),d(direction_);
    maximum_->setValue(199);head_->setValue(53);requests_->setText("98 183 37 122 14 124 65 67");direction_->setCurrentIndex(0);
    direction_->setEnabled(algorithm_->currentIndex()>=2);calculate();
}
void DiskSchedulerWidget::invalidate(){
    valid_=false;result_={};replay_->setCount(0);moves_->setRowCount(0);comparison_->setRowCount(0);chart_->display({}, {},0);
    status_->setText("等待有效输入");message_->setText("修改后自动重新计算，旧结果已清除。");sequence_->clear();
}
void DiskSchedulerWidget::calculate(){
    refresh_.stop();invalidate();
    try{
        input_={head_->value(),maximum_->value(),numbers(requests_->text()),direction_->currentIndex()==0?osv::Direction::Right:osv::Direction::Left};
        result_=osv::StandardDiskScheduler(static_cast<osv::DiskStrategy>(algorithm_->currentIndex())).run(input_);valid_=true;
        comparison_->setRowCount(4);for(int a=0;a<4;++a){
            auto r=osv::StandardDiskScheduler(static_cast<osv::DiskStrategy>(a)).run(input_);
            cell(comparison_,a,0,algorithm_->itemText(a));cell(comparison_,a,1,QString::number(r.totalMovement));cell(comparison_,a,2,QString::number(r.averageMovement,'f',2));
        }
        moves_->setRowCount(int(result_.moves.size()));for(int i=0;i<int(result_.moves.size());++i){const auto& m=result_.moves[i];
            cell(moves_,i,0,QString::number(i+1));cell(moves_,i,1,QString("%1 → %2").arg(m.from).arg(m.to));cell(moves_,i,2,QString::number(m.distance));
            cell(moves_,i,3,m.kind==osv::DiskMoveKind::Request?"请求":m.kind==osv::DiskMoveKind::Boundary?"边界":"回绕");
        }replay_->setCount(int(result_.moves.size()));
    }catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}
}
void DiskSchedulerWidget::render(int step){
    if(!valid_)return;
    int distance=0;for(int i=0;i<step;++i)distance+=result_.moves[i].distance;
    chart_->display(input_,result_,step);
    status_->setText(QString("当前磁头  %1   ·   已移动  %2   /   全程移动  %3   ·   每请求平均  %4").arg(step?result_.moves[step-1].to:input_.head).arg(distance).arg(result_.totalMovement).arg(QString::number(result_.averageMovement,'f',2)));
    sequence_->setText("完整服务顺序（不含边界与回绕点）："+(result_.sequence.empty()?QString("无请求"):vectorText(result_.sequence)));
    message_->setText(input_.requests.empty()?"没有请求，磁头保持原位，总移动量和平均移动量均为 0。":QString("%1：方向设置仅影响 SCAN / C-SCAN；SSTF 距离相同时按输入顺序。平均值 = 总移动量 ÷ 请求数。").arg(algorithm_->currentText()));
    moves_->clearSelection();if(step){moves_->selectRow(step-1);moves_->scrollToItem(moves_->item(step-1,0));}
}
void DiskSchedulerWidget::showExampleStep(int step){algorithm_->setCurrentIndex(3);loadExample();replay_->seek(step);}
bool DiskSchedulerWidget::smokeTest(){
    for(int a=0;a<4;++a){algorithm_->setCurrentIndex(a);loadExample();if(!valid_||result_.sequence.size()!=8||comparison_->rowCount()!=4)return false;}
    if(result_.totalMovement!=382)return false;
    replay_->seek(2);if(replay_->current()!=2)return false;
    direction_->setCurrentIndex(1);if(result_.totalMovement!=386)return false;
    requests_->setText("");calculate();if(!valid_||result_.totalMovement!=0||!result_.moves.empty())return false;
    requests_->setText("200");calculate();if(valid_||comparison_->rowCount()!=0)return false;
    showExampleStep(7);return true;
}
