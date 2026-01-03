/****************************************************************
*               ModernEngine.h - Modernization Layer             *
*                                                                 *
*   High-precision timing, FPS limiter, and CPU optimization     *
*   for modern Windows 10/11 systems with high-refresh monitors  *
*                                                                 *
*   This layer preserves the original game feel while fixing     *
*   performance issues on modern hardware.                        *
*****************************************************************/

#ifndef MODERN_ENGINE_H
#define MODERN_ENGINE_H

#pragma once

#include <windows.h>
#include <mmsystem.h>

/****************************************************************
*   CONFIGURATION OPTIONS                                        *
*****************************************************************/

// Target frame rate for the game
// Set to 0 for auto-detect (uses monitor refresh rate)
// Set to -1 to disable FPS limiter completely (not recommended)
#define DEF_TARGET_FPS              0

// Enable high-precision timing (recommended for 144Hz+ monitors)
#define DEF_USE_HIGH_PRECISION_TIMER  1

// Minimum sleep time in milliseconds when frame is ahead of schedule
// Lower values = smoother but more CPU usage
#define DEF_MIN_SLEEP_TIME_MS       1

// Enable CPU idle optimization (reduces busy-waiting)
#define DEF_CPU_IDLE_OPTIMIZATION   1

// Idle time threshold in microseconds before yielding CPU
#define DEF_IDLE_THRESHOLD_US       2000

// Game logic update rate (independent of visual FPS)
// Original game logic was designed for ~30 updates per second
#define DEF_LOGIC_UPDATE_RATE       30

// Enable borderless fullscreen mode (recommended for modern systems)
#define DEF_BORDERLESS_FULLSCREEN   1

/****************************************************************
*   HIGH-PRECISION TIMER CLASS                                   *
*   Uses QueryPerformanceCounter for microsecond accuracy        *
*****************************************************************/

class CHighPrecisionTimer
{
public:
    CHighPrecisionTimer()
    {
        QueryPerformanceFrequency(&m_liFrequency);
        m_dFrequencyMs = (double)m_liFrequency.QuadPart / 1000.0;
        m_dFrequencyUs = (double)m_liFrequency.QuadPart / 1000000.0;
        
        // Capture timeGetTime() base for compatibility with existing code
        // This ensures GetTicksMs() returns values compatible with timeGetTime()
        m_dwTimeGetTimeBase = timeGetTime();
        QueryPerformanceCounter(&m_liStartTime);
        m_liLastFrameTime = m_liStartTime;
    }

    // Reset the timer (for internal timing only, doesn't affect GetTicksMs compatibility)
    void Reset()
    {
        QueryPerformanceCounter(&m_liStartTime);
        m_liLastFrameTime = m_liStartTime;
        m_dwTimeGetTimeBase = timeGetTime();
    }

    // Get elapsed time in milliseconds since Reset()
    double GetElapsedMs() const
    {
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        return (double)(liNow.QuadPart - m_liStartTime.QuadPart) / m_dFrequencyMs;
    }

    // Get elapsed time in microseconds since Reset()
    double GetElapsedUs() const
    {
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        return (double)(liNow.QuadPart - m_liStartTime.QuadPart) / m_dFrequencyUs;
    }

    // Get time since last frame in milliseconds
    double GetDeltaTimeMs()
    {
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        double dDelta = (double)(liNow.QuadPart - m_liLastFrameTime.QuadPart) / m_dFrequencyMs;
        m_liLastFrameTime = liNow;
        return dDelta;
    }

    // Get current timestamp (COMPATIBLE with timeGetTime() values)
    // Uses timeGetTime() base + high-precision offset for smooth timing
    // This ensures compatibility with all existing code that compares DWORD timestamps
    DWORD GetTicksMs() const
    {
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        DWORD dwOffset = (DWORD)((liNow.QuadPart - m_liStartTime.QuadPart) * 1000 / m_liFrequency.QuadPart);
        return m_dwTimeGetTimeBase + dwOffset;
    }

    // Get raw counter value for precise comparisons
    LONGLONG GetRawCounter() const
    {
        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);
        return liNow.QuadPart;
    }

    // Get frequency for calculations
    LONGLONG GetFrequency() const { return m_liFrequency.QuadPart; }

private:
    LARGE_INTEGER m_liFrequency;
    LARGE_INTEGER m_liStartTime;
    LARGE_INTEGER m_liLastFrameTime;
    double m_dFrequencyMs;
    double m_dFrequencyUs;
    DWORD m_dwTimeGetTimeBase;  // Base timeGetTime() value for compatibility
};

/****************************************************************
*   FPS LIMITER CLASS                                            *
*   Ensures consistent frame timing without busy-waiting         *
*****************************************************************/

class CFPSLimiter
{
public:
    CFPSLimiter(int iTargetFPS = DEF_TARGET_FPS)
    {
        QueryPerformanceFrequency(&m_liFrequency);
        QueryPerformanceCounter(&m_liLastFrameTime);
        
        // Initialize multimedia timer for better Sleep() precision
        TIMECAPS tc;
        if (timeGetDevCaps(&tc, sizeof(tc)) == TIMERR_NOERROR)
        {
            m_uTimerResolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
            timeBeginPeriod(m_uTimerResolution);
            m_bTimerInitialized = true;
        }
        else
        {
            m_uTimerResolution = 1;
            m_bTimerInitialized = false;
        }

        m_dFrameTimeAccumulator = 0.0;
        m_iFPSCounter = 0;
        m_iCurrentFPS = 0;
        QueryPerformanceCounter(&m_liLastFPSUpdate);
        
        // Auto-detect monitor refresh rate if target is 0
        if (iTargetFPS == 0)
        {
            iTargetFPS = AutoDetectRefreshRate();
        }
        SetTargetFPS(iTargetFPS);
    }
    
    // Auto-detect monitor refresh rate
    static int AutoDetectRefreshRate()
    {
        DEVMODE dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        
        if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
        {
            int refreshRate = (int)dm.dmDisplayFrequency;
            // Sanity check: return reasonable values
            if (refreshRate >= 30 && refreshRate <= 360)
            {
                return refreshRate;
            }
        }
        return 60;  // Default fallback
    }

    ~CFPSLimiter()
    {
        if (m_bTimerInitialized)
        {
            timeEndPeriod(m_uTimerResolution);
        }
    }

    // Set target FPS (0 = unlimited)
    void SetTargetFPS(int iTargetFPS)
    {
        m_iTargetFPS = iTargetFPS;
        if (iTargetFPS > 0)
        {
            m_dTargetFrameTimeMs = 1000.0 / (double)iTargetFPS;
            m_llTargetFrameTime = (LONGLONG)((double)m_liFrequency.QuadPart / (double)iTargetFPS);
        }
        else
        {
            m_dTargetFrameTimeMs = 0.0;
            m_llTargetFrameTime = 0;
        }
    }

    // Wait until it's time to render the next frame
    // Returns true if frame should be rendered, false if we should skip
    bool WaitForNextFrame()
    {
        if (m_iTargetFPS <= 0)
            return true;  // No limiting

        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);

        LONGLONG llElapsed = liNow.QuadPart - m_liLastFrameTime.QuadPart;
        LONGLONG llRemaining = m_llTargetFrameTime - llElapsed;

        if (llRemaining > 0)
        {
            // Calculate remaining time in milliseconds
            double dRemainingMs = (double)llRemaining * 1000.0 / (double)m_liFrequency.QuadPart;

            // If we have more than 2ms to wait, use Sleep to reduce CPU usage
            if (dRemainingMs > 2.0)
            {
                // Sleep for most of the remaining time, leaving some margin
                DWORD dwSleepMs = (DWORD)(dRemainingMs - 1.5);
                if (dwSleepMs > 0)
                {
                    Sleep(dwSleepMs);
                }
            }
            else if (dRemainingMs > 0.5)
            {
                // For very short waits, yield to other threads
                Sleep(0);
            }

            // Spin-wait for the remaining time (most accurate)
            do
            {
                QueryPerformanceCounter(&liNow);
                llElapsed = liNow.QuadPart - m_liLastFrameTime.QuadPart;
            } while (llElapsed < m_llTargetFrameTime);
        }

        m_liLastFrameTime = liNow;

        // Update FPS counter
        m_iFPSCounter++;
        LONGLONG llFPSElapsed = liNow.QuadPart - m_liLastFPSUpdate.QuadPart;
        if (llFPSElapsed >= m_liFrequency.QuadPart)  // 1 second passed
        {
            m_iCurrentFPS = m_iFPSCounter;
            m_iFPSCounter = 0;
            m_liLastFPSUpdate = liNow;
        }

        return true;
    }

    // Get current measured FPS
    int GetCurrentFPS() const { return m_iCurrentFPS; }

    // Get target FPS setting
    int GetTargetFPS() const { return m_iTargetFPS; }

    // Get frame time for delta-time calculations
    double GetFrameTimeMs() const { return m_dTargetFrameTimeMs; }

private:
    LARGE_INTEGER m_liFrequency;
    LARGE_INTEGER m_liLastFrameTime;
    LARGE_INTEGER m_liLastFPSUpdate;
    LONGLONG m_llTargetFrameTime;
    double m_dTargetFrameTimeMs;
    double m_dFrameTimeAccumulator;
    int m_iTargetFPS;
    int m_iFPSCounter;
    int m_iCurrentFPS;
    UINT m_uTimerResolution;
    bool m_bTimerInitialized;
};

/****************************************************************
*   GAME LOGIC ACCUMULATOR                                       *
*   Decouples game logic updates from rendering                  *
*****************************************************************/

class CLogicAccumulator
{
public:
    CLogicAccumulator(int iUpdatesPerSecond = DEF_LOGIC_UPDATE_RATE)
    {
        SetUpdateRate(iUpdatesPerSecond);
        QueryPerformanceFrequency(&m_liFrequency);
        QueryPerformanceCounter(&m_liLastUpdateTime);
        m_dAccumulator = 0.0;
    }

    void SetUpdateRate(int iUpdatesPerSecond)
    {
        m_iUpdatesPerSecond = iUpdatesPerSecond;
        if (iUpdatesPerSecond > 0)
        {
            m_dUpdateIntervalMs = 1000.0 / (double)iUpdatesPerSecond;
        }
        else
        {
            m_dUpdateIntervalMs = 0.0;
        }
    }

    // Call this each frame to accumulate time
    // Returns the number of logic updates that should be performed
    int Accumulate()
    {
        if (m_iUpdatesPerSecond <= 0)
            return 1;  // No limiting, always update

        LARGE_INTEGER liNow;
        QueryPerformanceCounter(&liNow);

        double dElapsedMs = (double)(liNow.QuadPart - m_liLastUpdateTime.QuadPart) 
                            * 1000.0 / (double)m_liFrequency.QuadPart;
        m_liLastUpdateTime = liNow;

        m_dAccumulator += dElapsedMs;

        int iUpdateCount = 0;
        while (m_dAccumulator >= m_dUpdateIntervalMs)
        {
            m_dAccumulator -= m_dUpdateIntervalMs;
            iUpdateCount++;

            // Prevent spiral of death - cap at 5 updates per frame
            if (iUpdateCount >= 5)
            {
                m_dAccumulator = 0.0;
                break;
            }
        }

        return max(iUpdateCount, 1);  // Always at least 1 update
    }

    // Get interpolation factor for smooth rendering between logic updates
    // Returns value between 0.0 and 1.0
    double GetInterpolation() const
    {
        if (m_dUpdateIntervalMs <= 0.0)
            return 1.0;
        return m_dAccumulator / m_dUpdateIntervalMs;
    }

private:
    LARGE_INTEGER m_liFrequency;
    LARGE_INTEGER m_liLastUpdateTime;
    double m_dAccumulator;
    double m_dUpdateIntervalMs;
    int m_iUpdatesPerSecond;
};

/****************************************************************
*   GLOBAL TIMING WRAPPER                                        *
*   Drop-in replacement for timeGetTime() with higher precision  *
*****************************************************************/

// Global high-precision timer instance
extern CHighPrecisionTimer g_HighPrecisionTimer;
extern CFPSLimiter g_FPSLimiter;
extern CLogicAccumulator g_LogicAccumulator;

// High-precision replacement for timeGetTime()
// Use this instead of timeGetTime() for better accuracy
inline DWORD GetHighPrecisionTime()
{
#if DEF_USE_HIGH_PRECISION_TIMER
    return g_HighPrecisionTimer.GetTicksMs();
#else
    return timeGetTime();
#endif
}

// Initialize modern engine systems
// Call this early in WinMain before game initialization
inline void InitializeModernEngine()
{
    // Increase process priority slightly for better timing
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

    // Disable DPI scaling to prevent stretched sprites
    SetProcessDPIAware();
}

// Cleanup modern engine systems
// Call this before application exit
inline void ShutdownModernEngine()
{
    // Restore normal priority
    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}

/****************************************************************
*   DISPLAY MODE HELPER                                          *
*   For detecting monitor refresh rate and capabilities          *
*****************************************************************/

inline int GetMonitorRefreshRate(HWND hWnd = NULL)
{
    DEVMODE dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
    {
        return dm.dmDisplayFrequency;
    }
    return 60;  // Default fallback
}

inline void GetMonitorResolution(int* pWidth, int* pHeight, HWND hWnd = NULL)
{
    HMONITOR hMonitor = MonitorFromWindow(hWnd ? hWnd : GetDesktopWindow(), 
                                          MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    
    if (GetMonitorInfo(hMonitor, &mi))
    {
        *pWidth = mi.rcMonitor.right - mi.rcMonitor.left;
        *pHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    else
    {
        *pWidth = GetSystemMetrics(SM_CXSCREEN);
        *pHeight = GetSystemMetrics(SM_CYSCREEN);
    }
}

#endif // MODERN_ENGINE_H
