#ifndef SYNCRULESEDITOR_H
#define SYNCRULESEDITOR_H

#include <QDialog>

namespace OCC {

namespace Ui {
    class SyncRulesEditor;
}

class SyncRulesEditor : public QDialog
{
    Q_OBJECT

public:
    explicit SyncRulesEditor(QWidget *parent = nullptr);
    ~SyncRulesEditor();

private:
    Ui::SyncRulesEditor *ui;
};

} // namespace OCC

#endif // SYNCRULESEDITOR_H
