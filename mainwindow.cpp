#include "mainwindow.h"

// Qt
#include <QCoreApplication> 
#include <QMessageBox>
#include <QInputDialog> 
#include <qdir.h>
#include <QDebug>
#include <QMetaType>

// opencv
#include <opencv2/opencv.hpp>

// ui_mainWindow.h
#include "ui_mainwindow.h"


// 생성자 함수
MainWindow::MainWindow(QWidget* parent) :QMainWindow(parent)
{
    qDebug() << "MainWindow initiating";

    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<std::vector<cv::Rect>>("std::vector<cv::Rect>");

    // ui 도구
    m_ui = new Ui::MainWindow;
    m_ui->setupUi(this);

    connect(m_ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(m_ui->btnAttendance, &QPushButton::clicked, this, &MainWindow::onAttendanceClicked);

    // DB Worker
    m_dbWorker.connect("localhost", "5432", "ai_face_attendance_check", "postgres", "1234");

    // FaceDetectionWorker
    m_faceDetectionThread = new QThread(this);
    m_faceDetectionWorker = new FaceDetectionWorker();
    m_faceDetectionWorker->moveToThread(m_faceDetectionThread);

    m_faceDetectionThread->start();

    QMetaObject::invokeMethod(m_faceDetectionWorker, "initManager",
        Qt::QueuedConnection,
        Q_ARG(QString, "C:/Users/asus/Desktop/models/shape_predictor_68_face_landmarks.dat", ),
        Q_ARG(QString, "C:/Users/asus/Desktop/models/dlib_face_recognition_resnet_model_v1.dat"));

    connect(this, &MainWindow::requestDetection, m_faceDetectionWorker, &FaceDetectionWorker::processFrame);
    connect(m_faceDetectionWorker, &FaceDetectionWorker::resultReady, this, &MainWindow::onDetectionResult);
}

// 연결 버튼
void MainWindow::onConnectClicked()
{   
    if (m_rtspWorker) {
        m_rtspWorker->stop();
        delete m_rtspWorker;
        m_rtspWorker = nullptr;
        m_ui->btnConnect->setText(QStringLiteral("연결"));
        return;
    }

    QString url = m_ui->lineEditUrl->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("RTSP URL을 입력하세요."));
        return;
    }

	// rtspWorker 생성 및 시그널 연결
    m_rtspWorker = new RtspWorker(url, this);
	connect(m_rtspWorker, &RtspWorker::frameReady, this, &MainWindow::onFrameReady);
	connect(m_rtspWorker, &RtspWorker::errorOccurred, this, &MainWindow::onError);
    m_rtspWorker->start();
    m_ui->btnConnect->setText(QStringLiteral("연결 끊기"));
}

// 화면 랜더링 및 얼굴 감지 요청
void MainWindow::onFrameReady(QImage image)
{
    cv::Mat frame = cv::Mat(image.height(), image.width(), CV_8UC3,
        const_cast<uchar*>(image.bits()), image.bytesPerLine()).clone();
    cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

    if (m_attendanceMode && m_frameCount % 30 == 0) {
        m_isFaceDetectionWorkerBusy = true; 

        emit requestDetection(frame.clone());
    }

    cv::Scalar boxColor = m_attendanceMode ? cv::Scalar(255, 255, 0) : cv::Scalar(0, 255, 0);
    for (const auto& face : m_lastFaces) {
        cv::rectangle(frame, face, boxColor, 2);
    }

    m_frameCount++;

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage result(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    m_ui->labelVideo->setPixmap(QPixmap::fromImage(result).scaled(
        m_ui->labelVideo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// 얼굴 감지 결과 처리
void MainWindow::onDetectionResult(std::vector<cv::Rect> faces) {
    qDebug() << "face count: " << faces.size();

    m_lastFaces = faces;
    m_isFaceDetectionWorkerBusy = false;
    if (!faces.empty()) {
        m_attendanceMode = false; 
        m_ui->btnAttendance->setText(QStringLiteral("출석 체크!"));

        qDebug() << "### detecting successed! attendancd mode off";

    }
    else {
        m_failCount++;
        qDebug() << "### detecting fail counting" << m_failCount;
        if (m_failCount >= 10) {
            m_attendanceMode = false;
            m_ui->btnAttendance->setText(QStringLiteral("출석 체크!"));
            m_failCount = 0;
            qDebug() << "### fail counting over 10. attendancd mode off";
        }
    }
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
    //if (m_currentFrame.empty()) {
    //    QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("먼저 카메라를 연결하세요."));
    //    return;
    //}

    //// 생성자 단계에서 로드가 실패했을 경우를 대비한 버튼 클릭 시점 재시도 루틴
    //if (!m_faceManager.isInitialized()) {
    //    std::cout << "[재시도] dlib 모델이 로드되지 않아 다시 로드를 시도합니다..." << std::endl;

    //    std::string path1 = "C:/Users/asus/Desktop/models/shape_predictor_68_face_landmarks.dat";
    //    std::string path2 = "C:/Users/asus/Desktop/models/dlib_face_recognition_resnet_model_v1.dat";
    //    std::string errorResult = m_faceManager.init(path1, path2);
    //}

    //// 최종 실패 팝업
    //if (!m_faceManager.isInitialized()) {
    //    QString path = QDir(QCoreApplication::applicationDirPath()).filePath("models/shape_predictor_68_face_landmarks.dat");

    //    QString message = QStringLiteral("dlib 로드 실패!\n\n"
    //        "프로그램이 찾고 있는 실제 경로:\n%1\n\n"
    //        "해당 위치에 파일이 '진짜' 존재하는지 탐색기로 한 번 더 확인해주세요.")
    //        .arg(path);

    //    QMessageBox::critical(this, QStringLiteral("최종 확인"), message);
    //    return;
    //}

    //std::vector<cv::Rect> faces = m_faceManager.detectFaces(m_currentFrame);
    //if (faces.empty()) {
    //    QMessageBox::warning(this, QStringLiteral("경고"), QStringLiteral("얼굴을 감지할 수 없습니다."));
    //    return;
    //}

    //bool ok;
    //QString name = QInputDialog::getText(this, QStringLiteral("학생 등록"), QStringLiteral("이름:"), QLineEdit::Normal, "", &ok);
    //if (!ok || name.isEmpty()) return;

    //QString studentId = QInputDialog::getText(this, QStringLiteral("학생 등록"), QStringLiteral("학번:"), QLineEdit::Normal, "", &ok);
    //if (!ok || studentId.isEmpty()) return;

    //auto embedding = m_faceManager.extractEmbedding(m_currentFrame, faces[0]);
    //if (m_dbWorker.registerStudent(name.toStdString(), studentId.toStdString(), embedding)) {
    //    QMessageBox::information(this, QStringLiteral("성공"), name + QStringLiteral(" 학생이 등록되었습니다."));
    //}
    //else {
    //    QMessageBox::critical(this, QStringLiteral("오류"), QStringLiteral("등록에 실패했습니다."));
    //}
}

void MainWindow::onAttendanceClicked()
{
    m_attendanceMode = !m_attendanceMode;
    m_ui->btnAttendance->setText(m_attendanceMode ? QStringLiteral("출석 체크 중지") : QStringLiteral("출석 체크!"));
}



// 리소스 해제
MainWindow::~MainWindow()
{
    m_faceDetectionThread->quit();
    m_faceDetectionThread->wait();
    delete m_faceDetectionWorker;

    if (m_rtspWorker) {
        m_rtspWorker->stop();
        delete m_rtspWorker;
    }
    delete m_ui;
}