#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // move window to center
    QRect screenGeometry = QApplication::desktop()->screenGeometry(QApplication::desktop()->primaryScreen());
    int x = (screenGeometry.width()-this->width()) / 2;
    int y = (screenGeometry.height()-this->height()) / 2;
    this->move(x, y);
    this->show();

    this->setFixedSize(this->sizeHint());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::makeConnection()
{
    qDebug() << "MainWindow::makeConnection" << "Initializing serverClientThread and serverClient";

    QString hostName = ui->hostEdit->text();
    qint16 port = ui->portEdit->text().toInt();
    bool visualize = ui->visCheck->isChecked();

    // find out if left or right hand will be used
    bool mirror;
    if (ui->handCombo->currentText() == "Left") {
        mirror = false;
    } else {
        mirror = true;
    }

    // run serverClient in separated Thread
    serverClientThread = new QThread(this);
    serverClient = new ServerClient(hostName, port, visualize, mirror);
    serverClient->moveToThread(serverClientThread);

    connect(serverClientThread, SIGNAL(started()), serverClient, SLOT(run()));
    connect(serverClient, SIGNAL(connected()), this, SLOT(connectedToServer()));
    connect(serverClient, SIGNAL(disconnected()), this, SLOT(disconnectedFromServer()));

    connect(serverClient, SIGNAL(errorMessage(int,QString)), this, SLOT(displayError(int,QString)));
    connect(serverClient, SIGNAL(message(QString,int)), this, SLOT(displayMessage(QString,int)));

    serverClientThread->start();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "MainWindow::closeEvent";
    if(serverClientThread && serverClientThread->isRunning()) {
        qDebug() << "Closing ServerClientThread...";
        serverClient->stop = true;
        serverClientThread->quit();
        if(serverClientThread->wait(3000)) {
            qDebug() << "ServerClientThread closed successfully";
        } else {
            qCritical() << "Unable to close ServerClientThread";
        }
    }
    event->accept();
}

void MainWindow::on_connectButton_clicked()
{
    makeConnection();
}

void MainWindow::connectedToServer()
{
    qDebug() << "MainWindow::connectedToServer";
    this->ui->connectButton->setEnabled(false);
    this->ui->visCheck->setEnabled(false);;
}

void MainWindow::disconnectedFromServer()
{
    qDebug() << "MainWindow::disconnectedFromServer";
}

void MainWindow::displayError(int errNum, const QString &message)
{
    this->statusBar()->showMessage("ERROR: " + QString::number(errNum) + ", " + message);
}

void MainWindow::displayMessage(const QString &message, int timeout)
{
    this->statusBar()->showMessage(message, timeout);
}

void MainWindow::on_handCombo_currentIndexChanged(const QString &arg1)
{
    qDebug() << "Hand orientation switched to" << arg1;
    if (this->serverClient != nullptr) {
        if (arg1 == "Left") {
            this->serverClient->mirrorHand = false;
        } else {
            this->serverClient->mirrorHand = true;
        }
        qDebug() << "Hand orientation changed in serverClient module";
    }

}
