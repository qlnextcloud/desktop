#ifndef POLIRYSIMGLEEDITOR_H
#define POLIRYSIMGLEEDITOR_H

#include <QDialog>
#include <QCheckBox>

namespace OCC {
    namespace Ui {
        class PolirySimgleEditor;
    }

    class PolirySimgleEditor : public QDialog
    {
    Q_OBJECT

    public:
        explicit PolirySimgleEditor(QWidget *parent = nullptr);
        explicit PolirySimgleEditor(const QString &title, QWidget *parent = nullptr);
        ~PolirySimgleEditor();

        void prepareDaysCheckBox();
        QString getName();
        void setName(QString &name);
        QString getInterval();
        void setInterval(QString &Interval);
        QString getDays();
        void setDays(const QString &days);
        QString getDayState(Qt::CheckState state);
        static QString getMinuteUnit();
        static QString getHourUnit();
        static QString formatIntervalMinute(int interval);
        static QString formatIntervalHour(int interval);

    private:
        Ui::PolirySimgleEditor *ui;
        QVector<QCheckBox *> _checkBoxs;
    };

}
#endif // POLIRYSIMGLEEDITOR_H
