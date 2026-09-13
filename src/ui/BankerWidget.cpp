#include "ui/BankerWidget.h"
using namespace osvui;
BankerWidget::BankerWidget(QWidget* parent):QWidget(parent) {
    auto* root=new QVBoxLayout(this);root->setContentsMargins(28,24,28,24);root->setSpacing(12);
    root->addWidget(label("DEADLOCK AVOIDANCE  /  03","eyebrow"));root->addWidget(label("银行家算法实验室","title"));
    root->addWidget(label("基于实验 3 的安全性检查：Need = Max − Allocation。无需初始化，编辑后自动重新计算。"));
    QVBoxLayout* inputs;root->addWidget(card(inputs));
    auto* controls=new QHBoxLayout;
    processCount_=new QSpinBox;processCount_->setRange(1,30);processCount_->setValue(5);
    resourceCount_=new QSpinBox;resourceCount_->setRange(1,8);resourceCount_->setValue(3);
    controls->addWidget(label("进程数"));controls->addWidget(processCount_);controls->addWidget(label("资源种类"));controls->addWidget(resourceCount_);
    example_=new QComboBox;example_->addItems({"经典安全状态","非安全状态","Need 全为 0"});controls->addWidget(example_,1);
    auto* load=button("载入示例");controls->addWidget(load);inputs->addLayout(controls);
    auto* availableRow=new QHBoxLayout;availableRow->addWidget(label("Available"));available_=new QLineEdit;availableRow->addWidget(available_,1);inputs->addLayout(availableRow);
    auto* matrices=new QHBoxLayout;
    allocation_=table({},true);max_=table({},true);need_=table({});
    for(auto entry:std::vector<std::pair<QString,QTableWidget*>>{{"Allocation · 已分配",allocation_},{"Max · 最大需求",max_},{"Need · 自动计算",need_}}){
        auto* column=new QVBoxLayout;column->addWidget(label(entry.first,"section"));entry.second->setMinimumHeight(196);entry.second->setMaximumHeight(220);
        column->addWidget(entry.second);matrices->addLayout(column,1);
    }
    inputs->addLayout(matrices);inputs->addWidget(label("资源按 R0、R1… 对应列输入；进程按 P0、P1… 编号。每项为非负整数，Max 不得小于 Allocation。"));
    status_=label("","stats");root->addWidget(status_);
    QVBoxLayout* trace;root->addWidget(card(trace));trace->addWidget(label("安全性证明 · 逐次检查与释放资源","section"));
    message_=label("");trace->addWidget(message_);sequence_=label("");trace->addWidget(sequence_);
    work_=table({"资源","检查前 Work","当前 Need","完成时释放","检查后 Work"});work_->setFixedHeight(135);trace->addWidget(work_);
    replay_=new ReplayBar;trace->addWidget(replay_);
    trace->addWidget(label("每步检查一个未完成进程；Need ≤ Work 才能假设其完成并释放 Allocation。非安全表示无法保证全部完成，不等于已发生死锁。"));
    history_=table({"步骤","进程","能否完成","Work 变化"});history_->setMinimumHeight(170);root->addWidget(history_,1);
    refresh_.setSingleShot(true);refresh_.setInterval(350);
    connect(&refresh_,&QTimer::timeout,this,[this]{calculate();});
    for(auto* t:{allocation_,max_})connect(t,&QTableWidget::itemChanged,this,[this]{invalidate();refresh_.start();});
    connect(available_,&QLineEdit::textChanged,this,[this]{invalidate();refresh_.start();});
    for(auto* spin:{processCount_,resourceCount_})connect(spin,&QSpinBox::valueChanged,this,[this]{resizeMatrices();calculate();});
    connect(load,&QPushButton::clicked,this,[this]{loadExample();});
    connect(history_,&QTableWidget::cellClicked,this,[this](int row,int){replay_->seek(row+1);});
    replay_->changed=[this](int step){render(step);};loadExample();
}
void BankerWidget::resizeMatrices(){
    const int n=processCount_->value(),m=resourceCount_->value();QStringList cols,rows;
    for(int j=0;j<m;++j)cols<<QString("R%1").arg(j);
    for(int i=0;i<n;++i)rows<<QString("P%1").arg(i);
    for(auto* t:{allocation_,max_,need_}){
        QSignalBlocker blocker(t);t->setRowCount(n);t->setColumnCount(m);t->setHorizontalHeaderLabels(cols);t->setVerticalHeaderLabels(rows);t->verticalHeader()->show();
        t->horizontalHeader()->setMinimumSectionSize(52);
        for(int i=0;i<n;++i)for(int j=0;j<m;++j)if(!t->item(i,j))cell(t,i,j,t==need_?"—":"0");
    }
    // Preserve existing resource quantities when growing/shrinking dimensions.
    auto fields=available_->text().split(QRegularExpression("[\\s,，;；]+"),Qt::SkipEmptyParts);
    while(fields.size()<m)fields<<"0";
    while(fields.size()>m)fields.removeLast();
    const QSignalBlocker blocker(available_);available_->setText(fields.join(" "));
}
void BankerWidget::loadExample(){
    QSignalBlocker a(processCount_),b(resourceCount_),c(available_),d(allocation_),e(max_);
    processCount_->setValue(5);resourceCount_->setValue(3);resizeMatrices();
    available_->setText(example_->currentIndex()==1?"0 0 0":"3 3 2");
    const int alloc[5][3]={{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
    const int maximum[5][3]={{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};
    for(int i=0;i<5;++i)for(int j=0;j<3;++j){cell(allocation_,i,j,QString::number(alloc[i][j]));cell(max_,i,j,QString::number(example_->currentIndex()==2?alloc[i][j]:maximum[i][j]));}
    calculate();
}
osv::BankerInput BankerWidget::input()const{
    osv::BankerInput v;v.available=numbers(available_->text());
    if(int(v.available.size())!=resourceCount_->value())throw std::invalid_argument("Available 数量必须等于资源种类数");
    for(int i=0;i<processCount_->value();++i){
        std::vector<int> a,m;for(int j=0;j<resourceCount_->value();++j){a.push_back(integer(allocation_->item(i,j)->text()));m.push_back(integer(max_->item(i,j)->text()));}
        v.allocation.push_back(a);v.max.push_back(m);
    }return v;
}
void BankerWidget::invalidate(){
    valid_=false;result_={};replay_->setCount(0);history_->setRowCount(0);work_->setRowCount(0);
    status_->setText("等待有效输入");message_->setText("参数变更后自动重新检查；旧轨迹已清除。");sequence_->clear();
    for(int i=0;i<need_->rowCount();++i)for(int j=0;j<need_->columnCount();++j)cell(need_,i,j,"—");
}
void BankerWidget::calculate(){
    refresh_.stop();invalidate();
    try{
        input_=input();result_=osv::BankerAlgorithm().run(input_);valid_=true;
        for(int i=0;i<need_->rowCount();++i)for(int j=0;j<need_->columnCount();++j)cell(need_,i,j,QString::number(result_.need[i][j]));
        status_->setText(result_.safe?"SAFE · 存在安全序列，可以保证所有进程依次完成":"UNSAFE · 无法保证所有进程完成（不等于已经死锁）");
        history_->setRowCount(int(result_.steps.size()));
        for(int i=0;i<int(result_.steps.size());++i){const auto& s=result_.steps[i];cell(history_,i,0,QString::number(i+1));cell(history_,i,1,QString("P%1").arg(s.processId));cell(history_,i,2,s.canFinish?"可完成":"暂不能完成");cell(history_,i,3,vectorText(s.workBefore)+" → "+vectorText(s.workAfter));}
        replay_->setCount(int(result_.steps.size()));
    }catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}
}
void BankerWidget::render(int step){
    if(!valid_)return;
    std::vector<int> prefix;for(int i=0;i<step;++i)if(result_.steps[i].canFinish)prefix.push_back(result_.steps[i].processId);
    sequence_->setText(QString("已模拟完成：%1\n完整检测结果：%2%3").arg(prefix.empty()?"无":vectorText(prefix,"P")).arg(result_.safe?"安全序列 ":"可完成前缀 ").arg(vectorText(result_.safeSequence,"P")));
    if(!result_.safe)sequence_->setText(sequence_->text()+"；未完成 "+vectorText(result_.unfinished,"P"));
    work_->setRowCount(resourceCount_->value());
    const auto* s=step?&result_.steps[step-1]:nullptr;
    for(int j=0;j<resourceCount_->value();++j){
        cell(work_,j,0,QString("R%1").arg(j));cell(work_,j,1,QString::number(s?s->workBefore[j]:input_.available[j]));
        cell(work_,j,2,s?QString::number(result_.need[s->processId][j]):"—");cell(work_,j,3,s&&s->canFinish?QString::number(input_.allocation[s->processId][j]):"0");
        cell(work_,j,4,QString::number(s?s->workAfter[j]:input_.available[j]));
        if(s&&result_.need[s->processId][j]>s->workBefore[j])work_->item(j,2)->setBackground(QColor("#f8ddd7"));
    }
    message_->setText(s?QString("检查 P%1：%2。Work [%3] → [%4]").arg(s->processId).arg(s->canFinish?"Need ≤ Work，假设完成并释放资源":"Need 超过 Work，跳过此进程").arg(vectorText(s->workBefore),vectorText(s->workAfter)):"第 0 步：Work = Available；点击下一步开始检查。顶部为完整检测结论，下方为当前回放状态。");
    history_->clearSelection();if(step){history_->selectRow(step-1);history_->scrollToItem(history_->item(step-1,0));}
}
void BankerWidget::showExampleStep(int step){example_->setCurrentIndex(0);loadExample();replay_->seek(step);}
bool BankerWidget::smokeTest(){
    showExampleStep(2);if(!valid_||!result_.safe||work_->item(0,4)->text()!="5")return false;
    replay_->seek(1);if(work_->item(0,4)->text()!="3")return false;
    for(int i=1;i<3;++i){example_->setCurrentIndex(i);loadExample();if(!valid_||result_.safe!=(i==2))return false;}
    max_->item(0,0)->setText("-1");calculate();if(valid_||need_->item(0,0)->text()!="—")return false;
    showExampleStep(2);return true;
}
