#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDebug>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <opencv2/opencv.hpp>

#include "serverclient.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();
    void connectedToServer();
    void disconnectedFromServer();
    void displayError(int errNum, const QString &message);
    void displayMessage(const QString &message, int timeout = 0);

private:
    Ui::MainWindow *ui;

    void makeConnection();
    void closeEvent(QCloseEvent *event);

    ServerClient *serverClient = nullptr;
    QThread *serverClientThread = nullptr;
};

#endif // MAINWINDOW_H
