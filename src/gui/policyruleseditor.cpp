#include "policyruleseditor.h"
#include "ui_policyruleseditor.h"

namespace OCC {

PolicyRulesEditor::PolicyRulesEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PolicyRulesEditor)
{
    ui->setupUi(this);
}

PolicyRulesEditor::~PolicyRulesEditor()
{
    delete ui;
}

} // namespace OCC
