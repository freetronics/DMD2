#include "DMD2.h"
#include <assert.h>
#include <stdio.h>
class Frame : public DMDFrame {
public:
  Frame(uint16_t w, uint16_t h):DMDFrame(w,h) {}
  size_t bytes() { return bitmap_bytes(); }
};
static bool pattern(unsigned x, unsigned y) { return ((x*13+y*7)%19)<7; }
int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  const uint16_t sizes[][2]={{1,1},{7,15},{8,16},{32,16},{64,16},{32,32},
    {255,16},{256,16},{257,17},{512,32},{2040,16},{2048,16},{32,255},{32,256},{32,257}};
  for (auto &s:sizes) {
    Frame f(s[0],s[1]); assert(f.width==s[0] && f.height==s[1]);
    assert(f.bytes()==size_t((s[0]+7)/8)*(s[1]<=16?s[1]:((s[1]+15)/16)*16));
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      f.setPixel(x,y,pattern(x,y)?GRAPHICS_ON:GRAPHICS_OFF);
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      assert(f.getPixel(x,y)==pattern(x,y));
    DMDFrame copied(f);
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      assert(copied.getPixel(x,y)==pattern(x,y));
    f.clearScreen(); f.copyFrame(copied,0,0);
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      assert(f.getPixel(x,y)==pattern(x,y));
    for (int delta : {-int(f.width)-1, -1, 0, 1, int(f.width)+1}) {
      f.clearScreen(); f.copyFrame(copied,0,0); f.scrollX(delta);
      for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++) {
        int source=int(x)-delta;
        assert(f.getPixel(x,y)==(source>=0 && source<f.width && pattern(source,y)));
      }
    }
    for (int delta : {-int(f.height)-1, -1, 0, 1, int(f.height)+1}) {
      f.clearScreen(); f.copyFrame(copied,0,0); f.scrollY(delta);
      for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++) {
        int source=int(y)-delta;
        assert(f.getPixel(x,y)==(source>=0 && source<f.height && pattern(x,source)));
      }
    }
    f.setFlipPanels(true); f.clearScreen();
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      f.setPixel(x,y,pattern(x,y)?GRAPHICS_ON:GRAPHICS_OFF);
    for(unsigned y=0;y<f.height;y++) for(unsigned x=0;x<f.width;x++)
      assert(f.getPixel(x,y)==pattern(x,y));
    f.setPixel(f.width,f.height); assert(!f.getPixel(f.width,f.height));
    printf("PASS %ux%u\n",f.width,f.height);
  }
}
