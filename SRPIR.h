/*

  PIR (Passive InfraRed, Pyroelectric InfraRed) sensor driver

  tom jennings, tom@sr-ix.com


  This code emulates the behavior of PIR Detector/Controllers like
  ON Semi NCS36000 and others.

  HARDWARE REQUIREMENTS:

  Three-legged raw PIR sensor, with source-follower (open source, but not that/this kind!)
  driving a 47K resistor to ground, and an op amp for gain. Without the opamp, the signal 
  is *just sufficient* to drive a 3.3V ADC input if you generate a LARGE IR signal (your hand, 
  slow, inches away).  A pair of opamps with total voltage gain A=1000 to 4000, eg. 
  two A=60 in series.


  This code called in a loop will return true when an event (defined below) is detected.

  Requires SRTimer, SRPID, SRSmooth.



  THE SENSOR

  The sensor outputs an analog voltage that represents the difference in 10 - 15 uM infrared
  across an internal pair of cells. "Heat" moving past the window produces two analog
  bumps of opposite polarity. 

  The pulses are slow, many tens of milliseconds, befitting the physical motion
  of a warm blooded mammal (a large bag of hot water will do).

  The sensor is somewhat noisy, producing a bi-polar signal impressed on DC. The signal is
  tens/hundreds of millivolts, and requires gain (eg. an op amp).

(display in wide window)

The signal plotted time (X) vs voltage (Y)


                    glitch                A hot B cold
                   <----->            <----------------->


                                           **********
                                          **         *
                                       ***            **
                                      **               *
                    ****             **                **
THRESHOLD     -----**--**------------*------------------*----------------------------------------------------------------------------------------------------------->
                   *    *           **                  *
                  **    *          **                   **
                 **     *         **                     *   ****     ****                                             ***
                **      **      ***                      *****  *    **  *                                   *****    ** *********                      ****    ****
               **        ***** ***                        **    ** ***  **                          **********   ******          **                    **  *** **   **
                             ***                                 ***    *                        ****                             **                   *     ***
                                                                        *                       **                                 **                 **
                                                                        **                     **                                   **                *
-THRESHOLD    -----------------------------------------------------------*--------------------**-------------------------------------**---------------*------------->
                                                                         *                    *                                       **             **
                                                                         **                  *                                        **             *
                                                                          **                **                                         *             *
                                                                           *               **                                          **           **
                                                                           **             **                                            **          *
                                                                            ***         **                                               **        **
                                                                              ***      **                                                  *       *
                                                                                ********                                                   ***    **
                                                                                                                                             ******

                                                                          <------------------>                                        <-------------->
                                                                                                                                      spurious/unknown
                                                                              A cold B hot

Created with Monodraw


Here, an "event" is defined as 'we have detected a warm body moving past the sensor that 
we deem to be a person or equiv'


* Pulses have a minimum width (else "glitch"), abour 30 mS
* An event is either:
*   SINGLE MODE: One positive pulse of sufficient width (A-hot-B-cold, or A-cold-B-hot)
*   DUAL MODE: One positive pulse followed by a negative pulse within N seconds.

SINGLE mode is fastest-response to motion, and slightly more likely to generate false events.
DUAL mode is slower (trigger produced after body has passed by) but unlikely to generate false events.

(I suspect, but have not tested, that relative speed is revealed in the time between the two pulses.)




	
copyright tom jennings 2020


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

#ifndef __SRPIR_H
#define __SRPIR_H

class SRPIR {

private:


const float DEFAULTGAIN =           5.0;   // gain for PID (kludge: 3000 if no op amp)
const unsigned long PIRHOLDOFF =  10000;   // wait for the integrators to settle, minimum, mS
const int PIRGLITCH =                35;   // PIR pulse width minimum, else glitch, mS
const unsigned long PIRMAXEVENT = 20000;   // for dual-pulse, how long we'll wait for the 2nd, mS
const unsigned long PIRMINEVENT =   500;   // for dual-pulse, how little we'll wait for the 2nd, mS

const int LOOPTIMER =          0;
const int REPORTTIMER =        1;    // debug info
const int SENSORTIMER =        2;    // sensor signal processing timer (low pass, PID)
const int EVENTTIMER =         3;    // internal reaction timer (eg. play songs)
const int STIMTIMER =          4;    // delay timer for telling the flock
const int NUMTIMERS =          5;
SRTimer T;

const int LOOPTIME =          10;    // how often to look (no need for speed), mS
const int SENSETIME =         25;    // how often we run signal processing, mS
const int EVENTTIME =       3000;    // how often we generate events, mS

const int SENSELPTC =        500;    // raw sensor low-pass filter TC, mS
const int SENSETC =          500;    // event separator diff/int, mS

SRSmooth SenseLP;                    // raw data filter
SRSMPID Sense;                       // event separator

int threshold;                       // noise floor (arbitrary units)
bool PIRDualPulse;                   // single pulse (false) vs two pulse (true)
bool debugV;                         // set true, prints out a lot of crap
bool startup;                        // one-time startup crap

// From the outside world.
//
int pin;                             // analog input pin
float gain;                          // PID gain

// To the outside world.
//
bool trig;

public:

void begin (int p) {
int n;

  pinMode (pin, INPUT_PULLUP);
  T.begin (NUMTIMERS);
  T.setTimer (LOOPTIMER, LOOPTIME);          // minimize load
  T.setTimer (REPORTTIMER, 1000);            // printing debug shit
  T.setTimer (SENSORTIMER, SENSETIME);       // run the math
  T.setTimer (EVENTTIMER, EVENTTIME);        // event generation
  T.setTimer (STIMTIMER, 9999);              // set in loop()
 
  // Startup the low-pass filter and the PID detector. Attempt to seed the
  // filter and PID with a reasonable value off the sensor, to speed its
  // settling. They are slow.
  //
  n= analogRead (pin);
  SenseLP.begin (SENSELPTC, SENSETIME, n);   // analog sensor low-pass filter
  n= SenseLP.smooth (n);                     // "current value" (kinda sorta)
  Sense.begin   (SENSETC, SENSETIME, n);     // initial PID values

  setGain (DEFAULTGAIN);                     // reasonable gain
  setMode (false);                           // single pulse mode default
  setThreshold (8);                          // low threshold
  startup= true;
}


// Runs the sensor machines, returns true if an event is detected.
//
bool loop () {
static bool h = false;                       // dual mode, which pulse we need
float r, v;
static long t;
int n;

  if (T.timer (SENSORTIMER) == false) return false;

  r= analogRead (SENSOR);                     // raw sensor, noisy
  v= SenseLP.smooth (r);                      // removes most noise
  v= Sense.pid (v);                           // low-pass, differentiator removes DC

  if (millis() < PIRHOLDOFF) return false;    // let everything settle

  trig= false;

  switch (PIRDualPulse) {

    // DUAL PULSE MODE
    //
    case true:
      switch (h) {

          // A positive-going pulse of sufficient width starts event detection.
          //
          case false:
            n= findPulse (v, threshold, PIRGLITCH);
            if (n > 0) {
              h= true;
              t= millis();
  
              if (debugV) {
  	      Serial.print (F("SRPIR pos pulse height="));
  	      Serial.print (v);
  	      Serial.print (F(" width="));
  	      Serial.println (n);
              }
            }
            break;


          // Look for the negative-going pulse. Dont wait too long.
          //
          case true:
            n= findPulse (v, -threshold, PIRGLITCH);
            if (n > 0) {
              h= false;
              trig= true;

              if (debugV) {
  	      Serial.print (F("SRPIR neg pulse height="));
  	      Serial.print (v);
  	      Serial.print (F(" width="));
  	      Serial.print (n);
                Serial.print (F(" event width"));
              Serial.println (millis() - t);
              }
            }
            if (millis() - t > PIRMAXEVENT) {
              h= false;
  
              if (debugV) {
                Serial.print (F("SRPIR no neg pulse, start over"));
              }
            }
            break;
        }
        break;

    // SINGLE PULSE MODE
    //
    case false:
      n= findPulse (v, threshold, PIRGLITCH);
      if (n > 0) {
        trig= true;

        if (debugV) {
          Serial.print (F("SRPIR pos pulse height="));
          Serial.print (v);
          Serial.print (F(" width="));
          Serial.println (n);
        }
      }
      break;
  }
  return trig;
}


private:

// Return pulse width when the pulse height exceeds the threshold, either positive or
// negative.
//
int findPulse (int h, int thresh, int width) {
static bool s = false;
static long t;
int r;

  r= 0;
  switch (s) {

    // Await leading edge.
    //
    case false:
      if (((thresh > 0) && (h >= thresh)) ||
          ((thresh < 0) && (h <= thresh))) {
        s= true;
        t= millis();
      }
      break;

    // On trailing edge, measure width and test.
    //
    case true:
      if (((thresh > 0) && (h < thresh)) ||
          ((thresh < 0) && (h > thresh))) {
        s= false;
        r= millis() - t;
        if (r < width) r= 0;
      }
  }
  return r;
}

public:

// Turn on/off debug chatter.
//
void debug (bool d) {

  debugV= d;
}


// Set PIR mode, single (0) or double (1) pulse.
//
void setMode (bool m) {

  PIRDualPulse= m;
}

// Set noise/signal threshold.
//
void setThreshold (int n) {

  threshold= n;
}


// Set PID gains.
//
void setGain (float f) {

  Sense.propGain (f);
  Sense.integGain (-f);
  Sense.diffGain (f);
}


};     // end class
#endif // SRPIR_H

