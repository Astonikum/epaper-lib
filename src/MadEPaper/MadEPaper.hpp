#pragma once
#include <MadDisplay/panel_presentation.hpp>
#include <stdlib.h>

namespace mad {
// Driver is a concrete paged GxEPD2_BW/3C/7C type. Page height 16 matches the Studio RAM model.
template<class Driver,int16_t Width,int16_t Height,uint8_t Bits=1>
class EPaper final : public PaletteBuffer<Width,Height,Bits> {
  using Base=PaletteBuffer<Width,Height,Bits>;
 public:
  EPaper(Driver& driver,const char* profile) : Base(panelPalette(panelSpec(profile))),driver_(driver),spec_(panelSpec(profile)) {}
  ~EPaper() override { if (ownPrevious_) free(previous_); }
  EPaper(const EPaper&)=delete; EPaper& operator=(const EPaper&)=delete;
  // For large panels, the adapter object (containing its frame) and history may live in PSRAM.
  bool setHistoryBuffer(uint8_t* history,size_t capacity) {
    if (ready_ || previous_ || !history || capacity<Base::kByteCount) return false;
    const uintptr_t a=reinterpret_cast<uintptr_t>(history), b=reinterpret_cast<uintptr_t>(this->pixels());
    if ((a>b ? a-b : b-a)<Base::kByteCount) return false;
    previous_=history; return true;
  }
  bool begin(uint8_t rotation=0) {
    if (rotation>3 || !spec_.epaper() || spec_.width!=Width || spec_.height!=Height || spec_.bits()!=Bits || driver_.pageHeight()!=16) return false;
    if (!previous_) { previous_=static_cast<uint8_t*>(calloc(Base::kByteCount,1)); ownPrevious_=previous_!=nullptr; }
    if (!previous_) return false;
    driver_.init(0,true,2,false); driver_.setRotation(rotation);
    ready_=driver_.width()==Width && driver_.height()==Height;
    if (!driver_.epd2.hasFastPartialUpdate) spec_.partialRefreshMs=0;
    planner_.invalidate(); return ready_;
  }
  void present() override {
    if (!ready_) return;
    const auto update=planner_.plan(spec_,this->pixels(),previous_); transfer_=update.transfer;
    if (!transfer_.bytes) return;
    if (update.full) driver_.setFullWindow();
    else driver_.setPartialWindow(update.bounds.x,update.bounds.y,update.bounds.width,update.bounds.height);
    driver_.firstPage(); uint16_t page=0;
    do {
      driver_.fillScreen(0xffff);
      int16_t left=update.bounds.x, top=update.bounds.y;
      int16_t right=left+update.bounds.width, bottom=top+update.bounds.height;
      // Full refreshes may have two passes. Restrict each pass to its native page.
      if (update.full) {
        const uint8_t rotation=driver_.getRotation();
        const int16_t extent=(rotation%2) ? Width : Height;
        int16_t first=page*16, last=first+16; if (last>extent) last=extent;
        if (rotation>=2) { const int16_t saved=first; first=extent-last; last=extent-saved; }
        if (rotation%2) { left=first; right=last; } else { top=first; bottom=last; }
      }
      for (int16_t y=top;y<bottom;++y)
        for (int16_t x=left;x<right;++x)
          driver_.drawPixel(x,y,rgb565(Base::pixel(x,y)));
      page=(page+1)%driver_.pages();
    } while (driver_.nextPage());
    memcpy(previous_,this->pixels(),Base::kByteCount); planner_.committed(update);
  }
  void invalidate() { planner_.invalidate(); }
  const TransferStats& transfer() const { return transfer_; }
  Driver& driver() { return driver_; }
 private:
  Driver& driver_; PanelSpec spec_; EpaperPlanner planner_; TransferStats transfer_;
  uint8_t* previous_=nullptr; bool ready_=false,ownPrevious_=false;
};
} // namespace mad
