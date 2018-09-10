#ifndef POLICYRULESEDITOR_H
#define POLICYRULESEDITOR_H

#include <QDialog>

#include "common/configdb.h"

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
        int addPattern(const QString &id, const QString &name,
                       const QString &days, const QString &interval, int Referenced = 0);
        int addPattern(const QString &name, bool days, int interval);
        QString getDayNames(const QString &Days);
        int parseInterval(QString& intervalStr);
        void setPattern(const int row, const QString &name,
                                           const QString &days, const QString &interval);

        bool addPolicyRules(QVector<ConfigDb::PolicyInfo> &infos);
        void showPolicyRules();
        QString convertInterval(int interval);

            private slots:
        void slotAddPattern();
        void slotUpdatePolicyRules();
        void slotStateChanged(int state);
        void slotRemoveCurrentItem();
        void slotItemSelectionChanged();
        void slotEditCurrentItem();


    private:
        Ui::PolicyRulesEditor *ui;
        // ConfigDb _configDb;
        ConfigDb *_pconfigDb;
    };

} // namespace OCC
#endif // POLICYRULESEDITOR_H
