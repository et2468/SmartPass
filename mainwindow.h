#pragma once

#include <QMainWindow>
#include <QTimer>

#include <QVector>
#include <QThread>

#include "rtspworker.h"
#include "dbworker.h"
#include "facedetectionworker.h"

// Qt의 네임스페이스 정의
QT_BEGIN_NAMESPACE 
// ui_mainwindow.h의 Ui::MainWindow와 연결되는 부분
// 정방향 선언으로 클래스 이름만 알려주어 순환참조를 방지하고 컴파일 시간을 줄이는 효과가 있음
namespace Ui { class MainWindow; } 
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
	Q_OBJECT // Qt의 시그널/슬롯 메커니즘을 사용하기 위한 매크로

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow();
    
    signals:
        // 여기에 시그널을 추가하세요!
        // 워커에게 cv::Mat 데이터를 보내야 하므로 타입은 cv::Mat입니다.
        void requestDetection(cv::Mat frame);

    private slots:
        void onConnectClicked();
        void onFrameReady(QImage image);
        void onError(QString message);
        void onRegisterClicked();
        void onAttendanceClicked();

        void onDetectionResult(std::vector<cv::Rect> faces);

    private:
        Ui::MainWindow* m_ui;
        
        QThread* m_faceDetectionThread;
        FaceDetectionWorker* m_faceDetectionWorker;
        bool m_isFaceDetectionWorkerBusy = false;
        std::vector<cv::Rect> m_lastFaces; // 화면에 그릴 마지막 얼굴 좌표


        RtspWorker* m_rtspWorker = nullptr; 
        DbWorker m_dbWorker;               


        // 실시간 데이터 및 상태 스위치
        cv::Mat m_currentFrame;             // 현재 처리 중인 영상 프레임 (OpenCV)
        bool m_attendanceMode = false;      // 출석 체크 가동 상태 (True/False)


        int m_frameCount = 0;         // 전체 프레임 카운트
        int m_failCount = 0;          // [추가] 출석 인식 실패 횟수 카운터
};