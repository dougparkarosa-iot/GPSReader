/*
TinyGPS++ - a small GPS library for Arduino providing universal NMEA parsing
Based on work by and "distanceBetween" and "courseTo" courtesy of Maarten
Lamers. Suggestion to add satellites, courseTo(), and cardinal() by Matt Monson.
Location precision improvements suggested by Wayne Holder.
Copyright (C) 2008-2013 Mikal Hart
All rights reserved.

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

#include "TinyGPS++.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define _GPRMCterm "GPRMC" // Recommended minimum specific GPS/Transit data
#define _GPGGAterm "GPGGA" // Global Positioning System Fix Data
#define _GNRMCterm "GNRMC"
#define _GNGGAterm "GNGGA"

TinyGPSPlus::TinyGPSPlus()
    : parity(0), isChecksumTerm(false), curSentenceType(GPS_SENTENCE_OTHER),
      curTermNumber(0), curTermOffset(0), sentenceHasFix(false), customElts(0),
      customCandidates(0), encodedCharCount(0), sentencesWithFixCount(0),
      failedChecksumCount(0), passedChecksumCount(0) {
  term[0] = '\0';
}

//
// public methods
//

bool TinyGPSPlus::encode(char c) {
  ++encodedCharCount;

  switch (c) {
  case ',': // term terminators
    parity ^= (uint8_t)c;
  case '\r':
  case '\n':
  case '*': {
    bool isValidSentence = false;
    if (curTermOffset < sizeof(term)) {
      term[curTermOffset] = 0;
      isValidSentence = endOfTermHandler();
    }
    ++curTermNumber;
    curTermOffset = 0;
    isChecksumTerm = c == '*';
    return isValidSentence;
  } break;

  case '$': // sentence begin
    curTermNumber = curTermOffset = 0;
    parity = 0;
    curSentenceType = GPS_SENTENCE_OTHER;
    isChecksumTerm = false;
    sentenceHasFix = false;
    return false;

  default: // ordinary characters
    if (curTermOffset < (sizeof(term) - 1))
      term[curTermOffset++] = c;
    if (!isChecksumTerm)
      parity ^= c;
    return false;
  }

  return false;
}

bool TinyGPSPlus::isUpdated() const {
  return location.isUpdated() || date.isUpdated() || time.isUpdated() ||
         speed.isUpdated() || course.isUpdated() || altitude.isUpdated() ||
         satellites.isUpdated() || hdop.isUpdated();
}

//
// internal utilities
//
int TinyGPSPlus::fromHex(char a) {
  if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else
    return a - '0';
}

// static
/// Parse a (potentially negative) number with up to 2 decimal digits -xxxx.yy
/// Result is integer value 10*the float value. For example 1234.56 is 123456
/// -1234.56 is -123456.
/// \param term text to parse
/// \return the decimal value.
int32_t TinyGPSPlus::parseDecimal(const char *term) {
  bool negative = *term == '-';
  if (negative)
    ++term;
  int32_t ret = 100 * (int32_t)atol(term);
  while (isdigit(*term))
    ++term;
  if (*term == '.' && isdigit(term[1])) {
    ret += 10 * (term[1] - '0');
    if (isdigit(term[2]))
      ret += term[2] - '0';
  }
  return negative ? -ret : ret;
}

// static

/// Parse degrees in from NMEA format DDMM.MMMM
///
/// \param term input string to parse
/// \param deg output RawDegrees struct containing parsed term
void TinyGPSPlus::parseDegrees(const char *term, RawDegrees &deg) {
  uint32_t leftOfDecimal = (uint32_t)atol(term);
  uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
  uint32_t multiplier = 10000000UL;
  uint32_t tenMillionthsOfMinutes = minutes * multiplier;

  deg.deg = (int16_t)(leftOfDecimal / 100);

  while (isdigit(*term))
    ++term;

  if (*term == '.')
    while (isdigit(*++term)) {
      multiplier /= 10;
      tenMillionthsOfMinutes += (*term - '0') * multiplier;
    }

  deg.billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
  deg.negative = false;
}

#define COMBINE(sentence_type, term_number)                                    \
  (((unsigned)(sentence_type) << 5) | term_number)

// Processes a just-completed term
// Returns true if new sentence has just passed checksum test and is validated
bool TinyGPSPlus::endOfTermHandler() {
  // If it's the checksum term, and the checksum checks out, commit
  if (isChecksumTerm) {
    byte checksum = 16 * fromHex(term[0]) + fromHex(term[1]);
    if (checksum == parity) {
      passedChecksumCount++;
      if (sentenceHasFix)
        ++sentencesWithFixCount;

      switch (curSentenceType) {
      case GPS_SENTENCE_GPRMC:
        date.commit();
        time.commit();
        if (sentenceHasFix) {
          location.commit();
          speed.commit();
          course.commit();
        }
        break;
      case GPS_SENTENCE_GPGGA:
        time.commit();
        if (sentenceHasFix) {
          location.commit();
          altitude.commit();
        }
        satellites.commit();
        hdop.commit();
        break;
      }

      // Commit all custom listeners of this sentence type
      for (TinyGPSCustom *p = customCandidates;
           p != NULL &&
           strcmp(p->sentenceName, customCandidates->sentenceName) == 0;
           p = p->next)
        p->commit();
      return true;
    }

    else {
      ++failedChecksumCount;
    }

    return false;
  }

  // the first term determines the sentence type
  if (curTermNumber == 0) {
    if (!strcmp(term, _GPRMCterm) || !strcmp(term, _GNRMCterm))
      curSentenceType = GPS_SENTENCE_GPRMC;
    else if (!strcmp(term, _GPGGAterm) || !strcmp(term, _GNGGAterm))
      curSentenceType = GPS_SENTENCE_GPGGA;
    else
      curSentenceType = GPS_SENTENCE_OTHER;

    // Any custom candidates of this sentence type?
    for (customCandidates = customElts;
         customCandidates != NULL &&
         strcmp(customCandidates->sentenceName, term) < 0;
         customCandidates = customCandidates->next)
      ;
    if (customCandidates != NULL &&
        strcmp(customCandidates->sentenceName, term) > 0)
      customCandidates = NULL;

    return false;
  }

  if (curSentenceType != GPS_SENTENCE_OTHER && term[0])
    switch (COMBINE(curSentenceType, curTermNumber)) {
    case COMBINE(GPS_SENTENCE_GPRMC, 1): // Time in both sentences
    case COMBINE(GPS_SENTENCE_GPGGA, 1):
      time.setTime(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 2): // GPRMC validity
      sentenceHasFix = term[0] == 'A';
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 3): // Latitude
    case COMBINE(GPS_SENTENCE_GPGGA, 2):
      location.setLatitude(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 4): // N/S
    case COMBINE(GPS_SENTENCE_GPGGA, 3):
      location.setLatitudeNegative(term[0] == 'S');
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 5): // Longitude
    case COMBINE(GPS_SENTENCE_GPGGA, 4):
      location.setLongitude(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 6): // E/W
    case COMBINE(GPS_SENTENCE_GPGGA, 5):
      location.setLongitudeNegative(term[0] == 'W');
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 7): // Speed (GPRMC)
      speed.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 8): // Course (GPRMC)
      course.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPRMC, 9): // Date (GPRMC)
      date.setDate(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 6): // Fix data (GPGGA)
      sentenceHasFix = term[0] > '0';
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 7): // Satellites used (GPGGA)
      satellites.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 8): // HDOP
      hdop.set(term);
      break;
    case COMBINE(GPS_SENTENCE_GPGGA, 9): // Altitude (GPGGA)
      altitude.set(term);
      break;
    }

  // Set custom values as needed
  for (TinyGPSCustom *p = customCandidates;
       p != NULL &&
       strcmp(p->sentenceName, customCandidates->sentenceName) == 0 &&
       p->termNumber <= curTermNumber;
       p = p->next)
    if (p->termNumber == curTermNumber)
      p->set(term);

  return false;
}

/* static */
double TinyGPSPlus::distanceBetween(double lat1, double long1, double lat2,
                                    double long2) {
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1 - long2);
  double sd_long = sin(delta);
  double cd_long = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double c_lat1 = cos(lat1);
  double slat2 = sin(lat2);
  double c_lat2 = cos(lat2);
  delta = (c_lat1 * slat2) - (slat1 * c_lat2 * cd_long);
  delta = sq(delta);
  delta += sq(c_lat2 * sd_long);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (c_lat1 * c_lat2 * cd_long);
  delta = atan2(delta, denom);
  return delta * 6372795;
}

double TinyGPSPlus::courseTo(double lat1, double long1, double lat2,
                             double long2) {
  // returns course in degrees (North=0, West=270) from position 1 to position
  // 2, both specified as signed decimal-degrees latitude and longitude. Because
  // Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double d_lon = radians(long2 - long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(d_lon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(d_lon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0) {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

const char *TinyGPSPlus::cardinal(double course) {
  static const char *directions[] = {"N",  "NNE", "NE", "ENE", "E",  "ESE",
                                     "SE", "SSE", "S",  "SSW", "SW", "WSW",
                                     "W",  "WNW", "NW", "NNW"};
  int direction = (int)((course + 11.25f) / 22.5f);
  return directions[direction % 16];
}

void TinyGPSLocation::commit() {
  rawLatData = rawNewLatData;
  rawLngData = rawNewLngData;
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSLocation::setLatitude(const char *term) {
  TinyGPSPlus::parseDegrees(term, rawNewLatData);
}

void TinyGPSLocation::setLongitude(const char *term) {
  TinyGPSPlus::parseDegrees(term, rawNewLngData);
}

double TinyGPSLocation::lat() {
  updated = false;
  double ret = rawLatData.deg + rawLatData.billionths / 1000000000.0;
  return rawLatData.negative ? -ret : ret;
}

double TinyGPSLocation::lng() {
  updated = false;
  double ret = rawLngData.deg + rawLngData.billionths / 1000000000.0;
  return rawLngData.negative ? -ret : ret;
}

void TinyGPSDate::commit() {
  date = newDate;
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSTime::commit() {
  time = newTime;
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSTime::setTime(const char *term) {
  newTime = (uint32_t)TinyGPSPlus::parseDecimal(term);
}

void TinyGPSDate::setDate(const char *term) { newDate = atol(term); }

uint16_t TinyGPSDate::year() {
  updated = false;
  uint16_t year = date % 100;
  return year + 2000;
}

uint8_t TinyGPSDate::month() {
  updated = false;
  return (date / 100) % 100;
}

uint8_t TinyGPSDate::day() {
  updated = false;
  return date / 10000;
}

uint8_t TinyGPSTime::hour() {
  updated = false;
  return time / 1000000;
}

uint8_t TinyGPSTime::minute() {
  updated = false;
  return (time / 10000) % 100;
}

uint8_t TinyGPSTime::second() {
  updated = false;
  return (time / 100) % 100;
}

uint8_t TinyGPSTime::centisecond() {
  updated = false;
  return time % 100;
}

void TinyGPSDecimal::commit() {
  val = newval;
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSDecimal::set(const char *term) {
  newval = TinyGPSPlus::parseDecimal(term);
}

void TinyGPSInteger::commit() {
  val = newval;
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSInteger::set(const char *term) { newval = atol(term); }

TinyGPSCustom::TinyGPSCustom(TinyGPSPlus &gps, const char *_sentenceName,
                             int _termNumber) {
  begin(gps, _sentenceName, _termNumber);
}

void TinyGPSCustom::begin(TinyGPSPlus &gps, const char *_sentenceName,
                          int _termNumber) {
  lastCommitTime = 0;
  updated = valid = false;
  sentenceName = _sentenceName;
  termNumber = _termNumber;
  memset(stagingBuffer, '\0', sizeof(stagingBuffer));
  memset(buffer, '\0', sizeof(buffer));

  // Insert this item into the GPS tree
  gps.insertCustom(this, _sentenceName, _termNumber);
}

void TinyGPSCustom::commit() {
  strcpy(this->buffer, this->stagingBuffer);
  lastCommitTime = millis();
  valid = updated = true;
}

void TinyGPSCustom::set(const char *term) {
  strncpy(this->stagingBuffer, term, sizeof(this->stagingBuffer));
}

void TinyGPSPlus::insertCustom(TinyGPSCustom *pElt, const char *sentenceName,
                               int termNumber) {
  TinyGPSCustom **pPelt;

  for (pPelt = &this->customElts; *pPelt != NULL; pPelt = &(*pPelt)->next) {
    int cmp = strcmp(sentenceName, (*pPelt)->sentenceName);
    if (cmp < 0 || (cmp == 0 && termNumber < (*pPelt)->termNumber))
      break;
  }

  pElt->next = *pPelt;
  *pPelt = pElt;
}
