/****************************************************************
*               ModernEngine.cpp - Implementation                 *
*                                                                 *
*   Global instances and initialization for modern engine layer  *
*****************************************************************/

#include "ModernEngine.h"

// Global instances of timing systems
CHighPrecisionTimer g_HighPrecisionTimer;
CFPSLimiter g_FPSLimiter(DEF_TARGET_FPS);
CLogicAccumulator g_LogicAccumulator(DEF_LOGIC_UPDATE_RATE);
