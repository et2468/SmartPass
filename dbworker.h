#pragma once

#include <string>
#include <vector>
#include <libpq-fe.h>

// 학생 정보를 담는 구조체
struct Student {
    int id = -1;                     // 컴파일러 경고 해결을 위한 기본값 초기화 (-1: 유효하지 않음)
    std::string name;                // 학생 이름
    std::string studentId;           // 학번
    std::vector<float> embedding;    // 얼굴 특징 벡터 (예: dlib 128차원 벡터)
};

class DbWorker {
public:
    DbWorker();
    ~DbWorker();

    // PostgreSQL 데이터베이스 연결
    bool connect(const std::string& host, const std::string& port,
        const std::string& dbname, const std::string& user,
        const std::string& password);

    // 데이터베이스 연결 해제
    void disconnect();

    // 현재 데이터베이스 연결 상태 확인
    bool isConnected() const;

    // 새로운 학생 및 얼굴 임베딩 등록
    bool registerStudent(const std::string& name, const std::string& studentId,
        const std::vector<float>& embedding);

    // 얼굴 임베딩 유사도 검색 (출석 체크용 매칭)
    // threshold: 매칭으로 인정할 임계값 (예: pgvector 기준 거리가 0.6 이하)
    Student findSimilarStudent(const std::vector<float>& embedding, float threshold = 0.6f);

    // 출석 기록 저장 (학생 DB ID, 매칭 당시의 유사도 점수)
    bool recordAttendance(int studentId, float similarity);

private:
    PGconn* m_conn = nullptr;        // PostgreSQL 연결 핸들 포인터

    // C++의 vector<float> 데이터를 PostgreSQL 쿼리용 문자열(예: '[0.1, 0.2, ...]')로 변환
    std::string vectorToString(const std::vector<float>& vec);
};