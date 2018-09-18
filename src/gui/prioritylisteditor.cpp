#include "prioritylisteditor.h"
#include "ui_prioritylisteditor.h"
#include "configfile.h"
#include "priorityfiles.h"

#include <QFile>
#include <QMessageBox>


namespace OCC {

    static int patternCol = 0;
    static int enabledCol = 1;

    PriorityListEditor::PriorityListEditor(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::PriorityListEditor)
    {
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        ui->setupUi(this);

        // set description
        ui->descPriorityFilesLabel->setText(tr("Files or folders that match the pattern will be prioritized to other files or folders in the same directory."));

        ConfigFile cfgFile;
        readPriorityFile(cfgFile.priorityFile(ConfigFile::UserScope));

        connect(this, &QDialog::accepted, this, &PriorityListEditor::slotUpdateLocalPriorityList);
        connect(ui->addPriorityFileButton, &QAbstractButton::clicked, this, &PriorityListEditor::slotAddPattern);
        connect(ui->rmPriorityFilePushButton, &QAbstractButton::clicked, this, &PriorityListEditor::slotRemoveCurrentItem);
        connect(ui->priorityFilesTableWidget, &QTableWidget::itemSelectionChanged, this, &PriorityListEditor::slotItemSelectionChanged);

        ui->priorityFilesTableWidget->resizeColumnsToContents();
        ui->priorityFilesTableWidget->horizontalHeader()->setResizeMode(patternCol, QHeaderView::Stretch);
        ui->priorityFilesTableWidget->verticalHeader()->setVisible(false);
        ui->rmPriorityFilePushButton->setEnabled(false);
        ui->priorityFilesTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    }

    PriorityListEditor::~PriorityListEditor()
    {
        delete ui;
    }

    void PriorityListEditor::readPriorityFile(const QString &file)
    {
        QFile priority(file);
        if (priority.open(QIODevice::ReadOnly)) {
            while (!priority.atEnd()) {
                QString line = QString::fromUtf8(priority.readLine());
                line.chop(1);
                if (!line.isEmpty() && !line.startsWith("#")) {
                    bool enabled = false;
                    if (line.startsWith(']')) {
                        enabled = true;
                        line = line.mid(1);
                    }
                    addPattern(line, enabled);
                }
            }
        }
    }

    int PriorityListEditor::addPattern(const QString &pattern, bool enabled)
    {
        int newRow = ui->priorityFilesTableWidget->rowCount();
        ui->priorityFilesTableWidget->setRowCount(newRow + 1);

        QTableWidgetItem *patternItem = new QTableWidgetItem;
        patternItem->setText(pattern);
        ui->priorityFilesTableWidget->setItem(newRow, patternCol, patternItem);

        QTableWidgetItem *deletableItem = new QTableWidgetItem;
        deletableItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        deletableItem->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
        ui->priorityFilesTableWidget->setItem(newRow, enabledCol, deletableItem);
        return newRow;
    }


    void PriorityListEditor::slotAddPattern()
    {
        addPattern(QString(), true);
    }

    void PriorityListEditor::slotUpdateLocalPriorityList()
    {
        ConfigFile cfgFile;
        QString priorityFile = cfgFile.priorityFile(ConfigFile::UserScope);
        QFile priority(priorityFile);
        if (priority.open(QIODevice::WriteOnly)) {
            for (int row = 0; row < ui->priorityFilesTableWidget->rowCount(); ++row) {
                QTableWidgetItem *patternItem = ui->priorityFilesTableWidget->item(row, patternCol);
                QTableWidgetItem *enableItem = ui->priorityFilesTableWidget->item(row, enabledCol);

                if ((patternItem->flags() & Qt::ItemIsEnabled) && !patternItem->text().isEmpty()) {
                    QByteArray prepend;
                    if (enableItem->checkState() == Qt::Checked) {
                        prepend = "]";
                    } else if (patternItem->text().startsWith('#')) {
                        prepend = "\\";
                    }
                    priority.write(prepend + patternItem->text().toUtf8() + "\n");
                }
            }
        } else {
            QMessageBox::warning(this, tr("Could not open file"),
                                 tr("Cannot write changes to '%1'.").arg(priorityFile));
        }
        priority.close(); //close the file before reloading stuff.


        //FolderMan *folderMan = FolderMan::instance();

        /* handle the hidden file checkbox */

        /* the ignoreHiddenFiles flag is a folder specific setting, but for now, it is
         * handled globally. Save it to every folder that is defined.
         */
        /*
        folderMan->setIgnoreHiddenFiles(ignoreHiddenFiles());

        // We need to force a remote discovery after a change of the ignore list.
        // Otherwise we would not download the files/directories that are no longer
        // ignored (because the remote etag did not change)   (issue #3172)
                foreach (Folder *folder, folderMan->map()) {
                folder->journalDb()->forceRemoteDiscoveryNextSync();
                folderMan->scheduleFolder(folder);
            }

        ExcludedFiles::instance().reloadExcludes();
        */
        PriorityFiles::instance().reloadPrioritys();
    }

    void PriorityListEditor::slotRemoveCurrentItem()
    {
        ui->priorityFilesTableWidget->removeRow(ui->priorityFilesTableWidget->currentRow());
    }

    void PriorityListEditor::slotItemSelectionChanged()
    {
        if (ui->priorityFilesTableWidget->rowCount() <= 0) {
            ui->rmPriorityFilePushButton->setEnabled(false);
            return;
        }

        ui->rmPriorityFilePushButton->setEnabled(true);
    }

}
