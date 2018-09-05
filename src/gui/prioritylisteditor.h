#ifndef PRIORITYLISTEDITOR_H
#define PRIORITYLISTEDITOR_H

#include <QDialog>

namespace OCC {

namespace Ui {
    class PriorityListEditor;
}

class PriorityListEditor : public QDialog
{
    Q_OBJECT

public:
    explicit PriorityListEditor(QWidget *parent = nullptr);
    ~PriorityListEditor();

    int addPattern(const QString &pattern, bool enabled);
    void readPriorityFile(const QString &file);

private slots:
    void slotAddPattern();
    void slotUpdateLocalIgnoreList();
    void slotRemoveCurrentItem();



private:
    Ui::PriorityListEditor *ui;
};

}// namespace OCC
#endif // PRIORITYLISTEDITOR_H
