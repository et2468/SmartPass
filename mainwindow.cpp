#include "mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>

// opencvㅇ의 핵심 헤더 파일로, 이미지 처리와 관련된 모든 기능을 포함하는 종합적인 헤더
#include <opencv2/opencv.hpp>

#include <QCoreApplication> // 실행 파일 위치 추적용
#include <QDir>             // 윈도우 스타일 경로(\) 변환용
#include <iostream>         // 콘솔 디버깅 출력용

#include "ui_mainwindow.h" // Qt의 레이아웃과 위젯을 정의하는 헤더 (Qt의 UI 디자이너에서 자동으로 생성)

#include <dlib/revision.h>

// 0.MainWindow 클래스 생성자: UI 셋업, DB 연결, dlib 모델 로드, 버튼과 슬롯 함수 연결
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent)
{
	// UI Manager 셋업
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);

    connect(m_ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(m_ui->btnAttendance, &QPushButton::clicked, this, &MainWindow::onAttendanceClicked);

    // DB Worker 셋업
    m_dbWorker.connect("localhost", "5432", "ai_face_attendance_check", "postgres", "1234");

    // dlib 모델 로드
    QString appDir = QCoreApplication::applicationDirPath();

    std::string path1 = "C:/Users/asus/Desktop/models/shape_predictor_68_face_landmarks.dat";
    std::string path2 = "C:/Users/asus/Desktop/models/dlib_face_recognition_resnet_model_v1.dat";

    // dlib가 제공하는 DLIB_MAJOR_VERSION(예: 19)과 DLIB_MINOR_VERSION(예: 24)을 활용합니다.
    QString dlibVersionStr = QString("현재 설치된 dlib 버전: %1.%2")
        .arg(DLIB_MAJOR_VERSION)
        .arg(DLIB_MINOR_VERSION);

    // 3. 팝업창으로 짜잔! 띄우기
    QMessageBox::information(this, "vcpkg dlib 버전 확인", dlibVersionStr);


    // 함수를 실행하고 그 결과(리턴값)를 변수에 바로 받습니다.
    std::string errorResult = m_faceManager.init(path1, path2);

    // 리턴값이 텅 비어있다면? -> 에러 없이 성공했다는 뜻!
    if (errorResult.empty()) {
        QMessageBox::information(this, "성공", "🎉 dlib 인공지능 모델 로드에 성공했습니다!");
    }
    // 리턴값에 에러 내용(e)이 들어있다면? -> 팝업으로 띄우기!
    else {
        QString guiErrorMsg = QString::fromLocal8Bit(errorResult.c_str());
        QMessageBox::critical(this, "dlib 로드 에러 발생", guiErrorMsg);
    }
}

// 1.연결 버튼에 적용되는 슬롯 함수
void MainWindow::onConnectClicked()
{   
	// 이미 연결된 상태라면 연결 해제, rtspWorker 중지 및 메모리 해제, 버튼 텍스트 원복
    if (m_rtspWorker) {
        m_rtspWorker->stop();
        delete m_rtspWorker;
        m_rtspWorker = nullptr;
        m_ui->btnConnect->setText(QStringLiteral("연결"));
        return;
    }

	// 입력된 URL이 비어있으면 경고 팝업을 띄우고 함수 종료
    QString url = m_ui->lineEditUrl->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("RTSP URL을 입력하세요."));
        return;
    }

	// RtspWorker 생성 및 시작, 시그널 연결, 버튼 텍스트 변경
    m_rtspWorker = new RtspWorker(url, this);
	connect(m_rtspWorker, &RtspWorker::frameReady, this, &MainWindow::onFrameReady); // 프레임이 준비되었을 때 onFrameReady 슬롯 함수가 호출되도록 연결
	connect(m_rtspWorker, &RtspWorker::errorOccurred, this, &MainWindow::onError);  // 오류가 발생했을 때 onError 슬롯 함수가 호출되도록 연결
    m_rtspWorker->start(); // QThread의 start()함수 안에 run()함수가 호출
    m_ui->btnConnect->setText(QStringLiteral("연결 끊기"));
}

void MainWindow::onFrameReady(QImage image)
{
    cv::Mat frame = cv::Mat(image.height(), image.width(), CV_8UC3,
        const_cast<uchar*>(image.bits()),
        image.bytesPerLine()).clone();
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
    m_currentFrame = frame;

    // 얼굴 감지는 30프레임마다 한 번만
    if (m_frameCount % 30 == 0 && m_faceManager.isInitialized()) {
        m_lastFaces = m_faceManager.detectFaces(frame);
    }
    m_frameCount++;

    for (const auto& face : m_lastFaces) {
        cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
    }

    // 화면 표시만 매 프레임
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage result(frame.data, frame.cols, frame.rows,
        frame.step, QImage::Format_RGB888);
    m_ui->labelVideo->setPixmap(
        QPixmap::fromImage(result).scaled(
            m_ui->labelVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)
    );
}

void MainWindow::onError(QString message)
{
    QMessageBox::critical(this, QStringLiteral("오류"), message);
    m_ui->btnConnect->setText(QStringLiteral("연결"));
    m_rtspWorker->deleteLater();
    m_rtspWorker = nullptr;
}

void MainWindow::onRegisterClicked()
{
    if (m_currentFrame.empty()) {
        QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("먼저 카메라를 연결하세요."));
        return;
    }

    // 🌟 생성자 단계에서 로드가 실패했을 경우를 대비한 버튼 클릭 시점 재시도 루틴
    if (!m_faceManager.isInitialized()) {
        std::cout << "[재시도] dlib 모델이 로드되지 않아 다시 로드를 시도합니다..." << std::endl;

        std::string path1 = "C:/Users/asus/Desktop/models/shape_predictor_68_face_landmarks.dat";
        std::string path2 = "C:/Users/asus/Desktop/models/dlib_face_recognition_resnet_model_v1.dat";
        std::string errorResult = m_faceManager.init(path1, path2);
    }

    // 최종 실패 팝업
    if (!m_faceManager.isInitialized()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString cleanPath = QDir::toNativeSeparators(appDir + "/models/shape_predictor_68_face_landmarks.dat");

        QMessageBox::critical(this, QStringLiteral("최종 확인"),
            QStringLiteral("dlib 로드 실패!\n\n프로그램이 찾고 있는 실제 경로:\n") + cleanPath +
            QStringLiteral("\n\n해당 위치에 파일이 '진짜' 존재하는지 탐색기로 한 번 더 확인해주세요."));
        return;
    }

    std::vector<cv::Rect> faces = m_faceManager.detectFaces(m_currentFrame);
    if (faces.empty()) {
        QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("얼굴을 감지할 수 없습니다."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getText(this, QStringLiteral("학생 등록"), QStringLiteral("이름:"), QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString studentId = QInputDialog::getText(this, QStringLiteral("학생 등록"), QStringLiteral("학번:"), QLineEdit::Normal, "", &ok);
    if (!ok || studentId.isEmpty()) return;

    auto embedding = m_faceManager.extractEmbedding(m_currentFrame, faces[0]);
    if (m_dbWorker.registerStudent(name.toStdString(), studentId.toStdString(), embedding)) {
        QMessageBox::information(this, QStringLiteral("성공"), name + QStringLiteral(" 학생이 등록되었습니다."));
    }
    else {
        QMessageBox::critical(this, QStringLiteral("오류"), QStringLiteral("등록에 실패했습니다."));
    }
}

void MainWindow::onAttendanceClicked()
{
    m_attendanceMode = !m_attendanceMode;
    m_ui->btnAttendance->setText(m_attendanceMode ? QStringLiteral("출석 체크 중지") : QStringLiteral("출석 체크 시작"));
}


// 99.MainWindow 소멸될 때, 리소스를 해제
MainWindow::~MainWindow()
{
    if (m_rtspWorker) {
        m_rtspWorker->stop();
        delete m_rtspWorker;
    }
    delete m_ui;
}