#pragma once

#include "implot.h"

#define ANL_DEBUG_MODE				(0U)
#define ANL_SERI_MODE				(1U)
#define ANL_RUN_MODE				(ANL_DEBUG_MODE)

#define ANL_TX_BUFF_SIZE			(256)

#define ANL_XAXIS_RANGE_SEC			(5)
#define ANL_TGT_MSG_PERIOD_MS		(100)

#define ANL_SCROLLING_BUFF_SIZE		(2000)

#define DEFAULT_WND_BACK_CLR_RGB	(ImVec4(0.1f, 0.1f, 0.12f, 1.0f))
#define MAIN_WND_BACK_CLR_RGB		(DEFAULT_WND_BACK_CLR_RGB)

extern void MainLayout(void);