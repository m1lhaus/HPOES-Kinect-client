#include "serverclient.h"
#include "kinecthandler.h"
#include "segmentation.h"


ServerClient::ServerClient(QObject *parent) : QObject(parent)
{
}

ServerClient::ServerClient(QString hostUrl, quint16 socketPort, bool showImages, QObject *parent) : QObject(parent)
{
    this->hostName = hostUrl;
    this->port = socketPort;
    this->visualize = showImages;
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
    data.append("WIDTH 512");
    qDebug() << "Sending WIDTH to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }

    data.clear();
    data.append("HEIGHT 424");
    qDebug() << "Sending HEIGHT to server...";
    if (!sendData(data) || !waitForOK()) {
        qCritical() << "Server does not follow protocol, returning!";
        return;
    }

    data.clear();
    data.append("DTYPE uint16");
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
    Segmentation segmentation;
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
    double t0, exec_time;
    // --------------------

    if (this->visualize) {
        //cv::namedWindow("Depth");
        cv::namedWindow("Registered");
    }

    // --- MAIN FRAME GRABBING LOOP ---
    kinectHlander.startKinect();
    while (!this->stop) {
        // grab the frame
        //t0 = (double) cv::getTickCount();
        kinectHlander.grabFrame(depth, registered);
        //exec_time = ((double)cv::getTickCount() - t0)*1000./cv::getTickFrequency();
        //qDebug() << "Frame grab exec time:" << exec_time;

        // segment hand
        segmentation.segmentImage(depth, segmented);

        // send segmented hand to server
        if (!sendImage(segmented)) {
            qCritical() << "Image sending failed, terminating kinect recording...";
            break;
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
