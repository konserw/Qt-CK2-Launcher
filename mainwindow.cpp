#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "firstdialog.h"

#include "QDir"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    settings(new QSettings),
    ui(new Ui::MainWindow)
{
    // Check if this first run
    if (! settings->value("internal/isConfigured").toBool()) {
        // Open modal dialog
        FirstDialog dialog(this);
        dialog.exec();

        // Quit if user press Cancel or close dialog window
        if (!dialog.dialogCompleted) {
            parent->close();
        }
    }

    ui->setupUi(this);
    // Load Styles
    loadStyles();
    // Load launcher page
    ui->webView->load(QUrl(settings->value("internal/urlLauncher").toString()));
    // Disable context menu
    ui->webView->setContextMenuPolicy(Qt::CustomContextMenu);

    foreach(QString key, settings->allKeys())
    {
        if (key.right(4) == ".mod")
        {
            QDir dir(settings->value("userOptions/pathConfig").toString() + "mod/");
            if(!dir.exists(key))
                settings->remove(key);
        }
    }

    foreach(QString modName, listFiles(settings->value("userOptions/pathConfig").toString() + "mod/", "*.mod"))
    {
        QString modIniName = extractName(settings->value("userOptions/pathConfig").toString() + "/mod/", modName);

        if (!settings->contains(modName))
            settings->setValue(modName, Qt::Unchecked);

        QListWidgetItem *itemOne = new QListWidgetItem(
            modIniName,
            ui->listMod);
        if (settings->value(modName) == Qt::Checked)
            itemOne->setCheckState(Qt::Checked);
        else
            itemOne->setCheckState(Qt::Unchecked);
        itemOne->setData(Qt::UserRole, modName);
    }

    foreach(QString dlcName, listFiles(settings->value("userOptions/pathGame").toString() + "/dlc/", "*.dlc"))
    {
        QString dlcIniName = extractName(settings->value("userOptions/pathGame").toString() + "/dlc/", dlcName);
        if (!settings->contains(dlcName))
        {
            settings->setValue(dlcName, Qt::Checked);
        }

        QListWidgetItem *itemOne = new QListWidgetItem(
            dlcIniName,
            ui->listDLC);
        if (settings->value(dlcName) == Qt::Checked)
            itemOne->setCheckState(Qt::Checked);
        else
            itemOne->setCheckState(Qt::Unchecked);
        itemOne->setData(Qt::UserRole, dlcName);
    }
}

QStringList MainWindow::listFiles(QString directory, QString extension)
{
    QDir modDir(directory);
    QStringList filters;
    filters << extension;
    return modDir.entryList(filters, QDir::Files);
}

QString MainWindow::extractName(QString iniDir, QString iniName)
{
    QSettings ini(iniDir.append(iniName), QSettings::IniFormat);

    if (ini.value("name").toString() == "")
        return iniName;
    else
        return ini.value("name").toString();
}

/**
  Launcher use original game (proprietary) content which can't going bundled with this code.
  For now don't find any better way to change path to background images and icon.
  */
void MainWindow::loadStyles()
{
    QString launcherPath = settings->value("userOptions/pathGame").toString()+"launcher/";
    QIcon icon;
    icon.addFile(launcherPath+"logo.ico", QSize(), QIcon::Normal, QIcon::Off);
    setWindowIcon(icon);

    ui->widget->setStyleSheet(
        "#widget { background-image: url("+launcherPath+"background.jpg); }"
        "#buttonRegister { background-image: url("+launcherPath+"button_small.jpg);border-width: 1px; }"
        "#buttonRun { background-image: url("+launcherPath+"button_big.jpg);border-width: 1px; }"
    );
}



/*  *   *   *   *   *   *   *   *   *
            Slots
 *  *   *   *   *   *   *   *   *   */

void MainWindow::listItemChanged(QListWidgetItem* changed)
{

    bool checked = changed->checkState() == Qt::Checked;
    int index = 0;
    for (; changed->listWidget()->item(index) != changed; index++) ;

    QString modName = changed->data(Qt::UserRole).toString();
    if (checked)
    {
        settings->setValue(modName, Qt::Checked);
    }
    else
    {
        settings->setValue(modName, Qt::Unchecked);
    }
    settings->sync();
}

void MainWindow::buttonClickedRegister()
{
    QDesktopServices::openUrl(QUrl(settings->value("urlRegister").toString(), QUrl::TolerantMode));
}

void MainWindow::buttonClickedRun()
{
    execPath = settings->value("userOptions/pathGame").toString()+settings->value("internal/executableName").toString();

    QStringList arguments;
    QStringList keysList = settings->allKeys();

    foreach(QString key, keysList) {
        if (key.right(4) == ".mod" && settings->value(key) == Qt::Checked) {
            arguments << "-mod=mod/" + key;
        }
        else if (key.right(4) == ".dlc" && settings->value(key) == Qt::Unchecked) {
            arguments << "-exclude_dlc=dlc/" + key;
        }
    }

    QProcess *gameProcess = new QProcess();
    connect(gameProcess, SIGNAL(started()), SLOT(gameStarted()));
    connect(gameProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(gameError()));
    gameProcess->start(execPath, arguments);
    gameProcess->closeWriteChannel();
}

void MainWindow::gameError()
{
    QProcess *gameProcess = qobject_cast<QProcess *>(sender());
    QMessageBox error(this);
    QPushButton *configure = error.addButton(tr("Configure..."), QMessageBox::RejectRole);
    QPushButton *ok = error.addButton(tr("OK"), QMessageBox::AcceptRole);
    error.setDefaultButton(ok);
    error.setEscapeButton(ok);
    error.setWindowTitle(tr("Crusader Kings 2"));
    error.setText(tr("Failed to start %1: %2").arg(execPath, gameProcess->errorString()));
    error.setIcon(QMessageBox::Critical);
    error.exec();
    if (error.clickedButton() == configure) {
        FirstDialog dialog(this);
        dialog.exec();
    }
}

void MainWindow::gameStarted()
{
    // Quit after CK2 running
    QApplication::quit();
}

void MainWindow::webviewLoadFinished(bool status)
{
    if (!status) {
        ui->webView->load(QUrl(settings->value("userOptions/pathGame").toString()+"launcher/launcher.html"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
