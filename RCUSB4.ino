/*
 * Jaroslav Škarvada <jskarvad@redhat.com>
 * 2025/04/20, refactor, added auto calibration, EEPROM support
 *
 * kekse23.de RCUSB4 v1.1
 * Copyright (c) 2020, Nicholas Regitz
 *
 * Diese Datei ist Lizensiert unter der Creative Commons 4.0 CC BY-NC-SA
 * https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
 */

#include <EEPROM.h>
#include <Joystick.h>
#include "AVRPort23.h"

#define CHAN1 D,0
#define _INT1 0
#define CHAN2 D,1
#define _INT2 1
#define CHAN3 D,3
#define _INT3 3
#define CHAN4 D,2
#define _INT4 2
#define RXLED B,0
#define TXLED D,5

// channel logic is hardcoded, so it is not sufficient to
// change the number of channels only here
#define CHANNELS 4
#define AXISMIN 750
#define AXISMAX 2250

// number of loop cycles for the axis initial auto calibration,
// set it to the 0 to disable the inital auto calibration
#define CALTIMER 10000
// number of loop cycles to wait before performing calibration
// to avoid transient states, e.g. during radio pairing and receiver
// power on, set it to the 0 to disable the wait
#define CALWAIT 4000
// number of loop cycles for calibration LED blink (half period)
#define CALBLINK 50
// over 1/2 positive angle for auto calibration threshold
#define CALTHRESHPOS 1 / 2
// bellow 1/2 negative angle for auto calibration threshold
#define CALTHRESHNEG 1 / 2

#define CALZERO ((AXISMAX - AXISMIN) / 2 + AXISMIN)
#define CALMAXTHRESH (CALZERO + (AXISMAX - CALZERO) * CALTHRESHPOS)
#define CALMINTHRESH (CALZERO - (CALZERO - AXISMIN) * CALTHRESHNEG)

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  0, 0,                 // Button Count, Hat Switch Count
  true, true, false,    // X, Y, Z
  true, true, false,    // Rx, Ry, Rz
  false, false,         // Rudder, Throttle
  false, false, false); // Accelerator, Brake, Steering

unsigned int Caltimer = CALTIMER;
unsigned int Calwait = CALWAIT;
bool Send;
bool EECalValid;
bool EECalWrite;
unsigned int x;
volatile unsigned long Time[CHANNELS];
volatile unsigned int Value[CHANNELS];
volatile bool ValChanged[CHANNELS];
unsigned int NewValue[CHANNELS];
unsigned int AxisMax[CHANNELS];
unsigned int AxisMin[CHANNELS];
struct sEECal
{
  unsigned int AxisMax[CHANNELS];
  unsigned int AxisMin[CHANNELS];
  unsigned int ChkSum;
} EECal;

unsigned int eecalchksum(struct sEECal EECal)
{
  unsigned int ChkSum = 0;
  for (x = 0; x < CHANNELS; x++)
  {
    ChkSum += EECal.AxisMax[x] + EECal.AxisMin[x];
  }
  return ChkSum;
}

bool checkeecal(struct sEECal EECal)
{
  bool EECalValid = true;
  unsigned int ChkSum = 0;

  for (x = 0; x < CHANNELS; x++)
    if (EECal.AxisMax[x] < CALMAXTHRESH || EECal.AxisMax[x] > AXISMAX || \
      EECal.AxisMin[x] > CALMINTHRESH || EECal.AxisMin[x] < AXISMIN)
        EECalValid = false;
  if (eecalchksum(EECal) != EECal.ChkSum)
    EECalValid = false;

  return EECalValid;
}

unsigned int getaxismax(unsigned char axis)
{
  if (AxisMax[axis] > CALMAXTHRESH)
  {
    if (EECalValid)
    {
      if (AxisMax[axis] != EECal.AxisMax[axis])
        EECalWrite = true;
    }
    else
      EECalWrite = true;
    EECal.AxisMax[axis] = AxisMax[axis];
  }
  else
    EECal.AxisMax[axis] = EECalValid ? EECal.AxisMax[axis] : AXISMAX;
  return EECal.AxisMax[axis];
}

unsigned int getaxismin(unsigned char axis)
{
  if (AxisMin[axis] < CALMINTHRESH)
  {
    if (EECalValid)
    {
      if (AxisMin[axis] != EECal.AxisMin[axis])
        EECalWrite = true;
    }
    else
      EECalWrite = true;
    EECal.AxisMin[axis] = AxisMin[axis];
  }
  else
    EECal.AxisMin[axis] = EECalValid ? EECal.AxisMin[axis] : AXISMIN;
  return EECal.AxisMin[axis];
}

void setup()
{
  for (x = 0; x < CHANNELS; x++)
  {
    NewValue[x] = CALZERO;
    AxisMax[x] = CALMAXTHRESH;
    AxisMin[x] = CALMINTHRESH;
  }
  EEPROM.get(0, EECal);
  EECalValid = checkeecal(EECal);
  portMode(CHAN1, INPUT, HIGH);
  portMode(CHAN2, INPUT, HIGH);
  portMode(CHAN3, INPUT, HIGH);
  portMode(CHAN4, INPUT, HIGH);
  portMode(RXLED, OUTPUT, LOW);
  portMode(TXLED, OUTPUT, LOW);

  attachInterrupt(_INT1, isr1, CHANGE);
  attachInterrupt(_INT2, isr2, CHANGE);
  attachInterrupt(_INT3, isr3, CHANGE);
  attachInterrupt(_INT4, isr4, CHANGE);
}

void loop()
{
  Send = false;
  if (ValChanged[0])
  {
    NewValue[0] = (NewValue[0] + Value[0]) / 2;
    if (!Caltimer)
    {
      Joystick.setXAxis(NewValue[0]);
      Send = true;
    }
    ValChanged[0] = false;
  }

  if (ValChanged[1])
  {
    NewValue[1] = (NewValue[1] + Value[1]) / 2;
    if (!Caltimer)
    {
      Joystick.setYAxis(NewValue[1]);
      Send = true;
    }
    ValChanged[1] = false;
  }

  if (ValChanged[2])
  {
    NewValue[2] = (NewValue[2] + Value[2]) / 2;
    if (!Caltimer)
    {
      Joystick.setRxAxis(NewValue[2]);
      Send = true;
    }
    ValChanged[2] = false;
  }

  if (ValChanged[3])
  {
    NewValue[3] = (NewValue[3] + Value[3]) / 2;
    if (!Caltimer)
    {
      Joystick.setRyAxis(NewValue[3]);
      Send = true;
    }
    ValChanged[3] = false;
  }

  if (Calwait)
    Calwait--;
  else if (Caltimer)
  {
    for (x = 0; x < CHANNELS; x++)
    {
      if (NewValue[x] > AxisMax[x] && NewValue[x] <= AXISMAX)
        AxisMax[x] = NewValue[x];
      if (NewValue[x] < AxisMin[x] && NewValue[x] >= AXISMIN)
        AxisMin[x] = NewValue[x];
    }
    if (!(Caltimer % CALBLINK))
      portToggle(TXLED);
    Caltimer--;
    if (!Caltimer)
    {
      // auto calibration finish code
      EECalWrite = false;
      Joystick.begin(false);
      Joystick.setXAxisRange(getaxismax(0), getaxismin(0));
      Joystick.setYAxisRange(getaxismax(1), getaxismin(1));
      Joystick.setRxAxisRange(getaxismax(2), getaxismin(2));
      Joystick.setRyAxisRange(getaxismax(3), getaxismin(3));
      Joystick.setXAxis(NewValue[0]);
      Joystick.setYAxis(NewValue[1]);
      Joystick.setRxAxis(NewValue[2]);
      Joystick.setRyAxis(NewValue[3]);
      if (EECalWrite)
      {
        EECal.ChkSum = eecalchksum(EECal);
        EEPROM.put(0, EECal);
      }
      Joystick.sendState();
    }
  } else if (Send)
    Joystick.sendState();

  delay(1);
}

void isr1()
{
  if (portRead(CHAN1)) Time[0] = micros();
  else if (micros() > Time[0])
  {
    Value[0] = (Value[0] + (micros() - Time[0])) / 2;
    ValChanged[0] = true;
  }
}

void isr2()
{
  if (portRead(CHAN2)) Time[1] = micros();
  else if (micros() > Time[1])
  {
    Value[1] = (Value[1] + (micros() - Time[1])) / 2;
    ValChanged[1] = true;
  }
}

void isr3()
{
  if (portRead(CHAN3)) Time[2] = micros();
  else if (micros() > Time[2])
  {
    Value[2] = (Value[2] + (micros() - Time[2])) / 2;
    ValChanged[2] = true;
  }
}

void isr4()
{
  if (portRead(CHAN4)) Time[3] = micros();
  else if (micros() > Time[3])
  {
    Value[3] = (Value[3] + (micros() - Time[3])) / 2;
    ValChanged[3] = true;
  }
}
