#ifndef POLICYRULESEDITOR_H
#define POLICYRULESEDITOR_H

#include <QDialog>
namespace OCC {

namespace Ui {
class PolicyRulesEditor;
}

class PolicyRulesEditor : public QDialog
{
    Q_OBJECT

public:
    explicit PolicyRulesEditor(QWidget *parent = nullptr);
    ~PolicyRulesEditor();

private:
    Ui::PolicyRulesEditor *ui;
};

} // namespace OCC
#endif // POLICYRULESEDITOR_H
