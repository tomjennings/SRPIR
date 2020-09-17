/*
  basic signal processing functions

  exponential smoothing

  https://en.wikipedia.org/wiki/Exponential_smoothing

  see notes on tuning

  and especially http://people.duke.edu/~rnau/411avg.htm

  for SES and double SES



  tom jennings <tom@SensitiveResearch.com>
  
  25 aug 2020	all args are now floats. Renamed all vars.
  26 jun 2020	break out history for cheapo multiplexing.
  14 may 2018   renamed confusing local vars, ambiguity
  		led to bad code.
  28 apr 2018   added return values to set and begin,
  		s/set/setSF/ or setTC depending on
		args. this may break things.
  04 jan 2018   added begin (f, T, fill)
  23 apr 2017	added setTC (t, T) method
  27 dec 2016	added set() method. moved all methods
  		into the main object.
  01 jun 2016

copyright tom jennings 2016, 2017, 2018, 2020


This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2.1 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 51
Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SR_SMOOTH
#define SR_SMOOTH

#include "Arduino.h"

class SRSmooth {

private:
float fh;             // filter history
float sf;             // smoothing factor; 1 == no filtering, .01 heavy filter

public:

float begin (float f) {
  return setSF (f);
}

float begin (float TC, float loopT) {

  return setSF (loopT / TC);
}

float begin (float TC, float loopT, float f) {

  fh= f;
  return setSF (loopT / TC);
}





float smooth (float v) {

  fh= (sf * v) + ((1.00 - sf) * fh);
  return fh;
}

float hist (void) {

  return fh;
}

float hist (float h) {

  fh= h;
  return h;
}


/* set the time constant of the smoother to approximate time t,
given the loop rate (sample rate), T. this returns the calculated
smoothing factor. t and T are in milliseconds. */

float setTC (float TC, float loopT) {

  return setSF (loopT / TC);
}

/* likewise, also prefill. */

float setTC (float TC, float loopT, float f) {

  fill (f);
  return setTC (TC, loopT);
}

/* set the smoothing factor */

float setSF (float f) {
  return sf= f;
}

/* return the current smoothing factor */

float setSF (void) {

  return sf;
}

/* initialize smoothing history. */

float fill (float f) {
  return fh= f;
}

}; /* end class */


#endif

