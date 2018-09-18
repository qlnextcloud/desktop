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

        _pconfigDb = ConfigDb::instance();

        if (!_pconfigDb->isConnected()) {
            return;
        }

        ui->setupUi(this);
        ui->descPolicyRulesLabel->setText(tr("These policy rules can be used in synchronization rules."));
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
        delete ui;
    }

    void PolicyRulesEditor::slotAddPattern()
    {
        QString id;
        QString tempDays = QString(DEF_DAYS);

        auto *simgleEditor = new PolirySimgleEditor(tr("Add Policy Rule"), this);
        QStringList currentNamesList = generateCurrentNameList();
        simgleEditor->setCurrentNameList(currentNamesList);
        simgleEditor->setDays(tempDays);
        //simgleEditor->setAttribute(Qt::WA_DeleteOnClose, true);
        if (simgleEditor->exec() != QDialog::Accepted) {
            return ;
        }

        QString name = simgleEditor->getName();
        QString days = simgleEditor->getDays();
        QString interval = simgleEditor->getInterval();
        delete(simgleEditor);

        // 添加到table中
        addPattern(id, name, days, interval);
    }

    int PolicyRulesEditor::addPattern(const QString &id, const QString &name,
                   const QString &days, const QString &interval, int Referenced)
    {
        int newRow = ui->policyRulesTableWidget->rowCount();
        ui->policyRulesTableWidget->setRowCount(newRow + 1);

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
            return;
        }

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
            return;
        }

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
        QTableWidgetItem *idItem = ui->policyRulesTableWidget->item(row, idCol);
        QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
        QTableWidgetItem *daysItem = ui->policyRulesTableWidget->item(row, daysCol);
        QTableWidgetItem *intervalItem = ui->policyRulesTableWidget->item(row, intervalCol);

        bool canEdit = true;
        QString tempIdStr = idItem->text();
        bool ok = false;
        int tempIdInt = tempIdStr.toInt(&ok);
        if (ok && tempIdInt == DEFAULT_POLICY_RULE_ID) {
            canEdit = false;
        }

        // 设置信息到编辑界面
        auto *simgleEditor = new PolirySimgleEditor(tr("Edit Policy Rule"), this);
        QString tempName = nameItem->text();
        QString tempDays = daysItem->text();
        QString tempInterval = intervalItem->text();
        QStringList excludeList;
        excludeList.append(tempName);
        QStringList currentNamesList = generateCurrentNameList(excludeList);
        simgleEditor->setCurrentNameList(currentNamesList);
        simgleEditor->setName(tempName, canEdit);
        simgleEditor->setDays(tempDays, canEdit);
        simgleEditor->setInterval(tempInterval, canEdit);

        if (simgleEditor->exec() != QDialog::Accepted) {
            delete(simgleEditor);
            return ;
        }

        if (!canEdit) {
            delete(simgleEditor);
            return ;
        }

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
                temp._id = -1;                      // -1 表示新的
            }

            temp._name = nameItem->text();
            temp._days = daysItem->text().toLatin1();
            QString intervalAndUnit = intervalItem->text();
            temp._interval = parseInterval(intervalAndUnit);

            temp._referenced = referencedItem->text().toInt(&ok);
            if (!ok) {
                qDebug() << "_referenced toInt failed: referencedItem->text() = " << referencedItem->text();
                continue;
            }

            if (idItem->text().isEmpty()) {
                newRules.append(temp);
            } else {
                oldRules.append(temp);
            }
        }

        // delete all info
        if (!_pconfigDb->delAllPolicyInfo()) {
            return;
        }

        // 先处理old（有ID的），new的ID自动分配
        addPolicyRules(oldRules);
        addPolicyRules(newRules);
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
            _week.append(tr("Sunday"));
            _week.append(tr("Monday"));
            _week.append(tr("Tuesday"));
            _week.append(tr("Wednesday"));
            _week.append(tr("Thursday"));
            _week.append(tr("Friday"));
            _week.append(tr("Saturday"));
        }
    }
    QStringList PolicyRulesEditor::generateCurrentNameList() {
        QStringList currentNameList;
        for (int row = 0; row < ui->policyRulesTableWidget->rowCount(); ++row) {
            QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
            currentNameList.append(nameItem->text());
        }
        return currentNameList;
    }

    QStringList PolicyRulesEditor::generateCurrentNameList(QStringList &excludeList) {
        QStringList currentNameList;
        for (int row = 0; row < ui->policyRulesTableWidget->rowCount(); ++row) {
            QTableWidgetItem *nameItem = ui->policyRulesTableWidget->item(row, nameCol);
            QString name = nameItem->text();
            if (excludeList.indexOf(name) == -1 ) {
                currentNameList.append(name);
            }
        }
        return currentNameList;
    }

} // namespace OCC
