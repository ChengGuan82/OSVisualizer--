#pragma once
#include <QtWidgets>
#include <functional>
#include <algorithm>
#include <stdexcept>

namespace osvui {
inline QLabel* label(const QString& text, const char* style="muted") {
    auto* widget=new QLabel(text); widget->setObjectName(style); widget->setWordWrap(true); return widget;
}
inline QPushButton* button(const QString& text) { auto* b=new QPushButton(text); b->setCursor(Qt::PointingHandCursor); return b; }
inline QFrame* card(QVBoxLayout*& layout) {
    auto* frame=new QFrame; frame->setObjectName("card"); layout=new QVBoxLayout(frame);
    layout->setContentsMargins(18,14,18,14); layout->setSpacing(10); return frame;
}
inline QTableWidget* table(const QStringList& headers, bool editable=false) {
    auto* t=new QTableWidget(0,headers.size()); t->setHorizontalHeaderLabels(headers);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->verticalHeader()->hide(); t->setAlternatingRowColors(true);
    if(!editable)t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    return t;
}
inline void cell(QTableWidget* t,int r,int c,const QString& text) {
    auto* item=new QTableWidgetItem(text); item->setTextAlignment(Qt::AlignCenter); t->setItem(r,c,item);
}
inline QString vectorText(const std::vector<int>& values, const QString& prefix={}) {
    QStringList parts; for(int value:values)parts<<prefix+QString::number(value); return parts.join("  ");
}
inline int integer(const QString& text) {
    bool ok=false; int value=text.trimmed().toInt(&ok);
    if(!ok)throw std::invalid_argument("请输入有效整数，不能留空或包含文字");
    return value;
}
inline std::vector<int> numbers(const QString& text) {
    std::vector<int> values;
    for(const auto& token:text.split(QRegularExpression("[\\s,，;；]+"),Qt::SkipEmptyParts))values.push_back(integer(token));
    return values;
}
inline QWidget* scrollPage(QWidget* page) {
    auto* scroll=new QScrollArea; scroll->setWidgetResizable(true); scroll->setWidget(page); return scroll;
}
// Shared playback owns only a snapshot index, never algorithm state.
class ReplayBar final : public QWidget {
public:
    std::function<void(int)> changed;
    explicit ReplayBar(QWidget* parent=nullptr):QWidget(parent) {
        auto* row=new QHBoxLayout(this); row->setContentsMargins(0,0,0,0);
        previous_=button("← 上一步"); play_=button("自动执行"); next_=button("下一步 →");
        play_->setObjectName("primary"); slider_=new QSlider(Qt::Horizontal); position_=label("");
        row->addWidget(previous_); row->addWidget(play_); row->addWidget(next_); row->addWidget(slider_,1); row->addWidget(position_);
        timer_.setInterval(800);
        connect(previous_,&QPushButton::clicked,this,[this]{seek(current_-1);});
        connect(next_,&QPushButton::clicked,this,[this]{seek(current_+1);});
        connect(slider_,&QSlider::valueChanged,this,[this](int v){seek(v);});
        connect(play_,&QPushButton::clicked,this,[this]{
            if(timer_.isActive()){stop();return;} if(!count_)return;
            if(current_==count_)seek(0);
            timer_.start(); play_->setText("暂停");
        });
        connect(&timer_,&QTimer::timeout,this,[this]{updateIndex(current_+1);});
        setCount(0);
    }
    void setCount(int n) {stop();count_=n;updateIndex(0);}
    void seek(int index){stop();updateIndex(index);}
    void stop(){timer_.stop();play_->setText("自动执行");}
    int current()const{return current_;}
protected:
    void hideEvent(QHideEvent* event)override{stop();QWidget::hideEvent(event);}
private:
    void updateIndex(int value){
        current_=std::clamp(value,0,count_);
        {QSignalBlocker blocker(slider_);slider_->setRange(0,count_);slider_->setValue(current_);}
        previous_->setEnabled(current_>0);next_->setEnabled(current_<count_);play_->setEnabled(count_>0);slider_->setEnabled(count_>0);
        position_->setText(QString("%1 / %2 步").arg(current_).arg(count_));
        if(current_==count_)stop();
        if(changed)changed(current_);
    }
    int current_=0,count_=0; QTimer timer_;
    QPushButton *previous_,*play_,*next_; QSlider* slider_; QLabel* position_;
};
}
