/*
  Timer management code for the DMD library, includes implementation
  for AVR-based Arduino compatible (Eleven, Uno, etc.) and for Arduino Due

 Copyright (C) 2014 Freetronics, Inc. (info <at> freetronics <dot> com)

 Updated by Angus Gratton, based on DMD by Marc Alexander.

---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.
*/
#include "DMD2.h"

/*
  Uncomment the following line if you don't want DMD library to touch
  any timer functionality (ie if you want to use TimerOne library or
  similar, or other libraries that use timers.

  With timers enabled, on AVR-based Arduinos this code will register a
  Timer1 overflow handler (without disrupting any built-in library
  functions.) On ARM-based Arduino Due it will use use Timer7.

*/

//#define NO_TIMERS

#define ESP8266_TIMER0_TICKS microsecondsToClockCycles(250) // 250 microseconds between calls to scan_running_dmds seems to works better than 1000.

#ifdef NO_TIMERS

// Timer-free stub code which gets compiled in only if NO_TIMERS is set
void BaseDMD::begin() {
  beginNoTimer();
}

void BaseDMD::end() {
}

#else // Use timers

// Forward declarations for tracking currently running DMDs
static void register_running_dmd(BaseDMD *dmd);
static bool unregister_running_dmd(BaseDMD *dmd);
static void inline scan_running_dmds();

#ifdef ESP8266
static void ICACHE_RAM_ATTR esp8266_ISR_wrapper();
#endif

#ifdef __AVR__

/* This AVR timer ISR uses the standard /64 timing used by Timer1 in the Arduino core,
   so none of those registers (or normal PWM timing) is changed. We do skip 50% of ISRs
   as 50% timer overflows is approximately every 4ms, which is fine for flicker-free
   updating.
*/
ISR(TIMER1_OVF_vect)
{
  static uint8_t skip_isrs = 0;
  skip_isrs = (skip_isrs + 1) % 2;
  if(skip_isrs)
    return;
  scan_running_dmds();
}

void BaseDMD::begin()
{
  beginNoTimer(); // Do any generic setup

  char oldSREG = SREG;
  cli();
  register_running_dmd(this);
  TIMSK1 = _BV(TOIE1); // set overflow interrupt
  SREG = oldSREG;
}

void BaseDMD::end()
{
  char oldSREG = SREG;
  cli();
  bool still_running = unregister_running_dmd(this);
  if(!still_running)
    TIMSK1 &= ~_BV(TOIE1); // disable timer interrupt, no more DMDs are running
  SREG = oldSREG;
  // One final (manual) scan to turn off all LEDs
  clearScreen();
  scanDisplay();
}

#elif defined (__arm__) // __ARM__, Due assumed for now

/* ARM timer callback (ISR context), checks timer status then scans all running DMDs */
void TC7_Handler(){
  TC_GetStatus(TC2, 1);
  scan_running_dmds();
}

void BaseDMD::begin()
{
  beginNoTimer(); // Do any generic setup

  NVIC_DisableIRQ(TC7_IRQn);
  register_running_dmd(this);
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(TC7_IRQn);
  // Timer 7 is TC2, channel 1
  TC_Configure(TC2, 1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4); // counter up, /128 divisor
  TC_SetRC(TC2, 1, 2500); // approx 4ms at /128 divisor
  TC2->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;

  NVIC_ClearPendingIRQ(TC7_IRQn);
  NVIC_EnableIRQ(TC7_IRQn);
  TC_Start(TC2, 1);
}

void BaseDMD::end()
{
  NVIC_DisableIRQ(TC7_IRQn);
  bool still_running = unregister_running_dmd(this);
  if(still_running)
    NVIC_EnableIRQ(TC7_IRQn); // Still some DMDs running
  else
    TC_Stop(TC2, 1);
  clearScreen();
  scanDisplay();
}

#elif defined (ESP8266)

void BaseDMD::begin()
{
  beginNoTimer();
  timer0_detachInterrupt();

  register_running_dmd(this);

  timer0_isr_init();
  timer0_attachInterrupt(esp8266_ISR_wrapper);
  timer0_write(ESP.getCycleCount() + ESP8266_TIMER0_TICKS);
}

void BaseDMD::end()
{
  bool still_running = unregister_running_dmd(this);
  if(!still_running)
  {
    timer0_detachInterrupt(); // timer0 disables itself when the CPU cycle count reaches its own value, hence ESP.getCycleCount()
  }
  clearScreen();
  scanDisplay();
}

#endif // ifdef __AVR__

/* Following functions are static non-architecture-specific functions
   to manage a global array that tracks all known DMD instances.

   This is so a global C code interrupt can call all of the DMDs on
   timer interrupt.

   The array is grown with realloc. If a DMD is destroyed then it
   shrink the array, it just NULLs out the entry (presuming there
   isn't a lot of dynamic growth/shrinkage of DMDs within a sketch!)
*/
static volatile BaseDMD **running_dmds = 0;
static volatile int running_dmd_len = 0;

// Add a running_dmd to the list (caller must have disabled interrupts)
static void register_running_dmd(BaseDMD *dmd)
{
  int empty = -1;
  for(int i = 0; i < running_dmd_len; i++) {
    if(running_dmds[i] == dmd)
      return; // Already running and registered
    if(!running_dmds[i])
      empty = i; // Found an unused slot in the array
  }

  if(empty == -1) { // Grow array to fit new entry
    running_dmd_len++;
    BaseDMD **resized = (BaseDMD **)realloc(running_dmds, sizeof(BaseDMD *)*running_dmd_len);
    if(!resized) {
      // Allocation failed, bail out

      running_dmd_len--;
      return;
    }
    empty = running_dmd_len-1;
    running_dmds = (volatile BaseDMD **)resized;
  }
  running_dmds[empty] = dmd;
}

// Null out a running_dmd from the list (caller must have disabled interrupts)
static bool unregister_running_dmd(BaseDMD *dmd)
{
  bool still_running = false;
  for(int i = 0; i < running_dmd_len; i++) {
    if(running_dmds[i] == dmd)
      running_dmds[i] = NULL;
    else if (running_dmds[i])
      still_running = true;
  }
  return still_running;
}

// ESP8266 ISR Wrapper
#ifdef ESP8266
static void inline ICACHE_RAM_ATTR esp8266_ISR_wrapper()
{
  if(((int)0x40200000)) { //Make sure flash isn't being accessed.
    scan_running_dmds();
  }
  timer0_write(ESP.getCycleCount() + ESP8266_TIMER0_TICKS);
}
#endif

// This method is called from timer ISR to scan all the DMD instances present in the running sketch
static void inline __attribute__((always_inline)) scan_running_dmds()
{
  for(int i = 0; i < running_dmd_len; i++) {
    BaseDMD *next = (BaseDMD*)running_dmds[i];
    if(next) {
      next->scanDisplay();
    }
  }
}

#endif // ifdef NO_TIMERS
