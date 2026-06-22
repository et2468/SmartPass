#include "FaceDetectionWorker.h"

FaceDetectionWorker::FaceDetectionWorker(QObject* parent) : QObject(parent) {}

void FaceDetectionWorker::initManager(QString shapePath, QString recPath) {
    bool success = m_manager.init(shapePath.toStdString(), recPath.toStdString());
}

void FaceDetectionWorker::processFrame(cv::Mat frame) {
    // 모델이 초기화되지 않았다면 작업 수행 안 함
    if (!m_manager.isInitialized()) return;

    std::vector<cv::Rect> faces = m_manager.detectFaces(frame);
    emit resultReady(faces);
}
