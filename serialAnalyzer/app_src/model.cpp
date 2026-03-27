#include <vector>
#include <string>
#include <map>
#include "imgui.h"
#include "config.h"
#include "model.h"

analyzerModel::analyzerModel()
{
	this->elapsed_time = 0.0f;
	this->x_axis_range = 5;
	this->targetDomain = "";
	this->rx_remainder = "";
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
#if(ANL_RUN_MODE == ANL_SERI_MODE)
	domainSpace.reserve(this->multiData.size());

	for (const auto& pair : this->multiData)
	{
		domainSpace.push_back(pair.first); // 맵의 Key(도메인명) 추출
	}
#else
	domainSpace.push_back("tempDomain01");
	domainSpace.push_back("tempDomain02");
	domainSpace.push_back("tempDomain03");
#endif
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

// ... (이하 로그, Tx버퍼, 상태, 가짜데이터 로직은 이전과 100% 동일) ...

void analyzerModel::add_log(const char* prefix, const char* msg) 
{
	std::string log_str = std::string("[") + prefix + "]: " + msg;
	this->logs.push_back(log_str);
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

void analyzerModel::update_fakeData(float dt)
{
	this->elapsed_time += dt;

#if (ANL_RUN_MODE == ANL_DEBUG_MODE)
	// 디버그 모드일 때는 가짜 문자열을 파서에 밀어넣어서 테스트해 볼 수 있습니다.
	float fake_val1 = sinf(this->elapsed_time) * 10.0f + 50.0f;
	float fake_val2 = cosf(this->elapsed_time) * 10.0f + 50.0f;
	char fake_str[128];
	sprintf_s(fake_str, "Motor_Speed>%.2f\r\nPhase_Current>%.2f\r\n", fake_val1, fake_val2);
	this->parse_teleplot_data(fake_str);
#endif
}