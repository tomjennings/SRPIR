/*

 SR timer system.

 simple timer event system based upon millis().

 tom jennings

 11 mar 2019 _timer interval now unsigned. every
             method checks incoming timer number. all
             methods except begin have return value 
             success or value.
 17 dec 2017 timer now fires on .GE. not .GT.
 09 dec 2017 backed out TM4C timer stuff. too kludgey.
 08 dec 2017 added support for TM4C123x timer instead
             of millis() and micros() since they are bust,
             and added T.ticks() to return the timer
             contents.
 29 jul 2017 getTimer, untiTimer now long, not unsigned.
 04 mar 2017 slight edit to expose odd link error
 25 mar 2015 added setDeciTimer(), return values are
             now all long, struct interval is long.
 20 mar 2015 converted to new SR name system
 28 feb 2015 added missing getTimer()
 22 feb 2015

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

*/

#include "Arduino.h"

#ifndef SR_TIMER
#define SR_TIMER

class SRTimer {


struct _timer {
	uint32_t T;		// future time of event, mS
	uint32_t interval;	// timer period length, mS
};

public: 
	void begin (unsigned n);
	bool timer (unsigned n);
	bool setTimer (unsigned n, unsigned m);
	bool setDeciTimer (unsigned n, unsigned d);
	uint32_t getTimer (unsigned n);
	uint32_t untilTimer (unsigned n);
	bool resetTimer (unsigned n);
	bool trigTimer (unsigned n);
	
private:
	unsigned numTimers;
	struct _timer * timers;
};


void SRTimer::begin (unsigned n) {

	timers= new _timer [numTimers= n];
	for (n= 0; n < numTimers; n++) {
		timers[n].T= timers[n].interval= 0;
	}
};


// if timer N has been reached or exceeded, reset it and return true.
//
bool SRTimer::timer (unsigned n) {
uint32_t t;
int32_t e;

  if (n >= numTimers) return false;

  t= millis();
  e= t - timers[n].T;		/* use unsigned arith */
  if (e >= 0) {			/* to avoid wrap errors */
    timers[n].T= t + timers[n].interval;
    return true;
  }
  return false;
}

// set timer N to go true in M mS.
//
bool SRTimer::setTimer (unsigned n, unsigned m) {

  if (n >= numTimers) return false;

  timers[n].interval= m;
  timers[n].T= millis() + m;
  return true;
}

// set timer N to go true in S deciseconds.
//
bool SRTimer::setDeciTimer (unsigned n, unsigned d) {

  if (n >= numTimers) return false;
  timers[n].interval= 100L * d;		// make that deciseconds
  timers[n].T= millis() + timers[n].interval;
  return true;
}


// return the interval for timer N.
//
uint32_t SRTimer::getTimer (unsigned n) {

  if (n >= numTimers) return 0;
  return timers[n].interval;
}

// return the time left until timer N fires.
//
uint32_t SRTimer::untilTimer (unsigned n) {

  if (n >= numTimers) return 0;
  return timers[n].T - millis();
}

// re-set timer N.
//
bool SRTimer::resetTimer (unsigned n) {

  if (n >= numTimers) return false;
  timers[n].T= millis() + timers[n].interval;
  return true;
}

// force timer N to go true immediately (leaving set interval alone).
//
bool SRTimer::trigTimer (unsigned n) {

  if (n >= numTimers) return false;
  timers[n].T= millis();
  return true;
}


#endif

