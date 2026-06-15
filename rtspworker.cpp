#include "rtspworker.h"

// FFmpeg 라이브러리 헤더 파일 포함 (C 스타일 인터페이스)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// 생성자 함수 구현
RtspWorker::RtspWorker(const QString& url, QObject* parent)
    : QThread(parent), m_url(url), m_running(false) // m_running 초기화 추가
{
}

// RtspWorker 중지 함수
void RtspWorker::stop()
{
    m_running = false;
    wait(); // run() 함수가 완전히 종료될 때까지 MainThread가 대기
} // 👈 🚨 여기에 닫는 괄호가 누락되어 있었습니다! 고쳤습니다.

// RtspWorker 실행 함수
void RtspWorker::run()
{
    m_running = true;

    AVFormatContext* fmtCtx = nullptr;

    // RTSP 연결 옵션 설정
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);   // TCP 방식으로 RTSP 수신
    av_dict_set(&opts, "stimeout", "5000000", 0);      // 5초 타임아웃

    // RTSP 스트림 열기
    if (avformat_open_input(&fmtCtx, m_url.toStdString().c_str(), nullptr, &opts) < 0) {
        emit errorOccurred("RTSP 연결 실패: " + m_url);
        av_dict_free(&opts); // 자원 해제
        return;
    }
    av_dict_free(&opts); // 연결 성공 시에도 옵션 메모리 해제

    // 스트림 정보 가져오기
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        emit errorOccurred("스트림 정보를 가져올 수 없습니다.");
        avformat_close_input(&fmtCtx); // 👈 실패 시 오픈했던 fmtCtx 해제
        return;
    }

    // 비디오 스트림 인덱스 찾기
    int videoStream = -1;
    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if (videoStream < 0) {
        emit errorOccurred("비디오 스트림을 찾을 수 없습니다.");
        avformat_close_input(&fmtCtx); // 👈 실패 시 오픈했던 fmtCtx 해제
        return;
    }

    // 코덱 초기화
    AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        emit errorOccurred("알맞은 디코더를 찾을 수 없습니다.");
        avformat_close_input(&fmtCtx);
        return;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        emit errorOccurred("코덱 컨텍스트 할당 실패.");
        avformat_close_input(&fmtCtx);
        return;
    }

    avcodec_parameters_to_context(codecCtx, codecPar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        emit errorOccurred("코덱을 열 수 없습니다.");
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return;
    }

    // 프레임 및 패킷 할당
    AVFrame* frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    int width = codecCtx->width;
    int height = codecCtx->height;

    // RGB24 버퍼 할당
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t* buf = (uint8_t*)av_malloc(bufSize);
    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buf,
        AV_PIX_FMT_RGB24, width, height, 1);

    // 픽셀 포맷 변환 컨텍스트 (원본 포맷 → RGB24)
    SwsContext* swsCtx = sws_getContext(
        width, height, codecCtx->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    // 프레임 수신 루프
    while (m_running && av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index == videoStream) {
            // 패킷을 디코더에 전송
            if (avcodec_send_packet(codecCtx, packet) == 0) {
                // 디코딩된 프레임 수신
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    // RGB24로 변환
                    sws_scale(swsCtx,
                        frame->data, frame->linesize, 0, height,
                        frameRGB->data, frameRGB->linesize);

                    // QImage로 변환 후 시그널 전송
                    QImage img(frameRGB->data[0],
                        width, height,
                        frameRGB->linesize[0],
                        QImage::Format_RGB888);

                    emit frameReady(img.copy()); // copy()로 안전하게 deep copy 전달
                }
            }
        }
        av_packet_unref(packet); // 사용한 패킷 메모리 초기화
    }

    // [정상 루프 종료 및 중간 에러 발생 시 공통 자원 해제]
    sws_freeContext(swsCtx);
    av_free(buf);
    av_frame_free(&frameRGB);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
}