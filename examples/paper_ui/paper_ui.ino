#include <MadDisplay.h>
#include <GxEPD2_BW.h>
#include <MadEPaper.h>
// Exact panel revision: GDEH0154D67. ESP32 control pins CS, DC, RST, BUSY.
using Driver=GxEPD2_BW<GxEPD2_154_D67,16>;
Driver driver(GxEPD2_154_D67(5,2,4,15));
mad::EPaper<Driver,200,200,1> adapter(driver,"epd-154-bw");
mad::Display<decltype(adapter)> display(adapter);
mad::VerticalLayout root;
mad::Page page(root,mad::Color::White);
mad::Label title("MAD DISPLAY",mad::LabelStyle(mad::Color::Black,1));
void setup() {
  if (!display.begin()) while (true) { delay(100); }
  root.add(title); display.setFps(1);
  display.openPage(display.registerPage(page));
}
void loop() { display.render(micros()); }
