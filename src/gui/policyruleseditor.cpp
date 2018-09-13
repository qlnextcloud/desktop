#include "policyruleseditor.h"
#include "ui_policyruleseditor.h"
#include "configfile.h"
#include "owncloudlib.h"
#include "common/configdb.h"
#include "polirysimgleeditor.h"

#include "csync.h"      // qDebug

#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>

#define REF_TRUE    1           // 大于1都是true
#define REF_FALSE   0           // 小于1都是false
#define DEF_DAYS    "0111110"
#define HOUR_TO_SECOND 3600
#define MINUTE_TO_SECOND 60

namespace OCC {
    static int idCol = 0;
    static int nameCol = 1;
    static int daysCol = 2;
    static int daysNameCol = 3;
    static int intervalCol = 4;
    static int referencedCol = 5;
    PolicyRulesEditor::PolicyRulesEditor(QWidget *parent) :
            QDialog(parent),
            //_configDb(ConfigFile::globalConfigDbFile()),
            ui(new Ui::PolicyRulesEditor)
    {
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        prepareWeek();

        ConfigFile cfg;
        _pconfigDb = cfg.getGlobalConfigDb();

        if (!_pconfigDb->isConnected()) {
            qDebug() << "----isshe----: ----数据库连接失败----";
            return;
        }

        qDebug() << "----isshe----: ----数据库连接成功----";

        ui->setupUi(this);
        ui->descPolicyRulesLabel->setText(tr("-----isshe----- test description..."));
        showPolicyRules();

        ui->policyRulesTableWidget->resizeColumnsToContents();
        ui->policyRulesTableWidget->horizontalHeader()->setResizeMode(nameCol, QHeaderView::Stretch);
        ui->policyRulesTableWidget->horizontalHeader()->setResizeMode(daysNameCol, QHeaderView::Stretch);
        ui->policyRulesTableWidget->horizontalHeader()->setResizeMode(intervalCol, QHeaderView::Stretch);
        ui->policyRulesTableWidget->verticalHeader()->setVisible(false);

        ui->policyRulesTableWidget->setColumnHidden(idCol, true);
        ui->policyRulesTableWidget->setColumnHidden(daysCol, true);
        ui->policyRulesTableWidget->setColumnHidden(referencedCol, true);

        ui->policyRulesTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->policyRulesTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->policyRulesTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

        connect(ui->policyRulesTableWidget, &QTableWidget::itemSelectionChanged, this, &PolicyRulesEditor::slotItemSelectionChanged);
        connect(ui->addPolicyRulePushButton, &QAbstractButton::clicked, this, &PolicyRulesEditor::slotAddPattern);
        connect(this, &QDialog::accepted, this, &PolicyRulesEditor::slotUpdatePolicyRules);
        connect(ui->rmPolicyRulePushButton, &QAbstractButton::clicked, this, &PolicyRulesEditor::slotRemoveCurrentItem);
        connect(ui->editPolicyRulesPushButton, &QAbstractButton::clicked, this, &PolicyRulesEditor::slotEditCurrentItem);

        ui->rmPolicyRulePushButton->setEnabled(false);
        ui->editPolicyRulesPushButton->setEnabled(false);
    }

    PolicyRulesEditor::~PolicyRulesEditor()
    {
        if (_pconfigDb) {
            delete(_pconfigDb);
        }

        delete ui;
    }

    void PolicyRulesEditor::slotAddPattern()
    {
        QString id;
        QString tempDays = QString(DEF_DAYS);

        auto *simgleEditor = new PolirySimgleEditor(tr("Add Policy Rule"), this);
        simgleEditor->setDays(tempDays);
        //simgleEditor->setAttribute(Qt::WA_DeleteOnClose, true);
        if (simgleEditor->exec() != QDialog::Accepted) {
            qDebug() << "-----isshe------: simgleEditor->exec() != QDialog::Accepted";
            return ;
        }
        qDebug() << "-----isshe------: QDialog::Accepted: " << simgleEditor->getName();

        QString name = simgleEditor->getName();
        QString days = simgleEditor->getDays();
        QString interval = simgleEditor->getInterval();
        delete(simgleEditor);

        // TODO: 进行信息校验

        // 添加到table中
        addPattern(id, name, days, interval);
        //addPattern(QString("isshe"), false, 10);
    }

    int PolicyRulesEditor::addPattern(const QString &id, const QString &name,
                   const QString &days, const QString &interval, int Referenced)
    {
        int newRow = ui->policyRulesTableWidget->rowCount();
        ui->policyRulesTableWidget->setRowCount(newRow + 1);

        qDebug() << "---isshe---: id = " << id << ", name = " << name << ", days = " << days;
        // 新一行的 ID 是空
        auto *idItem = new QTableWidgetItem();
        idItem->setText(id);
        ui->policyRulesTableWidget->setItem(newRow, idCol, idItem);

        auto *nameItem = new QTableWidgetItem();
        nameItem->setText(name);
        ui->policyRulesTableWidget->setItem(newRow, nameCol, nameItem);

        auto *daysItem = new QTableWidgetItem();
        daysItem->setText(days);
        ui->policyRulesTableWidget->setItem(newRow, daysCol, daysItem);

        auto *daysNameItem = new QTableWidgetItem();
        QString dayNames = getDayNames(days);
        daysNameItem->setText(dayNames);
        ui->policyRulesTableWidget->setItem(newRow, daysNameCol, daysNameItem);

        auto *intervalItem = new QTableWidgetItem();
        intervalItem->setText(interval); //QString::number(interval));
        ui->policyRulesTableWidget->setItem(newRow, intervalCol, intervalItem);

        auto *ReferencedItem = new QTableWidgetItem();
        ReferencedItem->setText(QString::number(Referenced));
        ui->policyRulesTableWidget->setItem(newRow, referencedCol, ReferencedItem);

        return newRow;
    }

    void PolicyRulesEditor::setPattern(const int row, const QString &name,
                                      const QString &days, const QString &interval)
    {
        //QTableWidgetItem *idItem = ui->policyRulesTableWidget->item(row, idCol);

        QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
        nameItem->setText(name);
        ui->policyRulesTableWidget->setItem(row, nameCol, nameItem);

        QTableWidgetItem *daysItem = ui->policyRulesTableWidget->item(row, daysCol);
        daysItem->setText(days);
        ui->policyRulesTableWidget->setItem(row, daysCol, daysItem);

        QTableWidgetItem *daysNameItem = ui->policyRulesTableWidget->item(row, daysNameCol);
        QString dayNames = getDayNames(days);
        daysNameItem->setText(dayNames);
        ui->policyRulesTableWidget->setItem(row, daysNameCol, daysNameItem);

        QTableWidgetItem *intervalItem = ui->policyRulesTableWidget->item(row, intervalCol);
        intervalItem->setText(interval); //QString::number(interval));
        ui->policyRulesTableWidget->setItem(row, intervalCol, intervalItem);
        //QTableWidgetItem *referencedItem = ui->policyRulesTableWidget->item(row, referencedCol);
    }

    void PolicyRulesEditor::slotItemSelectionChanged()
    {
        if (ui->policyRulesTableWidget->rowCount() <= 0) {
            ui->rmPolicyRulePushButton->setEnabled(false);
            ui->editPolicyRulesPushButton->setEnabled(false);
            return;
        }

        ui->editPolicyRulesPushButton->setEnabled(true);

        int row = ui->policyRulesTableWidget->currentRow();
        QTableWidgetItem *referencedItem = ui->policyRulesTableWidget->item(row, referencedCol);
        bool ok = false;
        int referenced = referencedItem->text().toInt(&ok);
        if (!ok) {
            qDebug() << "---isshe---: referenced 获取失败，不给删!!!";
            return;
        }
        qDebug() << "---isshe---: slotRemoveCurrentItem referenced = " << referenced;

        ui->rmPolicyRulePushButton->setEnabled((referenced == 0));
        if (referenced) {
            ui->rmPolicyRulePushButton->setToolTip(tr("Be Referenced"));
        }
    }

    void PolicyRulesEditor::slotRemoveCurrentItem()
    {
        int row = ui->policyRulesTableWidget->currentRow();
        QTableWidgetItem *referencedItem = ui->policyRulesTableWidget->item(row, referencedCol);
        bool ok = false;
        int referenced = referencedItem->text().toInt(&ok);
        if (!ok) {
            qDebug() << "---isshe---: referenced 获取失败，不给删!!!";
            return;
        }
        qDebug() << "---isshe---: slotRemoveCurrentItem referenced = " << referenced;

        if (referenced >= REF_TRUE) {
            QMessageBox::warning(this, tr("Error"), tr("Cannot delete this rule."));
        } else {
            ui->policyRulesTableWidget->removeRow(row);
        }
    }

    void PolicyRulesEditor::slotEditCurrentItem()
    {
        // 获取当前行的信息
        int row = ui->policyRulesTableWidget->currentRow();
        QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
        QTableWidgetItem *daysItem = ui->policyRulesTableWidget->item(row, daysCol);
        QTableWidgetItem *intervalItem = ui->policyRulesTableWidget->item(row, intervalCol);

        // 设置信息到编辑界面
        auto *simgleEditor = new PolirySimgleEditor(tr("Edit Policy Rule"), this);
        QString tempName = nameItem->text();
        QString tempDays = daysItem->text();
        QString tempInterval = intervalItem->text();

        simgleEditor->setName(tempName);
        simgleEditor->setDays(tempDays);
        simgleEditor->setInterval(tempInterval);

        if (simgleEditor->exec() != QDialog::Accepted) {
            qDebug() << "-----isshe------: simgleEditor->exec() != QDialog::Accepted";
            return ;
        }

        // TODO:校验信息
        QString name = simgleEditor->getName();
        QString days = simgleEditor->getDays();
        QString interval = simgleEditor->getInterval();

        // 更新信息到当前行
        setPattern(row, name, days, interval);

        delete(simgleEditor);
    }

    int PolicyRulesEditor::parseInterval(QString& intervalAndUnit)
    {
        // default 10 minute
        int interval = 10;
        bool ok = false;

        QString intervalStr = intervalAndUnit.section(" ", 0, 0);
        int temp = intervalStr.toInt(&ok);
        if (ok) {
            interval = temp;
        }

        // 这里单位和
        qDebug() << "----isshe----: PolirySimgleEditor::getHourUnit() = " << PolirySimgleEditor::getHourUnit();
        if (intervalAndUnit.contains(PolirySimgleEditor::getMinuteUnit())) {
            interval *= 60;
        }

        if (intervalAndUnit.contains(PolirySimgleEditor::getHourUnit())) {
            interval *= 3600;
        }

        return interval;
    }

    void PolicyRulesEditor::slotUpdatePolicyRules()
    {
        // 这里要求在 add/edit的时候，就保证数据不同（名称等）
        // 整理规则，分为旧规则和新增规则
        // 删除数据库次表的所有内容
        // (含 ID )插入旧规则
        // 插入新规则（ID 自动分配）

        QVector<ConfigDb::PolicyInfo> oldRules;
        QVector<ConfigDb::PolicyInfo> newRules;
        for (int row = 0; row < ui->policyRulesTableWidget->rowCount(); ++row) {
            QTableWidgetItem *idItem = ui->policyRulesTableWidget->item(row, idCol);
            QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
            QTableWidgetItem *daysItem = ui->policyRulesTableWidget->item(row, daysCol);
            //QTableWidgetItem *daysNameItem = ui->policyRulesTableWidget->item(row, daysNameCol);
            QTableWidgetItem *intervalItem = ui->policyRulesTableWidget->item(row, intervalCol);
            QTableWidgetItem *referencedItem = ui->policyRulesTableWidget->item(row, referencedCol);

            ConfigDb::PolicyInfo temp;
            bool ok = false;
            temp._id = idItem->text().toInt(&ok);
            if (!ok) {
                qDebug() << "----isshe----: id toInt failed!!!: id = " << temp._id;
                temp._id = -1;                      // -1 表示新的
            }

            temp._name = nameItem->text();
            temp._days = daysItem->text().toLatin1();
            QString intervalAndUnit = intervalItem->text();
            qDebug() << "----isshe----: intervalAndUnit = " << intervalAndUnit;
            temp._interval = parseInterval(intervalAndUnit);
            qDebug() << "----isshe----: temp->_interval = " << temp._interval;

            temp._referenced = referencedItem->text().toInt(&ok);
            if (!ok) {
                qDebug() << "----isshe----: _referenced toInt failed!!!: id = " << temp._referenced;
                //temp._referenced = 0;
                continue;       // 跳过这条
            }

            if (idItem->text().isEmpty()) {
                newRules.append(temp);
            } else {
                oldRules.append(temp);
            }
        }

        // delete all info
        if (!_pconfigDb->delAllPolicyInfo()) {
            qDebug() << "----isshe----: delAllPolicyInfo failed!!!!!!-----";
            return;
        }

        // 先处理old（有ID的），new的ID自动分配
        if (!addPolicyRules(oldRules)) {
            qDebug() << "----isshe----: addPolicyRules oldRules failed!!!!-----";
        }
        if (!addPolicyRules(newRules)) {
            qDebug() << "----isshe----: addPolicyRules newRules failed!!!!-----";
        }
    }

    bool PolicyRulesEditor::addPolicyRules(QVector<ConfigDb::PolicyInfo> &infos)
    {
        bool res = true;

        if (!infos.isEmpty()) {
            QVector<ConfigDb::PolicyInfo>::iterator iter;
            for (iter=infos.begin(); iter!=infos.end(); iter++)
            {
                if (!_pconfigDb->addPolicyInfo(*iter)) {
                    res = false;
                    qDebug() << "----isshe---: add failed: id = " << iter->_id << ", name = " << iter->_name << ", days = " << iter->_days;
                    continue;        // next one
                }
            }
        }
        return res;
    }


    void PolicyRulesEditor::showPolicyRules() {
        QVector<ConfigDb::PolicyInfo> infos = _pconfigDb->getPolicyInfo();

        if (!infos.isEmpty()) {
            QVector<ConfigDb::PolicyInfo>::iterator iter;
            for (iter = infos.begin(); iter != infos.end(); iter++) {

                // _referenced应该为非负数
                if (iter->_referenced < REF_FALSE) {
                    qDebug() << "-----isshe----: iter->_referenced 有错误！！！, = " << iter->_referenced;
                }
                addPattern(QString::number(iter->_id), iter->_name, iter->_days,
                           convertInterval(iter->_interval), iter->_referenced);
            }
        }
    }

    QString PolicyRulesEditor::convertInterval(int interval) {
        QString intervalStr;
        if (interval < MINUTE_TO_SECOND) {
            intervalStr.append(PolirySimgleEditor::formatIntervalSecond(interval));
        } else if (interval < HOUR_TO_SECOND) {
            intervalStr.append(PolirySimgleEditor::formatIntervalMinute(interval / MINUTE_TO_SECOND));
        } else {
            intervalStr.append(PolirySimgleEditor::formatIntervalHour(interval / HOUR_TO_SECOND));
        }

        return intervalStr;
    }

    QString PolicyRulesEditor::getDayNames(const QString &Days)
    {
        QString res;
        for (int i = 0; i < Days.count(); i++) {
            if (Days.at(i) == '1') {
                res.append(tr("%1").arg(_week.at(i)) + ";");
            }
        }

        return res;
    }

    void PolicyRulesEditor::prepareWeek()
    {
        if (_week.isEmpty())
        {
            _week.append("Sunday");
            _week.append("Monday");
            _week.append("Tuesday");
            _week.append("Wednesday");
            _week.append("Thursday");
            _week.append("Friday");
            _week.append("Saturday");
        }
    }

    //---------------------------------------------------------------------------------------
    // 废弃！！！
    // 以下是直接编辑行的实现（未完成）
    int PolicyRulesEditor::addPattern(const QString &name, bool days, int interval)
    {
        int newRow = ui->policyRulesTableWidget->rowCount();
        ui->policyRulesTableWidget->setRowCount(newRow + 1);


        auto *idItem = new QTableWidgetItem();
        idItem->setText("");
        ui->policyRulesTableWidget->setItem(newRow, idCol, idItem);
        //ui->policyRulesTableWidget->setColumnHidden(idCol, true);


        auto *nameItem = new QTableWidgetItem();
        nameItem->setText(name);
        ui->policyRulesTableWidget->setItem(newRow, nameCol, nameItem);

        /*
        auto *daysItem = new QTableWidgetItem;
        daysItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        daysItem->setCheckState(days ? Qt::Checked : Qt::Unchecked);
        ui->policyRulesTableWidget->setItem(newRow, daysCol, daysItem);
        */
        /*
        QComboBox *comBox = new QComboBox();
        comBox->addItem("Y");
        comBox->addItem("X");
        ui->policyRulesTableWidget->setCellWidget(newRow, daysCol, comBox);
        */

        /*
        auto *pListWidget = new QListWidget(this);
        //QLineEdit *pLineEdit = new QLineEdit(this);
        for (int i = 0; i < 5; ++i)
        {
            auto *pItem = new QListWidgetItem(pListWidget);
            pListWidget->addItem(pItem);
            pItem->setData(Qt::UserRole, i);
            auto *pCheckBox = new QCheckBox(this);
            pCheckBox->setText(QStringLiteral("Qter%1").arg(i));
            pListWidget->addItem(pItem);
            pListWidget->setItemWidget(pItem, pCheckBox);
            connect(pCheckBox, SIGNAL(stateChanged(int)), this, SLOT(slotStateChanged(int)));
        }

        auto *comBox = new QComboBox();
        comBox->setModel(pListWidget->model());
        comBox->setView(pListWidget);
        QLineEdit *lineEdit = new QLineEdit();
        comBox->setLineEdit(lineEdit);
        ui->policyRulesTableWidget->setCellWidget(newRow, daysCol, comBox);

        lineEdit->setReadOnly(true);                                                                                                                    //ui.comboBox->setEditable(true);
        connect(lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));
        */

        auto *daysItem = new QTableWidgetItem();
        daysItem->setText("days");
        ui->policyRulesTableWidget->setItem(newRow, daysCol, daysItem);

        auto *intervalItem = new QTableWidgetItem();
        intervalItem->setText("interval");
        ui->policyRulesTableWidget->setItem(newRow, intervalCol, intervalItem);
        return newRow;
    }

} // namespace OCC
