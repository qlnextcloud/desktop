#include "syncruleeditor.h"
#include "ui_syncruleeditor.h"
#include "configfile.h"
#include "csync.h"  //qDebug

#include <QPushButton>
#include <QMessageBox>

#define NONE_RULE_ID (-1)
namespace OCC {

    SyncRuleEditor::SyncRuleEditor(SyncJournalDb *journalDb, FolderStatusModel::SubFolderInfo *subFolderInfo, QWidget *parent) :
            QDialog(parent),
            ui(new Ui::SyncRuleEditor),
            _pjournalDb(journalDb)

    {
        ui->setupUi(this);
        ui->groupBox->setTitle(tr("Set Sync Rule"));
        ui->selectRuleLabel->setText(tr("Select Rule:"));

        ui->selectRuleComboBox->setMinimumWidth(96);

        // 读全局数据库
        _pconfigDb = ConfigDb::instance();

        if (!_pconfigDb->isConnected()) {
            QMessageBox::warning(this, tr("Error"), tr("Connect Db failed."));
            return;
        }

        // 如果已经设置了，要恢复配置
        int selectedPolicyRuleId = -1;
        QVector<SyncJournalDb::SyncRuleInfo> infos = _pjournalDb->getSyncRulesInfo();
        QVector<SyncJournalDb::SyncRuleInfo>::iterator iter;
        QString subFolderPath = subFolderInfo->_path;
        subFolderPath.chop(1);      // 去掉 /
        for (iter = infos.begin(); iter != infos.end(); iter++) {
            if (subFolderPath == iter->_path) {
                selectedPolicyRuleId = iter->_policyruleid;
                break;
            }
        }

        // 显示数据
        showPolicyRules(selectedPolicyRuleId);
    }

    void SyncRuleEditor::showPolicyRules(int selectPolicyRulesId)
    {
        QVector<ConfigDb::PolicyInfo> infos = _pconfigDb->getPolicyInfo();

        if (!infos.isEmpty()) {
            QVector<ConfigDb::PolicyInfo>::iterator iter;
            for (iter = infos.begin(); iter != infos.end(); iter++) {
                ui->selectRuleComboBox->addItem(iter->_name, iter->_id);
                if (selectPolicyRulesId == iter->_id) {
                    int itemCount = ui->selectRuleComboBox->count();
                    // set current index point to this item
                    ui->selectRuleComboBox->setCurrentIndex(itemCount - 1);
                }
            }
        }
    }

    SyncRuleEditor::~SyncRuleEditor()
    {
        delete ui;
    }

    void SyncRuleEditor::setSyncRule(FolderStatusModel::SubFolderInfo *subFolderInfo)
    {
        // 检查subFolder的信息
        QString sFolderPath = subFolderInfo->_path;         // test/
        QString sFolderName = subFolderInfo->_name;         // test

        sFolderPath.chop(1);
        if (sFolderPath != sFolderName) {
            QMessageBox::warning(this, tr("Error"), tr("Not the first level directory"));
            return;
        }

        // 获取UI信息
        QString policyRuleName = ui->selectRuleComboBox->currentText();
        bool ok =  false;
        int policyRuleId = (ui->selectRuleComboBox->currentData().toInt(&ok));
        if (!ok){
            QMessageBox::warning(this, tr("Error"), tr("Get policy rule failed."));
            return;
        }

        // 看此目录是否已经存在旧的同步规则(sync rule)
        // 出错：-1; 存在：1; 不存在: 0
        SyncJournalDb::SyncRuleInfo oldSyncInfo;
        int isExist = _pjournalDb->getSyncRuleByPath(sFolderPath, &oldSyncInfo);
        if (isExist < 0) {
            QMessageBox::warning(this, tr("Error"), tr("Get Sync rule failed."));
            return;
        }

        // 如果规则设置为None，就看之前有没有，有就删除
        if (policyRuleId == NONE_RULE_ID) {
            if (isExist == 1) {
                _pjournalDb->delSyncRuleByPath(sFolderPath);
                _pconfigDb->updatePolicryRuleReferenced(oldSyncInfo._policyruleid, false);   // -1
            }
            return;
        }

        // 以下就是新增或者是修改了
        // 查全局数据库获取策略规则信息（判断策略规则是否存在）
        if(!_pconfigDb->isExistPolicyRule(policyRuleId)) //, policyRuleName))
        {
            QMessageBox::warning(this, tr("Error"), tr("Policy rule may have been deleted."));
            return;
        }

        // 写到同步规则数据库
        SyncJournalDb::SyncRuleInfo newSyncInfo;
        newSyncInfo._path = sFolderPath;
        newSyncInfo._inode = 0;                    // 要不要inode：inode主要是为了应对改名的问题
        newSyncInfo._policyruleid = policyRuleId;
        newSyncInfo._pasttime = 0;
        newSyncInfo._needschedule = 0;
        newSyncInfo._needsync = 0;
        newSyncInfo._enabled = 1;
        if (!_pjournalDb->setSyncRulesInfo(newSyncInfo)) {
            QMessageBox::warning(this, tr("Error"), tr("Set Sync Rule Failed."));
            return;
        }

        if (isExist == 1 && oldSyncInfo._policyruleid != policyRuleId) {
            _pconfigDb->updatePolicryRuleReferenced(oldSyncInfo._policyruleid, false);       // -1
            _pconfigDb->updatePolicryRuleReferenced(policyRuleId, true);        // +1
        } else if (isExist == 0) {
            _pconfigDb->updatePolicryRuleReferenced(policyRuleId, true);        // +1
        }
    }

}
