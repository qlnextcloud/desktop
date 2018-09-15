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
        void setName(QString &name, bool canEdit = true);
        QString getInterval();
        void setInterval(QString &Interval, bool canEdit = true);
        QString getDays();
        void setDays(const QString &days, bool canEdit = true);
        QString getDayState(Qt::CheckState state);
        static QString getSecondUnit();
        static QString getMinuteUnit();
        static QString getHourUnit();
        static QString formatIntervalSecond(int interval);
        static QString formatIntervalMinute(int interval);
        static QString formatIntervalHour(int interval);
        static bool isDay(const QString &days, int i);
        void accept();
        bool checkUserInput();
        bool isEmptyDays(const QString &days);
        void setCurrentNameList(QStringList &currentNameList);

    private:
        Ui::PolirySimgleEditor *ui;
        QVector<QCheckBox *> _checkBoxs;
        QString _errorString;
        QString _qLableErrorStyleSheet;
        QStringList _currentNameList;
    };

}
#endif // POLIRYSIMGLEEDITOR_H
