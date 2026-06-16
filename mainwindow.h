#pragma once

#include <QMainWindow>
#include <QTimer>
#include "rtspworker.h"
#include "facemanager.h"
#include "dbworker.h"

// Qt의 네임스페이스 정의
QT_BEGIN_NAMESPACE 
// ui_mainwindow.h의 Ui::MainWindow와 연결되는 부분
// 정방향 선언으로 클래스 이름만 알려주어 순환참조를 방지하고 컴파일 시간을 줄이는 효과가 있음
namespace Ui { class MainWindow; } 
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
	Q_OBJECT // Qt의 시그널/슬롯 메커니즘을 사용하기 위한 매크로

    // 0.생성 및 소멸 함수
    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow();
    
    // 1.이벤트 함수들: Qt의 slot은 시그널과 연결되어 특정 이벤트가 발생했을 때 자동으로 호출되는 함수
    private slots:
        void onConnectClicked();
        void onFrameReady(QImage image);
        void onError(QString message);
        void onRegisterClicked();
        void onAttendanceClicked();

	// 2.내부적으로 사용할 멤버 변수들
    private:
        // UI 및 화면 통제
        Ui::MainWindow* m_ui;

        // 백엔드 핵심 일꾼 (기능 매니저들)
        RtspWorker* m_rtspWorker = nullptr; // 실시간 영상 수집
        FaceManager m_faceManager;          // AI 얼굴 인식 엔진
        DbWorker m_dbWorker;                // DB 연동 및 관리

        // 실시간 데이터 및 상태 스위치
        cv::Mat m_currentFrame;             // 현재 처리 중인 영상 프레임 (OpenCV)
        bool m_attendanceMode = false;      // 출석 체크 가동 상태 (True/False)


        int m_frameCount = 0;         // 전체 프레임 카운트
        int m_failCount = 0;          // [추가] 출석 인식 실패 횟수 카운터
        std::vector<cv::Rect> m_lastFaces;
};