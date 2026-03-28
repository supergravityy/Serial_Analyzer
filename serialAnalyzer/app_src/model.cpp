#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "config.h"
#include "model.h"

#define ANL_CSV_BUFF_SIZE		(ANL_TX_BUFF_SIZE)

analyzerModel::analyzerModel()
{
	this->elapsed_time = 0.0f;
	this->x_axis_range = 5;
	this->targetDomain = "";
	this->rx_remainder = "";
	this->errCode = MODL_ERR_NONE;
	memset(this->tx_buffer, 0, ANL_TX_BUFF_SIZE);
}

analyzerModel::~analyzerModel()
{
	this->logs.clear();
	this->clear_graphData(); // 기존 Erase() 호출 대체
}

// --- 그래프 데이터 제어 (링 버퍼 로직) ---
void analyzerModel::add_graphData(const std::string& domain, float x, float y)
{
	// 맵에 해당 도메인이 없으면 ScrollingBuffer 구조체의 생성자가 자동 호출되며 새로 만들어짐
	ScrollingBuffer& tempData = this->multiData[domain];

	if (tempData.data.size() < tempData.MaxSize)
	{
		tempData.data.push_back(ImVec2(x, y));
	}
	else
	{
		tempData.data[tempData.cursor] = ImVec2(x, y);
		tempData.cursor = (tempData.cursor + 1) % tempData.MaxSize;
	}
}

void analyzerModel::clear_graphData(void)
{
	// 등록된 모든 도메인의 데이터를 비움
	for (auto& pair : this->multiData)
	{
		pair.second.data.clear();
		pair.second.cursor = 0;
	}
}

// --- 도메인 관리 및 데이터 파서 (핵심) ---
void analyzerModel::parse_teleplot_data(const std::string& raw_data)
{
	if (raw_data.empty()) return;

	// 이전 턴에서 덜 받은 문자열이 있다면 합침
	this->rx_remainder += raw_data;

	size_t posIdx = 0, sepIdx = 0;
	size_t next_pos = 0;
	std::string tempLine = "", domainName = "", value_str = "";
	float value = 0;

	// '\n' 기준으로 한 줄씩 추출
	while ((next_pos = this->rx_remainder.find('\n', posIdx)) != std::string::npos)
	{
		std::string line = this->rx_remainder.substr(posIdx, next_pos - posIdx);
		posIdx = next_pos + 1;

		// '\r'이 포함되어 있다면 제거 (Windows/Linux 차이 방지)
		if (!line.empty() && line.back() == '\r') line.pop_back();

		// Teleplot 파싱 로직: ">"를 찾음
		sepIdx = line.find('>');
		if (sepIdx != std::string::npos)
		{
			domainName = line.substr(0, sepIdx);
			value_str = line.substr(sepIdx + 1);

			try {
				value = std::stof(value_str);
				this->add_graphData(domainName, this->elapsed_time, value);
			}
			catch (...) {
				// stod/stof 에러 발생 시 (쓰레기값 수신) 무시
			}
		}
	}
	// 개행 문자가 없이 남은 부분은 짤린 패킷이므로 다시 보관
	this->rx_remainder = this->rx_remainder.substr(posIdx);
}

void analyzerModel::get_domain_names(std::vector<std::string>& domainSpace)
{
//#if(ANL_RUN_MODE == ANL_SERI_MODE)
	domainSpace.reserve(this->multiData.size());

	for (const auto& pair : this->multiData)
	{
		domainSpace.push_back(pair.first); // 맵의 Key(도메인명) 추출
	}
}

void analyzerModel::set_targetDomain(const std::string& domain)
{
	this->targetDomain = domain;
}

std::string analyzerModel::get_targetDomain(void)
{
	return this->targetDomain;
}

ScrollingBuffer* analyzerModel::get_targetBuffer(void)
{
	if (this->targetDomain.empty()) return nullptr;

	auto it = this->multiData.find(this->targetDomain);
	if (it != this->multiData.end())
	{
		return &(it->second);
	}
	return nullptr;
}

void analyzerModel::add_log_with_time(float elapsed_time, const std::string& log)
{
	// 1. 시간 변환 (elapsed_time은 누적 초 단위)
	int total_ms = (int)(elapsed_time * 1000);
	int h = (total_ms / 3600000);
	int m = (total_ms % 3600000) / 60000;
	int s = (total_ms % 60000) / 1000;
	int ms = total_ms % 1000;
	char time_str[ANL_CSV_BUFF_SIZE];
	std::string log_with_time;

	sprintf_s(time_str, "%02d:%02d:%02d.%03d", h, m, s, ms);

	// 2. "시간, 데이터" 형태로 한 줄 완성 (CSV 호환을 위해 쉼표 사용)
	log_with_time = "(" + std::string(time_str) + "), " + log;

	// 3. 기록 (개행 문자가 포함되어 있다면 제거 후 저장)
	if (!log_with_time.empty() && log_with_time.back() == '\n') log_with_time.pop_back();
	if (!log_with_time.empty() && log_with_time.back() == '\r') log_with_time.pop_back();

	this->csv_history.push_back(log_with_time);
}

void analyzerModel::export_csv(const std::string& path)
{
	// 1. 저장할 데이터가 있는지 먼저 확인
	if (this->csv_history.empty()) {
		this->errCode = MODL_ERR_EXPORT_NO_DATA;
		return;
	}

	try {
		std::filesystem::path p(path);

		// 2. 경로 유효성 검사 (부모 디렉토리가 존재하는지 확인)
		if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
			this->errCode = MODL_ERR_INVALID_PATH;
			return;
		}

		// 3. 파일 열기 (인자로 받은 path를 직접 사용)
		std::ofstream file(path);

		if (!file.is_open()) {
			this->errCode = MODL_ERR_CANT_OPEN_FILE;
			return;
		}

		// 4. 데이터 쓰기
		file << "(Timestamp), RawData\n";
		for (const auto& line : this->csv_history) 
		{
			file << line << "\n";

			// 쓰기 도중 에러 발생 여부 체크 (용량 부족 등)
			if (file.fail()) {
				this->errCode = MODL_ERR_DISK_NOT_ENOUGH;
				file.close();
				return;
			}
		}

		file.close();

		// 5. 성공 로그 출력 (파일명만 추출하여 출력)
		std::string successMsg = "CSV Exported to: " + p.filename().string();
		this->add_log("SYS", successMsg.c_str());
	}
	catch (const std::exception& e) {
		// OS 수준의 예외 상황 처리
		std::string msg = "Export Error: ";
		msg += e.what();
		this->add_log("ERR", msg.c_str());
	}
}

// ... (이하 로그, Tx버퍼, 상태, 가짜데이터 로직은 이전과 100% 동일) ...

void analyzerModel::add_log(const char* prefix, const char* msg) 
{
	std::string log_str = std::string("[") + prefix + "]: " + msg;
	this->logs.push_back(log_str);

	// Tx 및 시스템 log도 전부 저장
	this->add_log_with_time(this->get_elapsedTime(), log_str);
}

std::vector<std::string>& analyzerModel::get_logs(void) {
	return this->logs;
}

char* analyzerModel::get_txBuffer(void) {
	return this->tx_buffer;
}

void analyzerModel::clear_txBuffer(void) {
	memset(this->tx_buffer, 0, ANL_TX_BUFF_SIZE);
}

void analyzerModel::add_elapsedTime(float dt) {
	this->elapsed_time += dt;
}

float analyzerModel::get_elapsedTime(void) {
	return this->elapsed_time;
}

int* analyzerModel::get_xAxisRange_ptr(void) {
	return &this->x_axis_range;
}

MODEL_errCode analyzerModel::get_errCode(void) {
	return this->errCode;
}

std::string analyzerModel::update_fakeData(float dt)
{
	this->elapsed_time += dt;

	// 디버그 모드일 때는 가짜 문자열을 파서에 밀어넣어서 테스트해 볼 수 있습니다.
	float fake_val1 = sinf(this->elapsed_time) * 10.0f + 50.0f;
	float fake_val2 = cosf(this->elapsed_time) * 10.0f + 50.0f;
	char fake_str[128];
	std::string retStr;
	sprintf_s(fake_str, "Motor_Speed>%.2f\r\nPhase_Current>%.2f\r\n", fake_val1, fake_val2);
	
	retStr.assign(fake_str);

	return retStr;
}