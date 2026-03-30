#pragma once

#include "implot.h"

#define ANL_DEBUG_MODE				(0U)
#define ANL_SERI_MODE				(1U)
#define ANL_RUN_MODE				(ANL_SERI_MODE)

#define ANL_TX_BUFF_SIZE			(256U)
#define ANL_RX_TIMEOUT_SEC			(10U)

#define ANL_SCROLLING_BUFF_SIZE		(2000)

#define ImVec4_COLOR_RED			(ImVec4(1.0f, 0.4f, 0.4f, 1.0f))
#define ImVec4_COLOR_BLUE			(ImVec4(0.2f, 0.4f, 0.6f, 1.0f))
#define ImVec4_COLOR_GRAY			(ImVec4(0.4f, 0.4f, 0.4f, 1.0f))
#define ImVec4_COLOR_NAVY			(ImVec4(0.1f, 0.1f, 0.12f, 1.0f))
#define ImVec4_COLOR_YELLOW			(ImVec4(1, 1, 0, 1))
#define ImVec4_COLOR_CYAN			(ImVec4(0, 0.8f, 0.8f, 1.f))
#define ImVec4_COLOR_GREEN			(ImVec4(0, 0.8f, 0.3f, 1.f))

#define MAIN_WND_BACK_CLR_RGB		(ImVec4_COLOR_NAVY)

#define ANL_CSV_DEFAULT_PATH		("./log_data.csv")

extern void MainLayout(void);