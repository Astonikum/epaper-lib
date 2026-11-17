# MadEPaper

MadDisplay packed palette frames and shared refresh planning on top of GxEPD2
1.6.9. See [display catalog](https://github.com/Astonikum/display-studio/blob/main/DISPLAYS.md) for **exact panel revision classes**.
A screen's diagonal alone does not identify its controller.

Include the appropriate GxEPD2_BW.h, GxEPD2_3C.h or GxEPD2_7C.h before declaring
the driver. The adapter header itself only needs the concrete driver interface.

```cpp
using Driver = GxEPD2_BW<GxEPD2_154_D67,16>;
Driver driver(GxEPD2_154_D67(cs,dc,rst,busy));
mad::EPaper<Driver,200,200,1> panel(driver,"epd-154-bw");
mad::Display<decltype(panel)> display(panel);
// setup: display.begin(0); loop: display.render(micros());
```

Page height **16** is required and matches the Studio RAM estimate. Bits are 1
for black/white, 2 for black/white/red, and 4 for 6/7-color. The adapter validates
dimensions after rotation. 250x122 and 296x128 profiles need `begin(1)` with the
portrait GxEPD2 drivers. Other listed profiles use `begin(0)`.

Unchanged frames do not refresh. Fast monochrome panels use an aligned dirty
window, then a full cleaning refresh after seven partial updates. Fast partial
updates are disabled when the actual driver lacks that capability. Color profiles
use full refreshes. Refresh is blocking, with physical BUSY handling in GxEPD2.
`invalidate()` requests a cleaning refresh. Vendor APIs do not return a reliable
transaction-success status, so a disconnected panel cannot be diagnosed by this
adapter's begin result alone. Configure GxEPD2 diagnostics for hardware debugging.

Memory is two full packed frames plus a small vendor page. The adapter object
contains the current frame; history is allocated in begin. Large 6/7-color screens
need a board with sufficient RAM or PSRAM. To use PSRAM explicitly on ESP32:

```cpp
using Adapter = mad::EPaper<Driver,800,480,4>; // Driver must be the matching 7.3-inch type
void* objectMemory=heap_caps_malloc(sizeof(Adapter),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
uint8_t* history=static_cast<uint8_t*>(heap_caps_malloc(192000,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
// Check both allocations before use; include <new> and <esp_heap_caps.h>.
Adapter* panel=new (objectMemory) Adapter(driver,"epd-73-6c");
if (!panel->setHistoryBuffer(history,192000) || !panel->begin()) { /* handle failure */ }
// Keep driver, objectMemory and history alive while the display uses panel.
// On teardown: panel->~Adapter(); heap_caps_free(objectMemory); heap_caps_free(history).
```

The 6-color profile restricts the GDEP073E01 driver's palette to black, white,
red, yellow, blue and green. The 5.65-inch ACeP profile adds orange. Other panel
revisions require a matching GxEPD2 driver and measured timing adjustments.
Use `examples/paper_ui/paper_ui.ino` and `pio run` for the 1.54-inch ESP32 example.

Upstream: https://github.com/ZinggJM/GxEPD2

## Installation

Repository: https://github.com/Astonikum/epaper-lib

PlatformIO (`platformio.ini`):

```ini
lib_deps =
  https://github.com/Astonikum/epaper-lib.git#v0.2.0
```

PlatformIO resolves the core, adapter support and vendor dependencies automatically.

Arduino IDE: download **Code → Download ZIP**, then use **Sketch → Include Library → Add .ZIP Library**.
Install these GitHub ZIP libraries first:

- [MadDisplay](https://github.com/Astonikum/display-lib)
- [MadDisplayAdapters](https://github.com/Astonikum/adapter-common-lib)

Install from Arduino Library Manager: GxEPD2, Adafruit GFX Library, Adafruit BusIO.

These packages are distributed through GitHub; they are not yet indexed in the Arduino or PlatformIO registries. Arduino IDE does not install a library directly from a Git URL.

Requires C++14 on the host. Embedded examples select their board and its Arduino toolchain in `platformio.ini`. RAM requirements are documented above; a matching controller does not guarantee a board has enough memory.
