#include <vector>
#include <string>
#include "imgui.h"
#include "config.h"
#include "model.h"

analyzerModel::analyzerModel()
{
	this->sdata.MaxSize = ANL_SCROLLING_BUFF_SIZE;
	this->sdata.cursor = 0;
	this->sdata.data.reserve(this->sdata.MaxSize); // 메모리풀처럼 미리 큰 영역 할당

	this->elapsed_time = 0.0f;
	this->x_axis_range = 5;
	memset(this->tx_buffer, 0, ANL_TX_BUFF_SIZE);
}

analyzerModel::~analyzerModel()
{
	this->logs.clear();
	this->clear_graphData(); // 기존 Erase() 호출 대체
}

// --- 그래프 데이터 제어 (링 버퍼 로직) ---
void analyzerModel::add_graphData(float x, float y)
{
	// 버퍼가 다 안 찼을 때는 그냥 뒤에 밀어 넣음
	if (this->sdata.data.size() < this->sdata.MaxSize)
	{
		this->sdata.data.push_back(ImVec2(x, y));
	}
	// 버퍼가 꽉 찼을 때는 Offset 위치(가장 오래된 데이터)를 덮어씌움
	else
	{
		this->sdata.data[this->sdata.cursor] = ImVec2(x, y);
		this->sdata.cursor = (this->sdata.cursor + 1) % this->sdata.MaxSize;
	}
}

void analyzerModel::clear_graphData(void)
{
	if (this->sdata.data.size() > 0)
	{
		this->sdata.data.clear();
		this->sdata.cursor = 0;
	}
}

ScrollingBuffer& analyzerModel::get_graphData(void)
{
	return this->sdata;
}

// ... (이하 로그, Tx버퍼, 상태, 가짜데이터 로직은 이전과 100% 동일) ...

void analyzerModel::add_log(const char* prefix, const char* msg) {
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

void analyzerModel::update_data(float dt) 
{
#if (ANL_RUN_MODE == ANL_DEBUG_MODE)
	this->elapsed_time += dt;
	float fake_val = sinf(this->elapsed_time) * 10.0f + 50.0f;
	this->add_graphData(this->elapsed_time, fake_val);
#else
#endif
}