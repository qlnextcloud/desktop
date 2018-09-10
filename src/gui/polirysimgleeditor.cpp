#include "polirysimgleeditor.h"
#include "ui_polirysimgleeditor.h"

#include "csync.h"      // qDebug

namespace OCC {
    static QVector<int> intervalMinuteArray = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
    static QVector<int> intervalHourArray = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24};
    static QString minuteUnit = "Minutes";
    static QString hourUnit = "Hour(s)";
    PolirySimgleEditor::PolirySimgleEditor(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::PolirySimgleEditor) {
        ui->setupUi(this);
        prepareDaysCheckBox();
    }

    PolirySimgleEditor::PolirySimgleEditor(const QString &title, QWidget *parent) :
            QDialog(parent),
            ui(new Ui::PolirySimgleEditor) {
        ui->setupUi(this);
        ui->groupBox->setTitle(title);
        for (int i = 0; i < intervalMinuteArray.count(); i++) {
            ui->intervalComboBox->addItem(formatIntervalMinute(intervalMinuteArray.at(i)));
        }

        for (int i = 0; i < intervalHourArray.count(); i++) {
            ui->intervalComboBox->addItem(formatIntervalHour(intervalHourArray.at(i)));
        }

        prepareDaysCheckBox();
    }

    PolirySimgleEditor::~PolirySimgleEditor() {
        _checkBoxs.clear();
        delete ui;
    }

    void PolirySimgleEditor::prepareDaysCheckBox() {
        // 不要修改这里的顺序
        _checkBoxs.append(ui->mondayCheckBox);
        _checkBoxs.append(ui->tuesdayCheckBox);
        _checkBoxs.append(ui->wednesdayCheckBox);
        _checkBoxs.append(ui->thursdayCheckBox);
        _checkBoxs.append(ui->fridayCheckBox);
        _checkBoxs.append(ui->saturdayCheckBox);
        _checkBoxs.append(ui->sundayCheckBox);
    }

    QString PolirySimgleEditor::formatIntervalMinute(int interval) {
        return QString::number(interval) + " " + getMinuteUnit();

    }

    QString PolirySimgleEditor::formatIntervalHour(int interval) {
        return QString::number(interval) + " " + getHourUnit();
    }

    QString PolirySimgleEditor::getMinuteUnit() {
        return tr("%1").arg(minuteUnit);
    }

    QString PolirySimgleEditor::getHourUnit() {
        return tr("%1").arg(hourUnit);
    }

    QString PolirySimgleEditor::getName() {
        return ui->nameLineEdit->text();
    }

    void PolirySimgleEditor::setName(QString &name) {
        ui->nameLineEdit->setText(name);
    }

    QString PolirySimgleEditor::getInterval() {
        return ui->intervalComboBox->currentText();
    }

    void PolirySimgleEditor::setInterval(QString &Interval) {
        int itemCount = ui->intervalComboBox->count();

        for (int i = 0; i < itemCount; i++) {
            if (ui->intervalComboBox->itemText(i) == Interval) {
                ui->intervalComboBox->setCurrentIndex(i);
                break;
            }
        }
    }

    QString PolirySimgleEditor::getDayState(Qt::CheckState state) {
        if (state == Qt::Checked) {
            return "1";
        }
        return "0";
    }

    QString PolirySimgleEditor::getDays() {
        QString daysStr;

        for (int i = 0; i < _checkBoxs.count(); i++) {
            QCheckBox *checkBox = _checkBoxs.at(i);
            daysStr.append(getDayState(checkBox->checkState()));
        }
        return daysStr;
    }

    void PolirySimgleEditor::setDays(const QString &days) {
        for (int i = 0; i < days.count() && i < _checkBoxs.count(); i++) {
            QCheckBox *checkBox = _checkBoxs.at(i);
            checkBox->setCheckState(days.at(i) == '1' ? Qt::Checked : Qt::Unchecked);
        }
    }
}