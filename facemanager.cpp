#include "FaceManager.h"
#include <dlib/serialize.h>

FaceManager::FaceManager() : m_detector(dlib::get_frontal_face_detector()) {
    m_net = std::make_unique<anet_type>();
}

FaceManager::~FaceManager() = default;

std::string FaceManager::init(const std::string& shapePredictorPath, const std::string& faceRecognitionPath) {
    try {
        dlib::deserialize(shapePredictorPath) >> m_shapePredictor;
        dlib::deserialize(faceRecognitionPath) >> *m_net;
        m_initialized = true;
        return "Success";
    }
    catch (std::exception& e) {
        return e.what();
    }
}

std::vector<cv::Rect> FaceManager::detectFaces(const cv::Mat& frame) {
    dlib::cv_image<dlib::bgr_pixel> dlibImg(frame);
    auto dets = m_detector(dlibImg);

    std::vector<cv::Rect> results;
    for (auto& d : dets) {
        results.push_back(cv::Rect(d.left(), d.top(), d.width(), d.height()));
    }
    return results;
}

std::vector<float> FaceManager::extractEmbedding(const cv::Mat& frame, const cv::Rect& faceRect) {
    dlib::cv_image<dlib::bgr_pixel> dlibImg(frame);

    // 1. 얼굴 위치 추출
    dlib::rectangle rect(faceRect.x, faceRect.y, faceRect.x + faceRect.width, faceRect.y + faceRect.height);
    auto shape = m_shapePredictor(dlibImg, rect);

    // 2. 얼굴 정렬(Align) 및 150x150 변환
    dlib::matrix<dlib::rgb_pixel> face_chip;
    dlib::extract_image_chip(dlibImg, dlib::get_face_chip_details(shape, 150), face_chip);

    // 3. 임베딩 추출
    std::vector<dlib::matrix<dlib::rgb_pixel>> face_chips = { face_chip };
    auto embeddings = (*m_net)(face_chips);

    // 4. std::vector<float>로 변환
    std::vector<float> result;
    for (long i = 0; i < embeddings[0].nr(); ++i) {
        result.push_back(embeddings[0](i));
    }
    return result;
}