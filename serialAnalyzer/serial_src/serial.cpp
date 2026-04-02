#include <Windows.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include "serial.h"

#define SERIAL_PORTNAME_LEN     (30U)

analyzerSerial::analyzerSerial()
    : hComm(INVALID_HANDLE_VALUE), isOpened(false), exitThread(false)
{
    this->baudrate = 0;
    this->databit = 0;
    this->stopbit = 0;
    this->paritybit = SERIAL_PARITY_BIT_NONE;
    this->errCode = SERIAL_ERR_NONE;
    this->isOpened = false;
    memset(this->portName, 0, SERIAL_PORTNAME_LEN);
    memset(this->thrd_Buffer, 0, SERIAL_THRD_BUFF_SIZE);
}

analyzerSerial::~analyzerSerial()
{
    close();
}

/*---- 1. 포트 목록 가져오기(Registry 기반) ----*/ 
std::vector<std::string> analyzerSerial::get_ports(void)
{
    std::vector<std::string> portList;
    char valueName[MAX_PATH];
    char portName[MAX_PATH];
    DWORD nValueName, nPortName, nType;
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        for (DWORD i = 0; ; ++i)
        {
            nValueName = MAX_PATH; nPortName = MAX_PATH;
            if (RegEnumValueA(hKey, i, valueName, &nValueName, NULL, &nType, (LPBYTE)portName, &nPortName) == ERROR_SUCCESS)
                portList.push_back(std::string(portName));
            else break;
        }
        RegCloseKey(hKey);
    }
    // COM1, COM2 순서로 정렬
    std::sort(portList.begin(), portList.end());
    return portList;
}

static bool check_open_paras(unsigned int baud, unsigned int data, unsigned int stop, unsigned int parity)
{
    // 보드레이트 검사 (일반적인 범위)
    if (baud < 110 || baud > 921600) return false;

    // 데이터 비트 검사 (일반적으로 5~8)
    if (data < 5 || data > 8) return false;

    // 스탑 비트 검사 (1 또는 2)
    if (stop != 1 && stop != 2) return false;

    // 패리티 비트 검사 (0:None, 1:Odd, 2:Even, 3:Mark, 4:Space)
    // Windows API 상 NOPARITY는 0, ODDPARITY는 1, EVENPARITY는 2입니다.
    if (parity > SERIAL_PARITY_BIT_ODD) return false;

    return true;
}

/*---- 2. 시리얼 포트 열기 및 설정 ----*/
bool analyzerSerial::open(const char* portName, unsigned int baud, unsigned int data,
    unsigned int stop, unsigned int parity)
{
    DCB comm_configs = { 0 };
    std::string fullPath = "";
    COMMTIMEOUTS timeouts = { 0 };
    auto availablePorts = get_ports();
    bool exists = false;
    /*-----------------------------*/
    // 1. 파라미터 유효성 검사
    /*-----------------------------*/
    if (check_open_paras(baud, data, stop, parity) == false)
    {
        this->errCode = SERIAL_ERR_INIT_INVALID_SPEC;
        return false;
    }

    /*-----------------------------*/
    // 2. 포트 존재 여부 사전 체크 (먹통 방지 핵심)
    /*-----------------------------*/
    for (const auto& availPort : availablePorts) 
    {
        if (availPort == portName) 
        { 
            exists = true; 
            break; 
        }
    }

    if (exists == false) 
    {
        this->errCode = SERIAL_ERR_INIT_CANT_OPEN; // 목록에 없으면 즉시 에러 리턴
        return false;
    }

	if (this->isOpened == true) // 이미 열려 있는 상태라면 중복 실행 방지
        this->close();

    /*-----------------------------*/
    // 4. 핸들 생성
    /*-----------------------------*/
    fullPath = "\\\\.\\";
    fullPath += portName;

    // CreateFileA는 포트가 다른 프로그램(테라텀 등)에서 사용 중이면 한참을 기다릴 수 있습니다.
    this->hComm = CreateFileA(fullPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (this->hComm == INVALID_HANDLE_VALUE) {
        this->errCode = SERIAL_ERR_INIT_CANT_OPEN;
        return false;
    }

    /*-----------------------------*/
    // 5. 핸들 생성
    /*-----------------------------*/
    comm_configs.DCBlength = sizeof(DCB);
    if (GetCommState(this->hComm, &comm_configs) == false) 
    { 
        this->close(); 
        return false; 
    }

    comm_configs.BaudRate = baud;
    comm_configs.ByteSize = (BYTE)data;
    comm_configs.StopBits = (stop == 1) ? ONESTOPBIT : TWOSTOPBITS;
    comm_configs.Parity = (BYTE)parity;
    comm_configs.fBinary = TRUE;
    comm_configs.fParity = (parity != NOPARITY);

    if (SetCommState(hComm, &comm_configs) == false) {
        this->errCode = SERIAL_ERR_RUN_COMM_FAIL;
        this->close();
        return false;
    }

    /*-----------------------------*/
    // 5. 타임아웃 설정 -> 논블로킹
    /*-----------------------------*/
    timeouts.ReadIntervalTimeout = MAXDWORD; // 타임아웃 설정을 0으로 잡으면 데이터가 없어도 즉시 리턴 -> 논블로킹 설정
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 500;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(hComm, &timeouts);

    this->exitThread = false;
    this->isOpened = true;
    this->rxThread = std::thread(&analyzerSerial::rxWorker, this);

    this->errCode = SERIAL_ERR_NONE; // 성공 시 에러 초기화
    return true;
}

/*---- 3. 시리얼 포트 닫기 및 자원해제 ----*/
void analyzerSerial::close()
{
    // 이미 닫혀 있는 상태라면 중복 실행 방지
    if (this->isOpened == false && this->hComm == INVALID_HANDLE_VALUE) return;

    // 1. 스레드 종료 신호 및 상태 변경
    memset(this->portName, 0, SERIAL_PORTNAME_LEN);
    this->baudrate = 0;
    this->databit = 0;
    this->stopbit = 0;
    this->paritybit = SERIAL_PARITY_BIT_NONE;
    this->isOpened = false;
    this->exitThread = true;

    // 2. 수신 스레드 종료 대기
    if (this->rxThread.joinable())
    {
        this->rxThread.join();
    }

    // 3. Windows Serial Handle 닫기
    if (this->hComm != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->hComm);
        this->hComm = INVALID_HANDLE_VALUE;
    }

    // 4. 버퍼 비우기 (Mutex 사용)
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        this->rxBuffer.clear();
    }
}

SERIAL_ErrCode analyzerSerial::get_errCode(void)
{
    SERIAL_ErrCode temp = this->errCode;
    this->errCode = SERIAL_ERR_NONE;
    return temp;
}

bool analyzerSerial::is_opened(void)
{
    return this->isOpened;
}

/*---- 4. 동기식 쓰기 수행 ----*/
void analyzerSerial::write(const std::string& data)
{
    if (!this->isOpened || this->hComm == INVALID_HANDLE_VALUE) return;

    DWORD bytesWritten = 0;
    // 동기식 쓰기 수행
    if (WriteFile(this->hComm, data.c_str(), (DWORD)data.length(), &bytesWritten, NULL) == false)
    {
        this->errCode = SERIAL_ERR_RUN_COMM_FAIL;
    }
}

/*---- 5. 동기식 쓰기 수행 ----*/
std::string analyzerSerial::readPendings()
{
    // 1. 스레드간 경합 방지를 위해 Lock
    std::lock_guard<std::mutex> lock(this->mtx);
    std::string temp;

    if (this->rxBuffer.empty()) 
        return "";

    // 2. 현재까지 쌓인 문자열을 복사한 뒤 원본은 비움
    temp = this->rxBuffer;
    this->rxBuffer.clear();

    return temp;
}

/*---- 6. 수신 스레드 ----*/
void analyzerSerial::rxWorker()
{
    DWORD bytesRead = 0;
    long long elapsedTime = 0;
    this->last_rx_time = std::chrono::steady_clock::now();

    while (this->exitThread == false)
    {
        this->current_time = std::chrono::steady_clock::now();

        // 1. 버퍼로 읽기 시도 (Non-blocking 설정 덕분에 데이터가 없으면 즉시 반환됨)
        if (ReadFile(this->hComm, this->thrd_Buffer, SERIAL_THRD_BUFF_SIZE - 1, &bytesRead, NULL))
        {
            if (bytesRead > 0) // 2. 가져온 데이터가 있으면 공용 버퍼에 추가 (Lock 필수)
            {
                std::lock_guard<std::mutex> lock(this->mtx);
                this->rxBuffer.append(this->thrd_Buffer, bytesRead);
                memset(this->thrd_Buffer, 0, SERIAL_THRD_BUFF_SIZE);

				// 수신 성공 시 마지막 수신 시간 업데이트
                this->last_rx_time = std::chrono::steady_clock::now();
            }
			else // 2.1 데이터가 없으면 타임아웃 검사
            {
                elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>
                    (this->current_time - this->last_rx_time).count();

				// 3. 읽기 실패 감지 -> 타임아웃 발생 여부 판단
                if (elapsedTime > SERIAL_RX_TIMEOUT_MS)
                {
                    this->errCode = SERIAL_ERR_RUN_RX_TIMEOUT;

                    // [중요] 타임아웃 에러를 발생시킨 후 타이머를 다시 리셋 -> 이렇게 안 하면 1ms마다 무한정 에러 폭발
                    this->last_rx_time = this->current_time;
                }
            }
        }
        else // 2.1 케이블이 뽑히거나 OS 단에서 포트가 날아감
        {
            this->errCode = SERIAL_ERR_RUN_RX_FAIL;
        }

        // 4. CPU 과부하 방지를 위한 1ms 대기 (임베디드 통신 속도 고려)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}