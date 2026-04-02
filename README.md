# 시리얼 분석기

## Introduction

기존의 테라텀 및 TelePlot은 실시간으로 데이터를 보내고, 시각화 하는 동작을 동시에 할 수 없었습니다.

- 테라텀 : 실시간으로 데이터를 보낼 수 있음 → 시각화 불가능
- TelePlot : 실시간으로 데이터를 시각화 할 수 있음 → 전송 불가능

STM32 등 MCU에서 발생하는 실시간 데이터를 한꺼번에 시각화하고 제어 명령을 전송하기 위한 전용 GUI 툴을 개발하였습니다.

본 시리얼 분석기는 복잡한 설정 없이, 약속된 데이터 규격만 맞춰주면 즉시 실시간 모니터링 및 동시 제어가 가능합니다.

### Dear Imgui

url : https://github.com/ocornut/imgui

## Environment

| **분류** | **내용** | **비고** |
| :--- | :--- | :--- |
| **OS** | Windows 7 이상 | Windows 10/11 권장 |
| **Architecture** | x64 | |
| **GPU** | DirectX 11을 지원하는 그래픽 카드 | 하드웨어 가속 렌더링용 |
| **I/O** | 사용 가능한 COM Port | 타겟 보드(MCU) 또는 VSPE 등 가상 시리얼 포트 |
| **IDE** | Visual Studio 2022 (v17.14.29) | |
| **Compiler** | MSVC (ISO C++17 Standard) | |
| **SDK** | Windows 10 SDK 이상 | Win32 API 및 DX11 빌드 요구사항 |
| **Dependencies**| Dear ImGui, ImPlot | 소스 트리에 포함됨 |

MCU외에 VSPE로도 테스트 가능 (https://eterlogic.com/products.vspe.html)

## Layout

<img width="722" height="512" alt="스크린샷 2026-03-30 180545" src="https://github.com/user-attachments/assets/24842214-01f0-4e91-8579-04561656213c" />

| **기능 이름(color)** | **내용** |
| :--- | :--- |
| <span style="color:#FF6B6B">텍스트 출력 창(빨강)</span> | • 시스템 로그 및 Rx/Tx 내역, 에러 내역 등이 출력 되는 출력 창입니다<br>• 상단의 Auto-Scroll 을 체크하면, 출력 창이 가장 최신 로그로 업데이트 됩니다. |
| <span style="color:#FFA500">Tx 전송 창(주황)</span> | • 이곳에 입력 후, 엔터를 입력하면 타겟으로 전송됩니다. |
| <span style="color:#FFA500">로그 히스토리 출력 창(노랑)</span> | • 소스 코드 위치를 기준으로 경로와 파일명을 선택할 수 있습니다.<br>• 지금까지 수집한 로그를 .csv 파일 형태로 출력합니다. |
| <span style="color:#8FBC8F">그래프 출력 창(초록)</span> | • 도메인 창에서 감지된 도메인을 클릭하게 되면, 해당 도메인의 수신 값을 실시간으로 그래프로 출력합니다 |
| <span style="color:#87CEFA">그래프 x축 슬라이더(파랑)</span> | • 선택한 도메인의 수신 값을 그래프에 사용자가 선택한 범위(1 ~ 10초) 로 출력합니다. |
| <span style="color:#9370DB">도메인 창(보라) </span> | • 수신 받은 메시지에서 프로토콜 규격을 지킨 전혀 다른 도메인들을 차례대로 출력합니다. |
| <span style="color:#FF69B4">초기화 버튼(핑크) </span> | • 텍스트 출력 창, 감지 도메인 내역, 각 도메인 별 수신 데이터를 모두 없앱니다. |
| <span style="color:#A9A9A9">통신 스펙 설정 창(회색)</span> | • 현재 윈도우에서 감지한 COM 포트 내역 선택 창과 UART 설정 창이 있습니다.<br>• 하단의 버튼은 설정한 대로 스펙을 적용해 통신합니다. |
<br>

# 사용법
## Setup

- 장치 연결
    
    분석하고자 하는 MCU(Arduino, STM32, ESP32 등)를 PC의 USB 포트에 연결합니다.
    
- 포트 설정
    - `Port` 드롭다운에서 연결된 COM PORT를 선택합니다
    - `Baudrate`, `StopBit`, `ParityBit`, `DataBit`를 입력하고
    - `Connect` 버튼을 클릭합니다.
- **연결 확인:** 상단 배너에 에러가 없고, 텍스트 창에 데이터가 들어오기 시작하면 준비 완료입니다.

## Protocol

그래프를 그리려면 MCU에서 아래와 같은 **Teleplot 형식**으로 문자열을 출력해야 합니다. 본 프레임워크는 이 형식을 실시간으로 파싱하여 도메인별로 자동 분류합니다.

- 형식 : `도메인이름>값\n`
- 예시
    
    Virtual Port를 활용한 파이썬 코드 중.
    
    ```python
    msg += f"Temperature>{25.0 + 5.0 * math.sin(time_sec / 2.0):.2f}\r\n"
    ser.write(msg.encode('utf-8'))
    ```
    

## Main functions

### ① 실시간 그래프 모니터링 

- **도메인 선택:** `Detected Domains` 리스트에 MCU가 보낸 이름들이 자동으로 나열됩니다. 확인하고 싶은 항목을 클릭하면 즉시 그래프가 전환됩니다.
- **X축 범위 조절:** `XRange` 슬라이더를 통해 과거 몇 초간의 데이터를 볼지 결정할 수 있습니다 (1~10초).

### ② 명령 송신 및 로그 확인

- **명령 전달:** 하단 입력창에 텍스트를 입력하고 `Enter`를 누르면 MCU로 데이터가 전송됩니다. (끝에 `\r\n` 자동 추가)
- **로그 관리:** 상단의 `Auto-Scroll`을 체크하면 실시간 로그를 추적하며, 필요 시 `Reset All Data`로 화면을 비울 수 있습니다.

### ③ 데이터 내보내기 

- **CSV 저장:** `.CSV Export Path`에 저장 경로를 지정하고 `Export` 버튼을 누르면, 프로그램 실행 시점부터 지금까지의 모든 수신 기록이 타임스탬프와 함께 파일로 저장됩니다.
- **사후 분석:** 저장된 CSV는 엑셀(Excel)이나 MATLAB에서 즉시 활용 가능합니다.

## Errors

툴의 System 단에서 감지할 수 있는 에러들은 아래와 같습니다.

|**에러코드**|**하위 객체**|**원인**|
|:--|:--|:--|
|`G_ERR_NONE`|-|-|
|`G_ERR_INIT_INVALID_SPEC`|<span style="color:yellow">control</span>|올바르지 않은 메인창의 크기를 입력함|
|`G_ERR_INIT_DX11_ON`|<span style="color:yellow">control</span>|DirectX11가 활성화되어 있어 초기화할 구성이나 모드와 충돌|
|`G_ERR_INIT_HANDSHAKE_WIN_DX`|<span style="color:yellow">control</span>|윈도우(또는 핸드셰이크) 모드와 DirectX 설정 간 충돌으로 초기화 실패|
|`G_ERR_MAIN_WINDOW_INVALID_DATA`|<span style="color:yellow">view</span>|메인 윈도우에 전달된 데이터가 null 또는 형식/범위가 잘못되어 창 초기화/렌더링 불가|
|`G_ERR_MAIN_WINDOW_NO_CALLBACK`|<span style="color:yellow">view</span>|메인 윈도우에 필요한 콜백(이벤트 핸들러)이 등록되지 않음|
|`G_ERR_CHILD_WINDOW_NO_CALLBACK`|<span style="color:yellow">view</span>|메인 윈도우에 필요한 콜백(이벤트 핸들러)이 등록되지 않음|
|`G_ERR_CHILD_WINDOW_INVALID_DATA`|<span style="color:yellow">view</span>|자식 창에 전달된 파라미터가 잘못되어 생성 또는 동작에 실패|
|`G_ERR_SERIAL_INIT_INVALID_SPEC`|<span style="color:green">serial</span>|시리얼 포트 초기화 파라미터(baud, data, stop, parity 등)가 유효하지 않음|
|`G_ERR_SERIAL_INIT_CANT_OPEN`|<span style="color:green">serial</span>|지정한 COM 포트를 열 수 없음(존재하지 않음, 권한 문제, 다른 프로세스가 점유)|
|`G_ERR_SERIAL_RUN_COMM_FAIL`|<span style="color:green">serial</span>|통신 동작(읽기/쓰기/설정) 중 API 실패|
|`G_ERR_SERIAL_RUN_RX_FAIL`|<span style="color:green">serial</span>|수신 루틴에서 읽기 실패 (타겟과의 물리적 단선 이슈).|
|`G_ERR_SERIAL_RUN_RX_TIMEOUT`|<span style="color:green">serial</span>|수신 대기 중 타임아웃 발생(지정된 시간 내 데이터 수신 없음)|
|`G_ERR_MODEL_EXPORT_NO_DATA`|<span style="color:yellow">model</span>|내보낼(저장할) 데이터가 없음(빈 데이터셋)|
|`G_ERR_MODEL_INVALID_PATH`|<span style="color:yellow">model</span>|파일 경로가 잘못되었거나 허용되지 않는 형식(잘못된 문자, 길이 초과 등)|
|`G_ERR_MODEL_CANT_OPEN_FILE`|<span style="color:yellow">model</span>|모델 파일을 열 수 없음(권한 부족, 파일 잠금, 파일 없음)|
|`G_ERR_MODEL_DISK_NOT_ENOUGH`|<span style="color:yellow">model</span>|디스크 여유 공간 부족으로 파일 쓰기/내보내기 실패|
