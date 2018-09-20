#include "polirysimgleeditor.h"
#include "ui_polirysimgleeditor.h"

#include "csync.h"      // qDebug

namespace OCC {
    static QVector<int> intervalMinuteArray = {1, 2, 3, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
    static QVector<int> intervalHourArray = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 24};
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
        this->setWindowTitle(title);
        ui->groupBox->setTitle(title);
        for (int i = 0; i < intervalMinuteArray.count(); i++) {
            ui->intervalComboBox->addItem(formatIntervalMinute(intervalMinuteArray.at(i)));
        }

        for (int i = 0; i < intervalHourArray.count(); i++) {
            ui->intervalComboBox->addItem(formatIntervalHour(intervalHourArray.at(i)));
        }

        prepareDaysCheckBox();
        _qLableErrorStyleSheet = "QLabel { color : red; }";
        ui->errorLabel->setStyleSheet(_qLableErrorStyleSheet);
    }

    PolirySimgleEditor::~PolirySimgleEditor() {
        _checkBoxs.clear();
        delete ui;
    }

    void PolirySimgleEditor::prepareDaysCheckBox() {
        // 不要修改这里的顺序
        _checkBoxs.append(ui->sundayCheckBox);
        _checkBoxs.append(ui->mondayCheckBox);
        _checkBoxs.append(ui->tuesdayCheckBox);
        _checkBoxs.append(ui->wednesdayCheckBox);
        _checkBoxs.append(ui->thursdayCheckBox);
        _checkBoxs.append(ui->fridayCheckBox);
        _checkBoxs.append(ui->saturdayCheckBox);
    }

    QString PolirySimgleEditor::formatIntervalSecond(int interval)
    {
        return QString::number(interval) + " " + getSecondUnit();

    }
    QString PolirySimgleEditor::formatIntervalMinute(int interval)
    {
        return QString::number(interval) + " " + getMinuteUnit();
    }

    QString PolirySimgleEditor::formatIntervalHour(int interval)
    {
        return QString::number(interval) + " " + getHourUnit();
    }

    QString PolirySimgleEditor::getSecondUnit() {
        return tr("Seconds");
    }

    QString PolirySimgleEditor::getMinuteUnit() {
        return tr("Minute(s)");
    }

    QString PolirySimgleEditor::getHourUnit() {
        return tr("Hour(s)");
    }

    QString PolirySimgleEditor::getName() {
        return ui->nameLineEdit->text();
    }

    void PolirySimgleEditor::setName(QString &name, bool canEdit) {
        ui->nameLineEdit->setText(name);
        ui->nameLineEdit->setEnabled(canEdit);
    }

    QString PolirySimgleEditor::getInterval() {
        return ui->intervalComboBox->currentText();
    }

    void PolirySimgleEditor::setInterval(QString &Interval, bool canEdit) {
        int itemCount = ui->intervalComboBox->count();

        for (int i = 0; i < itemCount; i++) {
            if (ui->intervalComboBox->itemText(i) == Interval) {
                ui->intervalComboBox->setCurrentIndex(i);
                break;
            }
        }
        ui->intervalComboBox->setEnabled(canEdit);
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

    void PolirySimgleEditor::setDays(const QString &days, bool canEdit) {
        for (int i = 0; i < days.count() && i < _checkBoxs.count(); i++) {
            QCheckBox *checkBox = _checkBoxs.at(i);
            checkBox->setCheckState(days.at(i) == '1' ? Qt::Checked : Qt::Unchecked);
            checkBox->setEnabled(canEdit);
        }
    }

    bool PolirySimgleEditor::isDay(const QString &days, int i)
    {
        return days.at(i) == '1';
    }

    bool PolirySimgleEditor::isEmptyDays(const QString &days)
    {
        for (int i = 0; i < days.count(); i++) {
            if (days.at(i) == '1') {
                return false;
            }
        }
        return true;
    }

    bool PolirySimgleEditor::checkUserInput()
    {
        QString name = getName();
        QString days = getDays();

        if (name.isEmpty()) {
            _errorString = tr("Name is Empty.");
            return false;
        }

        if (isEmptyDays(days)) {
            _errorString = tr("Choose at least one day.");
            return false;
        }

        if (_currentNameList.indexOf(name) != -1)
        {
            _errorString = tr("This name already exists.");
            return false;
        }

        return true;
    }

    void PolirySimgleEditor::accept()
    {
        if (checkUserInput()) {
            done(QDialog::Accepted);
        } else {
            ui->errorLabel->setText(_errorString);
        }
    }

    void PolirySimgleEditor::setCurrentNameList(QStringList &currentNameList)
    {
        _currentNameList = currentNameList;
    }

}