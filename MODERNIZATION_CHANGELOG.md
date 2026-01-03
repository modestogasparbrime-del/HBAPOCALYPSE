# Helbreath Client - Modernization Changelog

## Overview

This document describes the technical modernizations made to the Helbreath Client to ensure smooth operation on modern Windows 10/11 systems with high-refresh-rate monitors.

**Important:** All changes preserve the original game's visual aesthetics, sprites, UI, and text. The gameplay behavior remains identical to the original.

---

## Changes Made

### 1. High-Precision Timing System (`ModernEngine.h/cpp`)

**Problem:** The original game used `timeGetTime()` which has ~15ms resolution on modern Windows, causing timing inconsistencies.

**Solution:** Implemented `QueryPerformanceCounter`-based timing with microsecond accuracy.

**Files Modified:**
- New: `ModernEngine.h` - Header with timing classes
- New: `ModernEngine.cpp` - Global timer instances
- Modified: `Game.cpp` - Uses `GetHighPrecisionTime()` instead of `timeGetTime()`
- Modified: `Wmain.cpp` - Integrated modern engine initialization

**Key Classes:**
- `CHighPrecisionTimer` - Drop-in replacement for `timeGetTime()` with sub-millisecond accuracy
- `CFPSLimiter` - Intelligent frame rate limiter with CPU-friendly waiting
- `CLogicAccumulator` - Decouples game logic from rendering for 144Hz+ monitor support

---

### 2. FPS Limiter with CPU Optimization

**Problem:** The original event loop used busy-waiting, consuming 100% CPU on one core.

**Solution:** Implemented an intelligent FPS limiter that:
- Uses `Sleep()` for long waits (>2ms) to reduce CPU usage
- Uses `Sleep(0)` to yield to other threads for short waits
- Uses spin-waiting only for the final sub-millisecond precision

**Configuration (in `ModernEngine.h`):**
```cpp
#define DEF_TARGET_FPS              240   // Target frame rate (supports up to 240Hz monitors)
#define DEF_USE_HIGH_PRECISION_TIMER  1   // Enable high-precision timing
```

---

### 3. Timing System Compatibility Fix

**Problem:** The initial high-precision timer returned time since initialization, but existing game code compared timestamps saved with `timeGetTime()` (time since system boot). This caused absurd values when displaying loading time or calculating FPS.

**Solution:** Modified `CHighPrecisionTimer::GetTicksMs()` to capture the `timeGetTime()` base value at initialization and add the high-precision offset to it. This ensures all timestamps are compatible with existing game code.

**Implementation:**
```cpp
// Constructor captures timeGetTime() base
m_dwTimeGetTimeBase = timeGetTime();

// GetTicksMs returns compatible timestamps
DWORD GetTicksMs() const {
    DWORD dwOffset = (DWORD)((liNow.QuadPart - m_liStartTime.QuadPart) * 1000 / m_liFrequency.QuadPart);
    return m_dwTimeGetTimeBase + dwOffset;
}
```

---

### 4. Optimized Event Loop (`Wmain.cpp`)

**Changes:**
- Replaced `PeekMessage/GetMessage` with `PeekMessage/PM_REMOVE` pattern
- Added `MsgWaitForMultipleObjects()` when inactive to minimize CPU usage
- Integrated FPS limiter for consistent frame pacing

**Before:**
```cpp
while (true) {
    if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
        if (!GetMessage(&msg, 0, 0, 0)) return;
        // ...
    }
    else if (G_pGame->m_bIsProgramActive) G_pGame->UpdateScreen();
    // ...
}
```

**After:**
```cpp
while (true) {
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (G_pGame->m_bIsProgramActive) {
        g_FPSLimiter.WaitForNextFrame();
        G_pGame->UpdateScreen();
    }
    // ...
}
```

---

### 5. Borderless Fullscreen (`DXC_ddraw.cpp`)

**Problem:** The original full-screen mode used exclusive fullscreen which caused issues on modern systems.

**Solution:** 
- Uses `DDSCL_NORMAL` cooperative level (borderless windowed mode)
- Game stretches to fill the entire screen (preserves original behavior)
- Removed letterboxing/pillarboxing to match original visual appearance

---

### 6. DPI Awareness

**Problem:** High-DPI displays could cause scaling issues.

**Solution:** Added `SetProcessDPIAware()` call during initialization to prevent Windows DPI scaling.

---

### 7. Memory Leak Fix

**Problem:** `m_pMobKillCount` array was not being freed in the destructor.

**Solution:** Added cleanup loop in `CGame::Quit()`:
```cpp
for (i = 0; i < 100; i++)
    if (m_pMobKillCount[i] != 0) delete m_pMobKillCount[i];
```

---

## Configuration Options

All configuration options are in `ModernEngine.h`:

| Option | Default | Description |
|--------|---------|-------------|
| `DEF_TARGET_FPS` | 240 | Target frame rate (0 = unlimited, max 240 for high refresh monitors) |
| `DEF_USE_HIGH_PRECISION_TIMER` | 1 | Enable QueryPerformanceCounter timing |
| `DEF_MIN_SLEEP_TIME_MS` | 1 | Minimum sleep granularity |
| `DEF_CPU_IDLE_OPTIMIZATION` | 1 | Enable CPU idle optimization |
| `DEF_LOGIC_UPDATE_RATE` | 30 | Game logic updates per second |
| `DEF_BORDERLESS_FULLSCREEN` | 1 | Enable borderless fullscreen mode |

---

## Compatibility Notes

### DirectDraw 7
The game continues to use DirectDraw 7 for rendering. For best compatibility on modern GPUs, consider:
- Using **dgVoodoo2** wrapper to translate DDraw calls to Direct3D 11
- Using **dxwrapper** for additional compatibility fixes

### Windows Versions
- **Windows 10/11:** Fully supported with all optimizations
- **Windows 7/8:** Should work, but high-precision timing may have reduced accuracy

### Monitor Refresh Rates
- **60Hz:** Works identically to original
- **144Hz+:** Game logic remains at 60 FPS, no speed acceleration
- **Variable Refresh (G-Sync/FreeSync):** Supported, game runs at target FPS

---

## Files Modified

| File | Change Type | Description |
|------|-------------|-------------|
| `ModernEngine.h` | New | High-precision timing and FPS limiter |
| `ModernEngine.cpp` | New | Global timer instances |
| `Wmain.cpp` | Modified | Modern engine integration, optimized event loop |
| `Game.cpp` | Modified | High-precision timing, memory leak fix |
| `DXC_ddraw.cpp` | Modified | Aspect ratio preservation, borderless mode |
| `Client.vcxproj` | Modified | Added new source files |

---

## Building

1. Open `Client.sln` in Visual Studio 2019 or later
2. Select Release configuration
3. Build the solution

The project includes all necessary headers and source files.

---

## Reverting Changes

To revert to the original behavior:

1. In `ModernEngine.h`, set:
   ```cpp
   #define DEF_USE_HIGH_PRECISION_TIMER  0
   #define DEF_TARGET_FPS                0
   ```

2. Or remove the `#include "ModernEngine.h"` lines and restore `timeGetTime()` calls.

---

## Credits

- Original Helbreath Client: Siementech (1998-2002)
- Modernization Layer: 2026
