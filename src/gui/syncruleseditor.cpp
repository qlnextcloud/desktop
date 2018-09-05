#include "syncruleseditor.h"
#include "ui_syncruleseditor.h"
namespace OCC {
SyncRulesEditor::SyncRulesEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SyncRulesEditor)
{
    ui->setupUi(this);
}

SyncRulesEditor::~SyncRulesEditor()
{
    delete ui;
}

} // namespace OCC
