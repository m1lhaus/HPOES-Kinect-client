#ifndef SERVERCLIENT_H
#define SERVERCLIENT_H

#include <QObject>
#include <QApplication>
#include <QtNetwork>

#include <opencv2/opencv.hpp>

class ServerClient : public QObject
{
    Q_OBJECT
public:
    explicit ServerClient(QObject *parent = 0);
    ServerClient(QString hostUrl, quint16 socketPort, bool showImages, bool mirror, QObject *parent = 0);
    ~ServerClient();

    bool sendData(QByteArray &data);
    bool sendImage(const cv::Mat &img);
    bool recvData(QByteArray &output);
    bool waitRecvData(QByteArray &output, int msecs=5000);

    bool stop = false;
    bool mirrorHand = false;
    bool visualize = false;
    QString hostName;
    quint16 port;
    const int timeout = 5000; //ms

private:
    bool connectToHost();
    bool disconnectFromHost();

    bool waitForHELLO();
    bool waitForOK();

    QTcpSocket *socket = nullptr;

signals:
    void errorMessage(int socketError, const QString &message);
    void message(const QString &message, int timeout = 0);
    void connected();
    void disconnected();

private slots:
    void run();

public slots:
    void close();

};

#endif // SERVERCLIENT_H
