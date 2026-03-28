#pragma once

#include <vector>
#include <string>
#include <map>
#include "imgui.h"
#include "config.h"

enum MODEL_errCode
{
	MODL_ERR_NONE,
	MODL_ERR_EXPORT_NO_DATA,
	MODL_ERR_INVALID_PATH,
	MODL_ERR_CANT_OPEN_FILE,
	MODL_ERR_DISK_NOT_ENOUGH
};

// 1. 실시간 그래프 데이터를 담을 링 버퍼 구조체 (생성자 추가)
typedef struct ScrollingBuffer
{
	int MaxSize;
	int cursor;
	std::vector<ImVec2> data;

	// std::map에서 새 항목이 생성될 때 자동으로 호출됨
	ScrollingBuffer()
	{
		MaxSize = ANL_SCROLLING_BUFF_SIZE;
		cursor = 0;
		data.reserve(MaxSize);
	}
} ScrollingBuffer;

class analyzerModel
{
public:
	analyzerModel();
	~analyzerModel();

	// --- 그래프 데이터 제어 (다중 도메인 적용) ---
	void add_graphData(const std::string& domain, float x, float y);
	void clear_graphData(void);

	// --- 도메인 관리 및 파싱 ---
	void parse_teleplot_data(const std::string& raw_data);
	void get_domain_names(std::vector<std::string>& domainSpace);

	void set_targetDomain(const std::string& domain);
	std::string get_targetDomain(void);
	ScrollingBuffer* get_targetBuffer(void); // 현재 그릴 도메인 버퍼 반환

	// --- 로그 데이터 제어 ---
	void add_log(const char* prefix, const char* msg);
	std::vector<std::string>& get_logs(void);
	void add_log_with_time(float elapsed_time, const std::string& log);
	void export_csv(const std::string& path);

	// --- Tx 버퍼 제어 (ImGui InputText와 연결) ---
	char* get_txBuffer(void);
	void clear_txBuffer(void);

	// --- 시스템 상태(시간, 설정) 제어 ---
	void add_elapsedTime(float dt);
	float get_elapsedTime(void);
	int* get_xAxisRange_ptr(void); // SliderInt 연결용 포인터 반환
	MODEL_errCode get_errCode(void);

	// --- 테스트용 가짜 데이터 생성기 ---
	std::string update_fakeData(float dt);

private:
	// 실제 데이터 보관소
	std::map<std::string, ScrollingBuffer> multiData;	// 도메인별 분리 저장소
	std::vector<std::string> csv_history;				// csv 파일 기록용 로그 저장소
	std::string targetDomain;							// 현재 선택된 도메인
	std::string rx_remainder;							// 파싱 중 잘린 데이터 보관용
	ScrollingBuffer* cached_TgtBuff;					// 포인터 캐싱

	std::vector<std::string> logs;
	char tx_buffer[ANL_TX_BUFF_SIZE];

	// 상태 변수
	MODEL_errCode errCode;
	float elapsed_time;
	int x_axis_range;
};