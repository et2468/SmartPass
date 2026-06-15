#pragma once

#include <QThread>
#include <QImage>

// Ui Thread가 포그라운드에서 사용자와 상호작용하고
// RtspWorker(QThread)는 백그라운드에서 RTSP 스트림을 처리
class RtspWorker : public QThread
{
	Q_OBJECT // Q_OBJECT 매크로: 해당 class가 Qt 기능을 사용할 수 있게 해줌 (예: 시그널과 슬롯)

	// 0.생성자 함수 선언: RTSP URL을 받고, 부모로 QObject(MainWindow)를 받을 예정
    public:
        explicit RtspWorker(const QString &url, QObject *parent = nullptr);
	
	// 1.일반 함수
	public:
		void stop(); // 스레드 중지 함수

	// 2.시그널 함수: Qt 라이브러리 함수
    signals:
		void frameReady(QImage image);          // 프레임이 준비되었을 때 UI Thread로 QImage를 전달하는 시그널
		void errorOccurred(QString message);    // 오류가 발생했을 때 UI Thread로 오류 메시지를 전달하는 시그널

	// 3.스레드 실행 함수: QThread의 run() 함수를 오버라이드하여 RTSP 스트림을 처리하는 로직을 구현할 예정
    protected:
        void run() override;

	// 4.멤버 변수 선언
    private:
		QString m_url;			// RTSP URL을 저장하는 멤버 변수
		bool m_running = false;	// 스레드 실행 상태를 나타내는 플래그 변수
};