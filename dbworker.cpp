#include "dbworker.h"
#include <sstream>
#include <iostream>

DbWorker::DbWorker() {}

DbWorker::~DbWorker()
{
    disconnect();
}

bool DbWorker::connect(const std::string& host, const std::string& port,
    const std::string& dbname, const std::string& user,
    const std::string& password)
{
    std::string connStr = "host=" + host + " port=" + port +
        " dbname=" + dbname + " user=" + user +
        " password=" + password;

    m_conn = PQconnectdb(connStr.c_str());

    if (PQstatus(m_conn) != CONNECTION_OK) {
        std::cerr << "DB 연결 실패: " << PQerrorMessage(m_conn) << std::endl;
        PQfinish(m_conn);
        m_conn = nullptr;
        return false;
    }
    return true;
}

void DbWorker::disconnect()
{
    if (m_conn) {
        PQfinish(m_conn);
        m_conn = nullptr;
    }
}

bool DbWorker::isConnected() const
{
    return m_conn && PQstatus(m_conn) == CONNECTION_OK;
}

std::string DbWorker::vectorToString(const std::vector<float>& vec)
{
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); i++) {
        if (i > 0) oss << ",";
        oss << vec[i];
    }
    oss << "]";
    return oss.str();
}

bool DbWorker::registerStudent(const std::string& name, const std::string& studentId,
    const std::vector<float>& embedding)
{
    if (!isConnected()) return false;

    std::string vecStr = vectorToString(embedding);
    std::string query = "INSERT INTO students (name, student_id, face_embedding) "
        "VALUES ('" + name + "', '" + studentId + "', '" + vecStr + "') "
        "ON CONFLICT (student_id) DO UPDATE SET face_embedding = EXCLUDED.face_embedding";

    PGresult* res = PQexec(m_conn, query.c_str());
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok)
        std::cerr << "학생 등록 실패: " << PQerrorMessage(m_conn) << std::endl;
    PQclear(res);
    return ok;
}

Student DbWorker::findSimilarStudent(const std::vector<float>& embedding, float threshold)
{
    Student student; // 구조체 내부에서 id = -1로 기본 초기화됨
    if (!isConnected()) return student;

    std::string vecStr = vectorToString(embedding);

    // 임계값(threshold) 조건 조정을 위해 쿼리 단계에서 코사인 유사도(1 - 거리)를 계산하고, 
    // 최소 임계값을 넘는 데이터 중에서 가장 유사도가 높은(가장 거리가 가까운) 1건만 조회합니다.
    std::string query = "SELECT id, name, student_id, "
        "1 - (face_embedding <=> '" + vecStr + "'::vector) AS similarity "
        "FROM students "
        "WHERE (1 - (face_embedding <=> '" + vecStr + "'::vector)) >= " + std::to_string(threshold) + " "
        "ORDER BY face_embedding <=> '" + vecStr + "'::vector ASC "
        "LIMIT 1";

    PGresult* res = PQexec(m_conn, query.c_str());

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        student.id = std::stoi(PQgetvalue(res, 0, 0));
        student.name = PQgetvalue(res, 0, 1);
        student.studentId = PQgetvalue(res, 0, 2);
        // 필요시 검색된 임베딩 정보를 결과 구조체에 채울 수도 있습니다.
    }

    PQclear(res);
    return student;
}

bool DbWorker::recordAttendance(int studentId, float similarity)
{
    if (!isConnected() || studentId == -1) return false;

    std::string query = "INSERT INTO attendance (student_id, similarity) "
        "VALUES (" + std::to_string(studentId) + ", " +
        std::to_string(similarity) + ")";

    PGresult* res = PQexec(m_conn, query.c_str());
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok)
        std::cerr << "출석 기록 실패: " << PQerrorMessage(m_conn) << std::endl;
    PQclear(res);
    return ok;
}