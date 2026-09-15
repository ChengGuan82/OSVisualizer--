#include "ui/BankerWidget.h"
#include "ui/BankerFlow.h"
using namespace osvui;
BankerWidget::BankerWidget(QWidget* parent):QWidget(parent) {
    auto* root=new QVBoxLayout(this);root->setSizeConstraint(QLayout::SetMinimumSize);root->setContentsMargins(28,24,28,24);root->setSpacing(12);
    root->addWidget(label("DEADLOCK AVOIDANCE  /  03","eyebrow"));root->addWidget(label("银行家算法实验室","title"));
    root->addWidget(label("基于实验 3 的安全性检查：Need = Max − Allocation。无需初始化，编辑后自动重新计算。"));
    QVBoxLayout* inputs;root->addWidget(card(inputs));inputs->setSizeConstraint(QLayout::SetMinimumSize);
    auto* controls=new QHBoxLayout;
    processCount_=new QSpinBox;processCount_->setRange(1,30);processCount_->setValue(5);
    resourceCount_=new QSpinBox;resourceCount_->setRange(1,8);resourceCount_->setValue(3);
    controls->addWidget(label("进程数"));controls->addWidget(processCount_);controls->addWidget(label("资源种类"));controls->addWidget(resourceCount_);
    example_=new QComboBox;example_->addItems({"经典安全状态","非安全状态","Need 全为 0"});controls->addWidget(example_,1);
    auto* load=button("载入示例");controls->addWidget(load);inputs->addLayout(controls);
    auto* availableRow=new QHBoxLayout;availableRow->addWidget(label("初始可用资源"));available_=new QLineEdit;availableRow->addWidget(available_,1);inputs->addLayout(availableRow);
    allocation_=table({},true);max_=table({},true);need_=table({});
    for(auto* t:{allocation_,max_,need_}){t->setParent(this);t->hide();}
    inputs->addWidget(label("系统资源池 · 统一刻度：浅色总量 / 亮色当前模拟可用量", "section"));
    bars_=new QHBoxLayout;inputs->addLayout(bars_);
    flow_=new BankerFlow;inputs->addWidget(flow_);
    cards_=new QGridLayout;cards_->setSpacing(12);inputs->addLayout(cards_);
    status_=label("","stats");root->addWidget(status_);
    QVBoxLayout* trace;root->addWidget(card(trace));trace->addWidget(label("安全性证明 · 逐次检查与释放资源","section"));
    message_=label("");trace->addWidget(message_);sequence_=label("");trace->addWidget(sequence_);
    work_=table({"资源","检查前 Work","当前 Need","完成时释放","检查后 Work"});work_->setParent(this);work_->hide();
    replay_=new ReplayBar;trace->addWidget(replay_);
    trace->addWidget(label("每次检查分四阶段；借出 Need 后归还 Allocation + Need，净增加原 Allocation。非安全表示无法保证全部完成，不等于已发生死锁。"));
    history_=table({"步骤","进程","能否完成","Work 变化"});history_->setParent(this);history_->hide();root->addStretch();
    refresh_.setSingleShot(true);refresh_.setInterval(350);
    connect(&refresh_,&QTimer::timeout,this,[this]{calculate();});
    for(auto* t:{allocation_,max_})connect(t,&QTableWidget::itemChanged,this,[this]{invalidate();refresh_.start();});
    connect(available_,&QLineEdit::textChanged,this,[this]{invalidate();refresh_.start();});
    for(auto* spin:{processCount_,resourceCount_})connect(spin,&QSpinBox::valueChanged,this,[this]{resizeMatrices();calculate();});
    connect(load,&QPushButton::clicked,this,[this]{loadExample();});
    connect(history_,&QTableWidget::cellClicked,this,[this](int row,int){replay_->seek((row+1)*4);});
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
    flow_->clear();valid_=false;result_={};replay_->setCount(0);history_->setRowCount(0);work_->setRowCount(0);
    for(auto* a:animations_)a->stop();
    for(auto* g:gauges_){g->setValue(0);g->parentWidget()->findChild<QLabel*>()->setText("等待有效输入");}
    for(auto* t:texts_)t->setText("等待有效输入");
    status_->setText("等待有效输入");message_->setText("参数变更后自动重新检查；旧轨迹已清除。");sequence_->clear();
    for(int i=0;i<need_->rowCount();++i)for(int j=0;j<need_->columnCount();++j)cell(need_,i,j,"—");
}
void BankerWidget::calculate(){
    refresh_.stop();invalidate();
    try{
        input_=input();result_=osv::BankerAlgorithm().run(input_);valid_=true;rebuildVisuals();
        for(int i=0;i<need_->rowCount();++i)for(int j=0;j<need_->columnCount();++j)cell(need_,i,j,QString::number(result_.need[i][j]));
        status_->setText(result_.safe?"SAFE · 存在安全序列，可以保证所有进程依次完成":"UNSAFE · 无法保证所有进程完成（不等于已经死锁）");
        history_->setRowCount(int(result_.steps.size()));
        for(int i=0;i<int(result_.steps.size());++i){const auto& s=result_.steps[i];cell(history_,i,0,QString::number(i+1));cell(history_,i,1,QString("P%1").arg(s.processId));cell(history_,i,2,s.canFinish?"可完成":"暂不能完成");cell(history_,i,3,vectorText(s.workBefore)+" → "+vectorText(s.workAfter));}
        replay_->setCount(int(result_.steps.size())*4);
    }catch(const std::exception& e){message_->setText(QString::fromUtf8(e.what()));}
}
void BankerWidget::rebuildVisuals(){
    for(auto* a:animations_)delete a;
    animations_.clear();gauges_.clear();texts_.clear();
    for(auto* layout:std::vector<QLayout*>{cards_,bars_})while(auto* item=layout->takeAt(0)){delete item->widget();delete item;}
    int largest=1;
    for(int j=0;j<resourceCount_->value();++j){int total=input_.available[j];for(const auto& row:input_.allocation)total+=row[j];largest=std::max(largest,total);}
    for(int j=0;j<resourceCount_->value();++j){
        int total=input_.available[j];for(const auto& row:input_.allocation)total+=row[j];
        auto* frame=new QWidget;auto* column=new QVBoxLayout(frame);frame->setMinimumHeight(175);auto* gauge=new QProgressBar;
        gauge->setOrientation(Qt::Vertical);gauge->setRange(0,std::max(1,total));gauge->setValue(input_.available[j]);
        gauge->setFixedSize(48,std::max(1,int(120.0*total/largest)));gauge->setTextVisible(false);
        gauge->setStyleSheet("QProgressBar{background:#dbe8e7;border:0;border-radius:5px;}QProgressBar::chunk{background:#16a394;border-radius:5px;}");
        column->addStretch();column->addWidget(gauge,0,Qt::AlignHCenter);auto* caption=label("");caption->setAlignment(Qt::AlignCenter);column->addWidget(caption);bars_->addWidget(frame,1);gauges_.push_back(gauge);
        auto* animation=new QVariantAnimation(this);animation->setDuration(450);animation->setEasingCurve(QEasingCurve::InOutCubic);
        connect(animation,&QVariantAnimation::valueChanged,gauge,[gauge](const QVariant& value){gauge->setValue(value.toInt());});animations_.push_back(animation);
    }
    for(int i=0;i<processCount_->value();++i){
        QVBoxLayout* column;auto* frame=card(column);auto* text=label("");text->setTextFormat(Qt::RichText);column->addWidget(text);texts_.push_back(text);
        auto* edit=button(QString("编辑 P%1").arg(i));column->addWidget(edit);cards_->addWidget(frame,i/3,i%3);
        connect(edit,&QPushButton::clicked,this,[this,i]{
            replay_->stop();QDialog dialog(this);dialog.setWindowTitle(QString("P%1 · 资源设置").arg(i));auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;layout->addLayout(form);
            std::vector<QSpinBox*> allocated,maximum;
            for(int j=0;j<resourceCount_->value();++j){auto* a=new QSpinBox;auto* m=new QSpinBox;a->setRange(0,1000000);m->setRange(0,1000000);a->setValue(integer(allocation_->item(i,j)->text()));m->setValue(integer(max_->item(i,j)->text()));form->addRow(QString("R%1 已分配").arg(j),a);form->addRow(QString("R%1 最大需求").arg(j),m);allocated.push_back(a);maximum.push_back(m);}
            auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
            if(dialog.exec()!=QDialog::Accepted)return;
            {QSignalBlocker a(allocation_),m(max_);for(int j=0;j<resourceCount_->value();++j){cell(allocation_,i,j,QString::number(allocated[j]->value()));cell(max_,i,j,QString::number(maximum[j]->value()));}}
            invalidate();refresh_.start();
        });
    }
}
// Four deterministic snapshots per algorithm check. Seeking never depends on
// animation timing: the bars and cards always show the selected snapshot.
void BankerWidget::render(int step){
    if(!valid_)return;
    renderVisuals(step);
}
void BankerWidget::renderVisuals(int step){
    const int phase=step?(step-1)%4+1:0;
    const int completed=step/4;
    const auto* current=step?&result_.steps[(step-1)/4]:nullptr;
    std::vector<int> pool=current?current->workBefore:input_.available;
    if(current&&phase==3&&current->canFinish)
        for(int j=0;j<resourceCount_->value();++j)pool[j]-=result_.need[current->processId][j];
    if(current&&phase==4)pool=current->workAfter;
    std::vector<bool> done(processCount_->value(),false);
    QStringList sequence;
    for(int k=0;k<completed;++k)if(result_.steps[k].canFinish){done[result_.steps[k].processId]=true;sequence<<QString("P%1").arg(result_.steps[k].processId);}
    const bool end=step==int(result_.steps.size())*4;
    const bool stuck=end&&!result_.safe;
    status_->setText(end?(result_.safe?"SAFE · 已找到安全序列":"UNSAFE · 所有剩余进程都缺少资源，无法继续（不等于已经死锁）"):
        QString("正在寻找安全序列 · 已完成 %1 / %2 个进程").arg(sequence.size()).arg(processCount_->value()));
    QString track="<b>安全序列轨道</b><br>";
    for(int i=0;i<processCount_->value();++i){
        if(i)track+=" → ";
        track+=i<sequence.size()?QString("<span style='background:#d9f0e8;color:#12735d;font-size:18px'> %1 </span>").arg(sequence[i]):QString("<span style='color:#94a5ad;font-size:18px'> [ %1 ] </span>").arg(i+1);
        if((i+1)%10==0)track+="<br>";
    }
    sequence_->setText(track);
    QStringList tokens;
    if(current&&current->canFinish&&phase>=3){
        for(int j=0;j<resourceCount_->value();++j){
            int quantity=phase==3?result_.need[current->processId][j]:input_.max[current->processId][j];
            if(quantity)tokens<<QString("R%1 × %2").arg(j).arg(quantity);
        }
    }
    flow_->display(current?current->processId:-1,phase,current&&current->canFinish,tokens);
    for(int j=0;j<resourceCount_->value();++j){
        int total=input_.available[j];for(const auto& row:input_.allocation)total+=row[j];
        // Avoid numerical labels disagreeing with an interpolated gauge.
        animations_[j]->stop();gauges_[j]->setValue(pool[j]);
        auto* caption=gauges_[j]->parentWidget()->findChild<QLabel*>();
        caption->setText(QString("R%1\n可用 %2 / 总量 %3").arg(j).arg(pool[j]).arg(total));
        gauges_[j]->setAccessibleName(caption->text());
    }
    for(int i=0;i<processCount_->value();++i){
        const bool active=current&&current->processId==i;
        const bool borrowed=active&&phase==3&&current->canFinish;
        const QString color=done[i]?"#16806a":active?"#a35a24":"#687b88";
        QString state=done[i]?"已模拟完成":stuck?"无法继续":active?(phase==1?"选中":phase==2?"比较资源":borrowed?"已借入资源 · 执行中":"资源不足 · 跳过"):"等待检查";
        QString text=QString("<b style='color:%1;font-size:16px'>P%2 · %3</b>").arg(color).arg(i).arg(state);
        for(int j=0;j<resourceCount_->value();++j){
            const int need=(done[i]||borrowed)?0:result_.need[i][j];
            const int held=done[i]?0:borrowed?input_.max[i][j]:input_.allocation[i][j];
            text+=QString("<br>R%1　还需 <b style='font-size:21px'>%2</b> <span style='color:#71828d'>持有 %3 / 最大 %4</span>").arg(j).arg(need).arg(held).arg(input_.max[i][j]);
            if(!done[i]&&((active&&phase>=2)||stuck)){
                int available=stuck?pool[j]:current->workBefore[j];
                int gap=result_.need[i][j]-available;
                text+=QString("<br><span style='color:%1'>需要 %2 / 可用 %3 · %4</span>").arg(gap>0?"#b65132":"#16806a").arg(result_.need[i][j]).arg(available).arg(gap>0?QString("缺 %1").arg(gap):"满足");
            }
        }
        texts_[i]->setMinimumHeight(36+resourceCount_->value()*32+((!done[i]&&((active&&phase>=2)||stuck))?resourceCount_->value()*22:0));
        texts_[i]->setText(text);
        texts_[i]->parentWidget()->setMinimumHeight(texts_[i]->minimumHeight()+80);
        texts_[i]->parentWidget()->layout()->activate();
        texts_[i]->parentWidget()->setStyleSheet(QString("QFrame#card{border:2px solid %1;border-radius:10px;}").arg(active?color:"#dce5e8"));
    }
    if(!current){message_->setText("点击自动执行，或用下一步逐阶段观察；安全序列将在完成后逐格填入。");return;}
    QString explanation;
    if(phase==1)explanation="选中一个未完成进程，准备检查。";
    else if(phase==2)explanation="逐项比较 Need 与 Work；每一种资源都足够才能继续。";
    else if(!current->canFinish)explanation="资源不足，本次跳过；资源池保持不变。";
    else if(phase==3)explanation="借出 Need，资源池暂时减少；进程获得其最大需求量后模拟执行。";
    else explanation="进程完成，归还 Allocation + Need；资源池相对检查前净增加原 Allocation。";
    message_->setText(QString("检查 P%1 · 阶段 %2 / 4：%3").arg(current->processId).arg(phase).arg(explanation));
}
void BankerWidget::showExampleStep(int step){example_->setCurrentIndex(0);loadExample();replay_->seek(step*4);}
bool BankerWidget::smokeTest(){
    showExampleStep(0);if(!replay_->smokeTest())return false;
    if(status_->text().contains("SAFE")||sequence_->text().contains("P1"))return false;
    // P0 is skipped; P1 borrows [1,2,2] then returns [3,2,2].
    replay_->seek(6);if(gauges_[0]->value()!=3||!texts_[1]->text().contains("满足"))return false;
    replay_->seek(7);if(gauges_[0]->value()!=2||gauges_[1]->value()!=1||gauges_[2]->value()!=0||sequence_->text().contains("P1"))return false;
    replay_->seek(8);if(gauges_[0]->value()!=5||!sequence_->text().contains("P1")||!texts_[1]->text().contains("已模拟完成"))return false;
    replay_->seek(4);if(gauges_[0]->value()!=3||sequence_->text().contains("P1"))return false;
    if(!(gauges_[0]->height()>gauges_[2]->height()&&gauges_[2]->height()>gauges_[1]->height()))return false;
    replay_->seek(int(result_.steps.size())*4);if(!status_->text().startsWith("SAFE")||gauges_[0]->value()!=10)return false;
    example_->setCurrentIndex(1);loadExample();replay_->seek(int(result_.steps.size())*4);
    if(!status_->text().startsWith("UNSAFE")||!texts_[0]->text().contains("缺"))return false;
    example_->setCurrentIndex(2);loadExample();replay_->seek(3);if(gauges_[0]->value()!=3)return false;
    max_->item(0,0)->setText("-1");calculate();if(valid_)return false;
    showExampleStep(0);resourceCount_->setValue(1);processCount_->setValue(1);if(!valid_||texts_.size()!=1||gauges_.size()!=1)return false;
    available_->setText("");calculate();if(valid_)return false;
    showExampleStep(2);return true;
}
