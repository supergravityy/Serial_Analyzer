#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include "config.h"

#define SERIAL_PARITY_BIT_NONE		(0U)
#define SERIAL_PARITY_BIT_ODD		(1U)
#define SERIAL_PARITY_BIT_EVEN		(2U)
#define SERIAL_PARITY_BIT_MODE		(SERIAL_PARITY_BIT_NONE)
#define SERIAL_PORTNAME_LEN			(30U)
#define SERIAL_THRD_BUFF_SIZE		(1024U)
#define SERIAL_RX_TIMEOUT_MS		(ANL_RX_TIMEOUT_SEC * 1000U)

enum SERIAL_ErrCode
{
	SERIAL_ERR_NONE,
	SERIAL_ERR_INIT_INVALID_SPEC,
	SERIAL_ERR_INIT_CANT_OPEN,
	SERIAL_ERR_RUN_COMM_FAIL,
	SERIAL_ERR_RUN_RX_FAIL,
	SERIAL_ERR_RUN_RX_TIMEOUT
};

class analyzerSerial
{
public:
	analyzerSerial();
	~analyzerSerial();
	
	// 포트 목록을 가져오는 기능은 객체가 생성되기 전(연결 전)에도 
	// UI에서 호출해야 하므로 static으로 선언하는 것이 구조적으로 옳습니다.
	static std::vector<std::string> get_ports();

	bool open(const char* portName, unsigned int baud, unsigned int data, 
		unsigned int stop, unsigned int parity);
	void close();

	void write(const std::string& data);
	std::string readPendings(); // 수신된 데이터를 파서에게 넘겨줌

	bool is_opened();
	SERIAL_ErrCode get_errCode();

private:
	void rxWorker(); // 내부 수신 스레드 함수

	char portName[SERIAL_PORTNAME_LEN];
	char thrd_Buffer[SERIAL_THRD_BUFF_SIZE];
	unsigned int baudrate;
	unsigned int stopbit;
	unsigned int databit; 
	unsigned int paritybit;
	SERIAL_ErrCode errCode;
	std::chrono::steady_clock::time_point last_rx_time;
	std::chrono::steady_clock::time_point current_time;

	HANDLE hComm;        // Windows Serial Handle
	bool isOpened;
	
	std::thread rxThread;
	std::mutex mtx;           // 수신 버퍼 접근 보호 (Race Condition 방지)
	std::string rxBuffer;     // 임시 수신 보관소
	bool exitThread;          // 스레드 종료 플래그
};