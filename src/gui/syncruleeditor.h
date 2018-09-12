#ifndef SYNCRULEEDITOR_H
#define SYNCRULEEDITOR_H


#include "common/configdb.h"
#include "common/syncjournaldb.h"
#include "folderstatusmodel.h"

#include <QDialog>
namespace OCC {
namespace Ui {
class SyncRuleEditor;
}

class SyncRuleEditor : public QDialog
{
    Q_OBJECT

public:
    explicit SyncRuleEditor(SyncJournalDb *journalDb, FolderStatusModel::SubFolderInfo *subFolderInfo, QWidget *parent = nullptr);
    ~SyncRuleEditor();
    void setSyncRuleFocus();
    void showPolicyRules(int selectPolicyRulesId);
    void setSyncRule(FolderStatusModel::SubFolderInfo *subFolderInfo);

private:
    Ui::SyncRuleEditor *ui;
    ConfigDb *_pconfigDb;
    SyncJournalDb *_pjournalDb;

    bool _hasRuleInDb;
    int _inode;
    FolderStatusModel::SubFolderInfo *_subFolderInfo;

};
}
#endif // SYNCRULEEDITOR_H
