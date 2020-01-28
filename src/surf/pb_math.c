// DATEINAME        : pb_math.c
// ERSTELLUNGSDATUM : 20.09.93
// COPYRIGHT (C) 1993: ATLAS ELEKTRONIK GMBH, 28305 BREMEN
//
// See README file for copying and redistribution conditions.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xdr_surf.h"
#include "mem_surf.h"
#include "util_surf.h"
#include "pb_math.h"

#define HALF_PI (double)(3.14159265359/2.0)

double pbAtan2(double y, double x) {
  if (fabs(x) > 0.0)
    return atan2(y, x);

  if (y < 0.0)
    return -HALF_PI;

  return HALF_PI;
}

double setToPlusMinusPI(double angle) {
  if (angle > PI) {
    angle = setToPlusMinusPI(angle - (2.0*PI));
  }
  if(angle < -PI) {
    angle = setToPlusMinusPI(angle + (2.0*PI));
  }
 return angle;
}

void rotateCoordinates(
    double rotAngle, XY_Coords* origCoords, XY_Coords* targetCoords) {
  const double angle = setToPlusMinusPI(rotAngle);
  targetCoords->x = origCoords->x * cos(angle) + origCoords->y * sin(angle);
  targetCoords->y = -(origCoords->x * sin(angle)) + origCoords->y * cos(angle);
}

void xyToRhoPhi(
    double x0, double y0, double pointX, double pointY,
    double *rho, double *phi) {
  const double x = pointX - x0;
  const double y = pointY - y0;
  *rho = sqrt((x*x) + (y*y));
  const double angle = pbAtan2(x,y);
  if(angle < 0.0)
    *phi = angle + (2.0 * PI);
  else
    *phi = angle;
}

void lambdaPhiToRhoPhi(
    double x0, double y0, double pointX, double pointY,
    double *rho,double *phi) {
  double x = pointX - x0;
  double y = pointY - y0;
  x = RAD_TO_METER_X(x,y0);
  y = RAD_TO_METER_Y(y);
  *rho = sqrt((x*x) + (y*y));
  const double angle = pbAtan2(x,y);
  if(angle < 0.0)
    *phi = angle + (2.0 * PI);
  else
    *phi = angle;
}

Boolean signf(double value) {
  return value >= 0.0;
}

Boolean signsh(short value) {
  return value >= 0;
}

// Berechnung von Tiefe und Ablagen aus Traveltime
Boolean depthFromTT(FanParam* fanParam, Boolean isPitchcompensated) {
  // aktuellen Winkel Aufgrund cmean rechnen
  if(fanParam->cmean == 0.0 || fanParam->ckeel == 0.0 ||
     fanParam->travelTime == 0.0)
    return False;

  const double arg = (fanParam->cmean / fanParam->ckeel) * sin(fanParam->angle);

  if (arg >= 1.0 || arg <= -1.0)
     return False;

  const double alpha = asin(arg);

  // Winkelparameter

  double tanAl  = tan(alpha);
  double tan2Al = tanAl * tanAl;

  // TODO(schwehr): Simplify
  double tanP;
  double tan2P;
  if (!isPitchcompensated) {
    tanP  = tan(fanParam->pitchTx);
    tan2P = tanP * tanP;
  } else {
    tanP  = 0.0;
    tan2P = 0.0;
  }

  // mittleren rueckgestreuten Weg auf Bezugsniveau HubRx rechnen
  double travelWay = fanParam->travelTime * fanParam->cmean;
  travelWay = travelWay
            - (((fanParam->heaveTx - fanParam->heaveRx)/2.0) * cos(alpha));

  // Pitchkompensierte Tiefe ohne Huboffset (3D-Pythagoras)

  const double depth = travelWay / sqrt(tan2Al + tan2P + 1);


  // Positionen
  fanParam->posAhead = (depth * tanP)  + fanParam->transducerOffsetAhead;
  fanParam->posStar  = (depth * tanAl) + fanParam->transducerOffsetStar;


  // Tiefe rechnen
  fanParam->depth = depth              // Pitch-kompensierte Messtiefe
                  - fanParam->heaveRx  // absolute Hubkompensation
                  + fanParam->draught; // Tiefgang

  return True;
}

// Berechnung von Traveltime und Ablagen aus Depth
Boolean TTfromDepth(FanParam *fanParam, Boolean isPitchcompensated) {
  /* aktuellen Winkel Aufgrund cmean rechnen */

  if(fanParam->cmean == 0.0 || fanParam->ckeel == 0.0 ||
     fanParam->travelTime == 0.0)
    return False;

  const double arg = (fanParam->cmean / fanParam->ckeel) * sin(fanParam->angle);
  if (arg >= 1.0 || arg <= -1.0)
     return False;

  const double alpha = asin(arg);

 /* Winkelparameter */

  const double tanAl = tan(alpha);
  const double tan2Al = tanAl * tanAl;

  // TODO(schwehr): Simplify
  double tanP;
  double tan2P;
  if (!isPitchcompensated) {
    tanP  = tan(fanParam->pitchTx);
    tan2P = tanP * tanP;
  } else {
    tanP  = 0.0;
    tan2P = 0.0;
  }

  // Tiefe auf HubRx-Niveau rechnen

  double depth = fanParam->depth       // Pitch-kompensierte Messtiefe
                 + fanParam->heaveRx   // absolute Hubkompensation
                 - fanParam->draught;  // Tiefgang

  // Travelway auf HubRx-Niveau (3D-Pythagoras)

  double travelWay = depth * sqrt(tan2Al + tan2P + 1);

  // mittleren rueckgestreuten Weg auf Bezugsniveau HubRx rechnen */

  travelWay = travelWay
            + (((fanParam->heaveTx - fanParam->heaveRx)/2.0) * cos(alpha/2.0));
  fanParam->travelTime = travelWay / fanParam->cmean;

 /* Positionen */
  fanParam->posAhead = (depth * tanP)  + fanParam->transducerOffsetAhead;
  fanParam->posStar  = (depth * tanAl) + fanParam->transducerOffsetStar;

  return True;
}


// Berechnung von Draught aus Ablage und Depth

Boolean draughtFromDepth(FanParam *fanParam) {
  // aktuellen Winkel aufgrund cmean rechnen */
  if(fanParam->cmean == 0.0 || fanParam->ckeel == 0.0) return False;

  const double arg = (fanParam->cmean / fanParam->ckeel) * sin(fanParam->angle);

  if(arg >= 1.0 || arg <= -1.0)
    return False;

  const double alpha = asin(arg);

  // Faechertiefe
  double fanDepth =
   fabs(fanParam->posStar - fanParam->transducerOffsetStar)/tan(fabs(alpha));

  fanParam->draught = fanParam->depth - fanDepth;
  return True;
}

// Berechnung von Heave aus Ablage,Tiefgang und Depth
Boolean heaveFromDepth(FanParam *fanParam) {
  // aktuellen Winkel aufgrund cmean rechnen
  if(fanParam->cmean == 0.0 || fanParam->ckeel == 0.0) return False;

  const double arg = (fanParam->cmean / fanParam->ckeel) * sin(fanParam->angle);
  if(arg >= 1.0 || arg <= -1.0)
    return False;

  const double alpha = asin(arg);

  /* Faechertiefe */
  const double fanDepth = (fanParam->posStar - fanParam->transducerOffsetStar)/tan(alpha);
  fanParam->heaveTx = - (fanParam->depth - fanDepth - fanParam->draught);
  fanParam->heaveRx = fanParam->heaveTx;

  return True;
}

// nach Del Grosso: ...
double cMeanToTemperature(double salinity, double cMean) {
  const double k0 = -(1448.6 + (1.15*(salinity-35.0)) - cMean);
  const double k1 = -4.618;
  const double k2 = 0.0523;

  const double arg = k1 * k1 - 4.0 * k0 * k2;
  if(arg < 0) return 0.0; /* nur komplexe Loesungen */

  const double t1 = (-k1 + sqrt(arg))/(2.0 * k2);
  const double t2 = (-k1 - sqrt(arg))/(2.0 * k2);
  if (t2 >= 0.0 && t2 < 60.0)
    return t2;
  if (t1 >= 0.0 && t1 < 60.0)
    return t1;

  return 0.0;
}

double temperatureToCMean(double salinity, double temperature) {
  return
      1448.6
      + (temperature * 4.618)
      - (temperature * temperature * 0.0523)
      + (1.15 * (salinity - 35.0));
}

double temperatureToCMeanDelGrosso(double salinity, double temperature) {
  return
      1448.6
      + (temperature * 4.618)
      - (temperature * temperature * 0.0523)
      + (1.15 * (salinity - 35.0));
}

double temperatureToCMeanMedwin(double salinity, double temperature) {
  return
      1449.2
      + (temperature * 4.6)
      - (temperature * temperature * 0.055)
      + (temperature * temperature * temperature*0.00029)
      + ((1.34 - 0.01 * temperature) * (salinity - 35.0));
}

// Time changes

SurfTime surfTimeOfDayFromAbsTime(SurfTime absTime) {
  const double HOURS_PER_DAY = 3600.0 * 24.0;

  while (absTime > (1000.0 * HOURS_PER_DAY))
    absTime = absTime - (1000.0 * HOURS_PER_DAY);
  while (absTime > (100.0 * HOURS_PER_DAY))
    absTime = absTime - (100.0 * HOURS_PER_DAY);
  while (absTime > (10.0 * HOURS_PER_DAY))
    absTime = absTime - (10.0 * HOURS_PER_DAY);
  while (absTime > (HOURS_PER_DAY))
    absTime = absTime - (HOURS_PER_DAY);
  return absTime;
}

void timeFromRelTime(SurfTime relTime, char *buffer) {
  short day = 0;
  while (relTime >= (24.0*3600.0)) {
    relTime = relTime - (24.0*3600.0);
    day++;
  }

  short hour = 0;
  while (relTime >= 3600.0) {
    relTime = relTime - 3600.0;
    hour++;
  }

  short min = 0;
  while (relTime >= 60.0) {
    relTime = relTime - 60.0;
    min++;
  }

  short sec = 0;
  while (relTime > 0.0) {
    relTime = relTime - 1.0;
    sec++;
  }

  if(sec >= 60) {
    min++;
    sec = 0;
  }
  if(min >= 60) {
    hour++;
    min = 0;
  }
  if(hour >= 24) {
    day++;
    hour = 0;
  }

  sprintf(buffer, "%02d:%02d:%02d", hour, min, sec);
}


Boolean relTimeFromTime(char *buffer, SurfTime *relTime) {
  if (strlen(buffer) > 8) buffer[8] = 0;
  if (strlen(buffer) != 8) return False;
  if (buffer[2] != ':' || buffer[5] != ':') return False;

  int hour;
  int min;
  int sec;
  const int nr = sscanf(buffer, "%2d:%2d:%2d", &hour, &min, &sec);
  if (nr != 3) return False;

  *relTime = hour * 3600.0 + min * 60.0 + sec;
  return True;
}


