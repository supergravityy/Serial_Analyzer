import serial
import time
import math
import threading

# VSPE로 만든 가상 포트 (Analyzer가 COM17이라면 반대편 포트를 적으세요)
PORT_NAME = 'COM17' 
BAUDRATE = 115200
TIME_UNIT = 0.05 # 50ms

try:
    ser = serial.Serial(PORT_NAME, BAUDRATE, timeout=0.1)
except Exception as e:
    print(f"포트 열기 실패: {e}")
    exit()

print(f"[{PORT_NAME}] 가상 MCU 가동 중... (Ctrl+C로 종료)")

# --- [추가] PC에서 오는 명령(TX)을 수신하는 백그라운드 스레드 ---
def receive_from_pc():
    while ser.is_open:
        try:
            if ser.in_waiting > 0:
                # 데이터를 읽고 문자열로 디코딩
                rx_data = ser.readline().decode('utf-8', errors='ignore').strip()
                if rx_data:
                    print(f"\n[PC 명령 수신] : {rx_data}")
        except:
            break

# 수신 스레드 시작
rx_thread = threading.Thread(target=receive_from_pc, daemon=True)
rx_thread.start()

# --- 기존 데이터 송신(RX) 루프 ---
time_sec = 0.0
try:
    while True:
        motor_speed = 1500.0 + 500.0 * math.sin(time_sec)
        phase_current = 5.0 + 2.0 * math.cos(time_sec * 2.0)
        
        msg = f"Motor_Speed>{motor_speed:.2f}\r\nPhase_Current>{phase_current:.2f}\r\n"
        msg += f"Temperature>{25.0 + 5.0 * math.sin(time_sec / 2.0):.2f}\r\n"
        msg += "@#$!@#$>\r\n" # 쓰래기 데이터 전송
        ser.write(msg.encode('utf-8'))
        
        time_sec += TIME_UNIT
        time.sleep(TIME_UNIT) # 50ms 대기

except KeyboardInterrupt:
    print("\n통신 종료 중...")
    ser.close()
    print("종료 완료.")