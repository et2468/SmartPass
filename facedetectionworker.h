#ifndef FACEDETECTIONWORKER_H
#define FACEDETECTIONWORKER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include "FaceManager.h"

class FaceDetectionWorker : public QObject {
    Q_OBJECT
public:
    explicit FaceDetectionWorker(QObject* parent = nullptr);

public slots:
    void initManager(QString shapePath, QString recPath);
    void processFrame(cv::Mat frame);

signals:
    void resultReady(std::vector<cv::Rect> faces);
private:
    FaceManager m_manager;
};
#endif