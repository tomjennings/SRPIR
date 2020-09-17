/*

   PID controller

   not really, I term is actually done with Exponential Smoothing, 
   which works better and is tuneable without recompiling
   or messing with malloc() and requires less memory.

   floating point only

   the older SRPID that is all integer is funky and obsolete.


  tom jennings <tom@SensitiveResearch.com>
  
  03 sep 2020   Oops: integFill() did not apply gain to seed value.
  25 aug 2020   all args are floats now, including loop time,
  		to accommodate long time constants.
  28 apr 2018   added smoother SF calc, prefill methods to begin.
  		everything now returns something.
  31 oct 2017   removed bounding methods; moved all the non-linearity
                into unwound Roadster PID cooling code.
  28 oct 2017   nasty bug fixed, bounding integral in pid(), neglected to
                bound the smoother's internal value.
		also, fill() did not set local integralV,
		unlikely to have any effect, as it would only matter if
		integral() was called after fill() and before pid().
  13 aug 2017   added integFill (f) to accommodate roadster cooling
                hot engine startup.
  12 aug 2017   bounding renamed to integBounds (lo hi). applies
                only to integral.
  09 may 2017	added integRegimePositive() for Roadster cooling.
  		this clips negative integration input values
		to zero.
  08 may 2017   changed I term to be pre-scale, not post-scale.
  		changes to I gain (gain scheduling) caused large
		discontinuous jump.
  02 may 2017	method difference() was returning integral
  14 jan 2017	renamed class to differentiate from old one
  29 dec 2016	derived from SRPIDint
  

copyright tom jennings 2015, 2016, 2017, 2018, 2020


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SR_SMPID
#define SR_SMPID

#include "Arduino.h"

class SRSMPID {
private:
  float propGainV;
  float diffGainV;
  float integGainV;
  float proportionV, integralV, differenceV;	/* accessible intermediates */
  SRSmooth S;					/* our local smoother */
  float prev_d;					/* differentiation history */

/* -------------------------------------------------------------------------- */

public:

/* minimum begin; set smoothing factor (0..1) */

float begin (float sf) {

  prev_d= 0;
  proportionV= integralV= differenceV= 0;

  /* this historic default setup makes PID zero-seeking, and 
   * good at simple information extraction from a digital pin.
   */
  integGainV= 1;
  propGainV= -1;  
  diffGainV= 1;
  return S.begin (sf);
}

/* begin calculating SF from loop time t and time constant loopT */

float begin (float tc, float loopT) {

  prev_d= 0;
  proportionV= integralV= differenceV= 0;
  integGainV= 1;
  propGainV= -1;  
  diffGainV= 1;
  return S.begin (tc, loopT);
}

/* begin, calc SF from t and loopT, also pre-fill. */

float begin (float tc, float loopT, float fill) {

  prev_d= 0;
  proportionV= integralV= differenceV= 0;
  integGainV= 1;
  propGainV= -1;  
  diffGainV= 1;
  return S.begin (tc, loopT, fill);
}

/* likewise, explicit set-time-constant methods. */

float setTC (float sf) {
  return S.setSF (sf);
}

float setTC (float tc, float loopT) {
  return setTC (tc, loopT);
}

float setTC (float tc, float loopT, float fill) {
  return S.setTC (tc, loopT, fill);
}



float pid (float n) {

  integralV= S.smooth (n * integGainV);
  proportionV= n * propGainV;
  differenceV= (n - prev_d) * diffGainV;
  prev_d= n;
  return proportionV + integralV + differenceV;
}

float SF (float f) { return S.setSF (f); }
float SF (void) { return S.setSF(); }

float propGain (float f) { return propGainV= f; }
float propGain (void) { return propGainV; }
float integGain (float f) { return integGainV= f; }
float integGain (void) { return integGainV; }
float diffGain (float f) { return diffGainV= f; }
float diffGain (void) { return diffGainV; }

float integFill (float f) {

	integralV= f * integGainV;
	S.fill (integralV);
	return integralV;
}

float proportion (void) { return proportionV; }
float integral (void) { return integralV; }
float difference (void) { return differenceV; }


}; /* end class */

#endif

