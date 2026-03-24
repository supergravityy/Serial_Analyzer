#pragma once

#include <vector>
#include <string>
#include <math.h>
#include "config.h"
#include "imgui.h"

// 1. 실시간 그래프 데이터를 담을 링 버퍼 구조체
typedef struct ScrollingBuffer 
{
	int MaxSize;
	int cursor;
	std::vector<ImVec2> data; // C++ vector는 메모리 관리상 유지합니다.
}ScrollingBuffer;

class analyzerModel
{
public:
	analyzerModel();
	~analyzerModel();

	// --- 그래프 데이터 제어 ---
	void add_graphData(float x, float y); // (기존 AddPoint 역할)
	void clear_graphData(void);           // (기존 Erase 역할)
	ScrollingBuffer& get_graphData(void);

	// --- 로그 데이터 제어 ---
	void add_log(const char* prefix, const char* msg);
	std::vector<std::string>& get_logs(void);

	// --- Tx 버퍼 제어 (ImGui InputText와 연결) ---
	char* get_txBuffer(void);
	void clear_txBuffer(void);

	// --- 시스템 상태(시간, 설정) 제어 ---
	void add_elapsedTime(float dt);
	float get_elapsedTime(void);
	int* get_xAxisRange_ptr(void); // SliderInt 연결용 포인터 반환

	// --- 테스트용 가짜 데이터 생성기 ---
	void update_data(float dt);
private:
	// 실제 데이터 보관소
	ScrollingBuffer sdata;
	std::vector<std::string> logs;
	char tx_buffer[ANL_TX_BUFF_SIZE];

	// 상태 변수
	float elapsed_time;
	int x_axis_range;
};

