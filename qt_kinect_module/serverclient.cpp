#include "serverclient.h"
#include "kinecthandler.h"
#include "segmentation/segmentation.h"


ServerClient::ServerClient(QObject *parent) : QObject(parent)
{
}

ServerClient::ServerClient(QString hostUrl, quint16 socketPort, bool showImages, bool mirror, QObject *parent) : QObject(parent)
{
    this->hostName = hostUrl;
    this->port = socketPort;
    this->visualize = showImages;
    this->mirrorHand = mirror;
}

ServerClient::~ServerClient()
{
    disconnectFromHost();
}

void ServerClient::run()
{
    qDebug() << "serverClient started" << hostName << port;
    if (!connectToHost()) {
        qCritical() << "Connection to host failed!";
        return;
    }
    emit message("Connected to server on " + this->hostName + ":" + QString::number(this->port));

    QByteArray data = QByteArray::fromStdString("HELLO");
    qDebug() << "Sending HELLO to server...";
    if (!sendData(data) || !waitForHELLO()) {
        qCritical() << "Server-client handshake failed";
        return;
    }

    data.clear();
    data.append("WIDTH 96");
    qDebug() << "Sending WIDTH to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }

    data.clear();
    data.append("HEIGHT 96");
    qDebug() << "Sending HEIGHT to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }

    data.clear();
    data.append("DTYPE float32");
    qDebug() << "Sending DTYPE to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }

    data.clear();
    data.append("DATA");
    qDebug() << "Sending DATA flag to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }
    emit message("Communication protocol done");

    // --- INIT SEGMENTATION ---
    Segmentation segmentation(96, 96);
    cv::Mat segmented;
    // -------------------------

    // --- INIT KINNECT ---
    KinectHandler kinectHlander;
    if (!kinectHlander.initKinnect()) {
        qCritical() << "Kinnect initialization failed";
        return;
    }
    emit message("Kinect initialized, recording...");
    cv::Mat depth, registered;
    // --------------------

    if (this->visualize) {
        //cv::namedWindow("Depth");
        cv::namedWindow("Registered");
    }

    QElapsedTimer kinectTimeMeasure, segTimeMeasure, tcpTimeMeasure;

    // --- MAIN FRAME GRABBING LOOP ---
    kinectHlander.startKinect();
    while (!this->stop) {
//        this->thread()->msleep(500);

        // grab the frame
        // kinectTimeMeasure.restart();
        kinectHlander.grabFrame(depth, registered);
        // qDebug() << "Frame grab exec time:" << kinectTimeMeasure.elapsed() << "ms";

        // segment hand
        // segTimeMeasure.restart();
        if (segmentation.segmentImage(registered, depth, segmented)) {
            // qDebug() << "Segmentation exec time:" << segTimeMeasure.elapsed() << "ms";

            if (this->mirrorHand) {
                Mat flipped;
                cv::flip(segmented, flipped, 1);
                segmented = flipped;
            }

            // send segmented hand to server
            // tcpTimeMeasure.restart();
            if (!sendImage(segmented) || !waitForOK()) {
                qCritical() << "Image sending failed, terminating kinect recording...";
                break;
            }
            // qDebug() << "Tcp data send time:" << tcpTimeMeasure.elapsed() << "ms";

        } else {
            qDebug() << "No hand segmented!";
        }

        if (this->visualize) {
            //cv::imshow("Depth", depth);
            cv::imshow("Registered", registered);
        }
    }
    kinectHlander.stopKinnect();
    emit message("Kinect stopped, now closing...");
    // --------------------------------

    if (this->visualize)
        cv::destroyAllWindows();
}

void ServerClient::close()
{
    qDebug() << "Closing ServerClient...";
}


bool ServerClient::connectToHost()
{
    socket = new QTcpSocket(this);
    socket->connectToHost(hostName, port);
    if (!socket->waitForConnected(timeout)) {
        emit errorMessage(socket->error(), socket->errorString());
        qCritical() << "Unable to connect" << socket->errorString();
        return false;
    } else {
        qDebug() << "Connected to host:" << hostName << ":" << port;
        emit connected();
        return true;
    }
}

bool ServerClient::disconnectFromHost()
{
    if (!socket || !socket->isOpen()) {
        qCritical() << "There is no opened socket to close!";
        return false;
    }

    socket->disconnectFromHost();
    if (socket->state() == QAbstractSocket::UnconnectedState || socket->waitForDisconnected(timeout)) {
        qDebug("Client disconnected from host");
        emit disconnected();
        return true;
    } else {
        emit errorMessage(socket->error(), socket->errorString());
        qCritical() << "Unable to disconnect from host! Error:" << socket->error() << socket->errorString();
        return false;
    }
}

bool ServerClient::waitForHELLO()
{
    QByteArray data;
    waitRecvData(data);
    qDebug() << "Received answer:" << data;

    if (data != "HELLO") {
        qCritical() << "Expected HELLO answer, but got" << data << "instead!";
        return false;
    } else {
        return true;
    }
}

bool ServerClient::waitForOK()
{
    QByteArray data;
    waitRecvData(data);
    qDebug() << "Received answer:" << data;
    if (data != "OK") {
        qCritical() << "Expected OK answer, but got" << data << "instead!";
        return false;
    } else {
        return true;
    }
}

bool ServerClient::sendData(QByteArray &data)
{
    if (!socket || !socket->isWritable()) {
        qCritical() << "There is no opened socket to write!";
        return false;
    }

    socket->write(data);
    if (!socket->waitForBytesWritten(timeout)) {
        emit errorMessage(socket->error(), socket->errorString());
        qCritical() << "Error when writing data to socket!" << socket->error() << socket->errorString();
        return false;
    } else {
        qDebug() << data.size() << "bytes written to socket";
        return true;
    }
}

bool ServerClient::sendImage(const cv::Mat &img)
{
    const size_t data_size = img.cols * img.rows * img.elemSize();
    QByteArray data = QByteArray::fromRawData( (const char*)img.ptr(), data_size );
    return sendData(data);
}

bool ServerClient::recvData(QByteArray &output)
{
    int bytesReceived = 0;
    while (socket->bytesAvailable()) {
        qDebug() << "Reading" << socket->bytesAvailable() << "bytes...";
        QByteArray data = socket->readAll();
        bytesReceived += data.size();
        output.append(data);
    }
    qDebug() << bytesReceived << "bytes received";
    return true;
}

bool ServerClient::waitRecvData(QByteArray &output, int msecs)
{
    qDebug() << "Waiting for data to receive...";
    if (socket->waitForReadyRead(msecs)) {
        return recvData(output);

    } else {
        emit errorMessage(socket->error(), socket->errorString());
        qCritical() << "Error when reading from socket!" << socket->error() << socket->errorString();
        return false;
    }
}
