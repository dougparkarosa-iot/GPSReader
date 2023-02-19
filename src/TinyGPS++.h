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

#ifndef __TinyGPSPlus_h
#define __TinyGPSPlus_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//#include "WProgram.h"
#endif
#include <limits.h>

#define _GPS_VERSION "2.0.0-a1" // software version of this library
#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399
#define _GPS_MAX_FIELD_SIZE 15

/// \brief stuct for NMEA format degrees
/// Struct to hold degrees in the National Marine Electronics Association (NMEA)
/// format
///
/// see parseDegrees()
struct RawDegrees {
  uint16_t deg;        ///< Degrees
  uint32_t billionths; ///< billionths of minutes
  bool negative;       ///< true if negative false otherwise

public:
  /// Constructor
  RawDegrees() : deg(0), billionths(0), negative(false) {}
};

/// \brief GPS Location
class TinyGPSLocation {

public:
  /// Query if the location data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Query if the location data has been updated
  /// \return true if valid false otherwise.
  bool isUpdated() const { return updated; }

  /// Age of the location in miliseconds.
  ///
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Get the raw latitude
  /// Marks the data as not updated.
  /// \return the latitude
  const RawDegrees &rawLat() {
    updated = false;
    return rawLatData;
  }

  /// Get the raw longitude
  /// Marks the data as not updated.
  /// \return the longitude
  const RawDegrees &rawLng() {
    updated = false;
    return rawLngData;
  }

  /// Get the latitude
  /// Marks the data as not updated.
  /// \return the latitude
  double lat();

  /// Get the longitude
  /// Marks the data as not updated.
  /// \return the longitude
  double lng();

  /// Constructor
  TinyGPSLocation()
      : valid(false), updated(false), rawLatData(), rawLngData(),
        rawNewLatData(), rawNewLngData(), lastCommitTime() {}

  /// Commit changes
  void commit();

  /// Set the latitude by parsing the input string
  /// \param term string containing the latitude
  void setLatitude(const char *term);

  /// Change the sign of the latitude data.
  /// \param negative true if negative false if positive
  void setLatitudeNegative(bool negative) { rawNewLatData.negative = negative; }

  /// Set the longitude by parsing the input string
  /// \param term string containing the longitude
  void setLongitude(const char *term);

  /// Change the sign of the longitude data.
  /// \param negative true if negative false if positive
  void setLongitudeNegative(bool negative) {
    rawNewLngData.negative = negative;
  }

private:
  bool valid, updated;
  RawDegrees rawLatData, rawLngData, rawNewLatData, rawNewLngData;
  uint32_t lastCommitTime;
};

/// \brief Class to hold GPS date
class TinyGPSDate {
  friend class TinyGPSPlus;

public:
  /// Query if the date data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Query if the date data is updated
  /// \return true if updated false otherwise.
  bool isUpdated() const { return updated; }

  /// Get the age of the date in miliseconds.
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Access the date value and mark it as no longer
  /// updated.
  /// \return date
  uint32_t value() {
    updated = false;
    return date;
  }

  /// Extract the year and mark it as no longer updated.
  /// \return year
  uint16_t year();

  /// Extract the month and mark it as no longer updated
  /// \return month
  uint8_t month();

  /// Extract day of month and mark it as no longer updated.
  /// \return day of month
  uint8_t day();

  /// Constructor
  TinyGPSDate()
      : valid(false), updated(false), date(0), newDate(), lastCommitTime() {}

private:
  bool valid, updated;
  uint32_t date, newDate;
  uint32_t lastCommitTime;
  void commit();
  void setDate(const char *term);
};

/// \brief Class to hold GPS time
struct TinyGPSTime {
  friend class TinyGPSPlus;

public:
  /// Query if the time data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Query if the time data has been updated.
  /// \return true if updated false otherwise.
  bool isUpdated() const { return updated; }

  /// Get the age of the time data in milliseconds
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Get the integral value storing the time and mark it as not updated.
  /// \return the time.
  uint32_t value() {
    updated = false;
    return time;
  }

  /// Get the hour and mark it as not updated.
  /// \return the hour.
  uint8_t hour();

  /// Get the minute and mark it as not updated.
  /// \return the minute
  uint8_t minute();

  /// Get the second and mark it as not updated.
  /// \return the second.
  uint8_t second();

  /// Get hundredths of a second and mark as not updated.
  /// \return hundredths of seconds.
  uint8_t centisecond();

  /// Constructor
  TinyGPSTime()
      : valid(false), updated(false), time(0), newTime(), lastCommitTime() {}

private:
  bool valid, updated;
  uint32_t time, newTime;
  uint32_t lastCommitTime;
  void commit();
  void setTime(const char *term);
};

/// \brief Class to hold GPS decimal value
///
/// integer value 10*the float value. For example 1234.56 is 123456
/// -1234.56 is -123456.
class TinyGPSDecimal {
  // friend class TinyGPSPlus;

public:
  /// Query if the decimal data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Query if the decimal data has been updated.
  /// \return true if data has been updated false otherwise.
  bool isUpdated() const { return updated; }

  /// Get the age of the decimal data in milliseconds
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Get the value of the decimal data and mark it as not updated.
  /// \return the decimal value.
  int32_t value() {
    updated = false;
    return val;
  }

  /// Constructor
  TinyGPSDecimal()
      : valid(false), updated(false), lastCommitTime(), val(0), newval() {}

  /// Commit changes
  void commit();

  /// Set the data from input string.
  /// \param term the input string
  /// uses TinyGPSPlus::parseDecimal to interpret term
  void set(const char *term);

private:
  bool valid, updated;
  uint32_t lastCommitTime;
  int32_t val, newval;
};

/// \brief Class to hold a 32 bit integer value
class TinyGPSInteger {
public:
  /// Query if the data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Query if the data has been updated.
  /// \return true if data has been updated false otherwise.
  bool isUpdated() const { return updated; }

  /// Get the age of the data in milliseconds
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Get the value of the data and mark it as not updated.
  /// \return the decimal value.
  uint32_t value() {
    updated = false;
    return val;
  }

  /// Constructor
  TinyGPSInteger()
      : valid(false), updated(false), lastCommitTime(), val(0), newval() {}

  /// Commit changes
  void commit();

  /// Set the data from input string.
  /// \param term the input string
  /// uses atol to interpret term
  void set(const char *term);

private:
  bool valid, updated;
  uint32_t lastCommitTime;
  uint32_t val, newval;
};

/// \brief Class to hold GPS speed value
class TinyGPSSpeed : public TinyGPSDecimal {
public:
  /// Return speed in knots.
  /// \return speed in knots.
  double knots() { return value() / 100.0; }

  /// Return speed in miles per hour.
  /// \return speed in mph.
  double mph() { return _GPS_MPH_PER_KNOT * value() / 100.0; }

  /// Return speed in meters per second.
  /// \return speed in meters per second.
  double mps() { return _GPS_MPS_PER_KNOT * value() / 100.0; }

  /// Return speed in kilometers per hour.
  /// \return speed in kilometers per hour.
  double kmph() { return _GPS_KMPH_PER_KNOT * value() / 100.0; }
};

/// \brief Class to hold GPS course 
/// Course is degrees relative to north 0 is north, clockwise through 360.
class TinyGPSCourse : public TinyGPSDecimal {
public:
  double deg() { return value() / 100.0; }
};

/// \brief Class to hold GPS altitude value
struct TinyGPSAltitude : TinyGPSDecimal {

  /// Get altitude in meters.
  /// marks data as not updated.
  ///
  /// \return altitude in meters
  double meters() { return value() / 100.0; }

  /// Get altitude in miles.
  /// marks data as not updated.
  ///
  /// \return altitude in miles.
  double miles() { return _GPS_MILES_PER_METER * value() / 100.0; }

  /// Get altitude in kilometers.
  /// marks data as not updated.
  ///
  /// \return altitude in kilometers
  double kilometers() { return _GPS_KM_PER_METER * value() / 100.0; }

  /// Get altitude in feet.
  /// marks data as not updated.
  ///
  /// \return altitude in feet.
  double feet() { return _GPS_FEET_PER_METER * value() / 100.0; }
};

/// \brief Class to hold Horizontal dilution of precision (HDOP)
///
/// This is a GPS Decimal value
class TinyGPSHDOP : public TinyGPSDecimal {
public:
  /// Get the HDOP value and mark as not updated.
  /// \return HDOP value.
  double hdop() { return value() / 100.0; }
};

class TinyGPSPlus;

/// \brief Class to allow parsing of custom fields
class TinyGPSCustom {
public:
  /// Constructor
  TinyGPSCustom(){};

  /// Constructor
  /// \param gps the TinyGPSPlus class to do parsing.
  /// \param sentenceName the name of the sentence for example "GPGSA"
  /// \param termNumber the number of the term to parse.
  /// Refer to the UsingCustomFields.ino sketch for more information
  TinyGPSCustom(TinyGPSPlus &gps, const char *sentenceName, int termNumber);

  /// Start parsing. This is called by the similar constructor and is useful
  /// if a default constructed TinyGPSCustom is used.
  /// \param gps the TinyGPSPlus class to do parsing.
  /// \param _sentenceName the name of the sentence for example "GPGSA"
  /// \param _termNumber the number of the term to parse.
  void begin(TinyGPSPlus &gps, const char *_sentenceName, int _termNumber);

  /// Query if the data has been updated.
  /// \return true if data has been updated false otherwise.
  bool isUpdated() const { return updated; }

  /// Query if the data is valid.
  /// \return true if valid false otherwise.
  bool isValid() const { return valid; }

  /// Get the age of the data in milliseconds
  /// \return age in milliseconds if valid. ULONG_MAX otherwise.
  uint32_t age() const {
    return valid ? millis() - lastCommitTime : (uint32_t)ULONG_MAX;
  }

  /// Get the value of the custom data and mark it as not updated.
  /// \return the custom data as a string.
  const char *value() {
    updated = false;
    return buffer;
  }

private:
  void commit();
  void set(const char *term);

  char stagingBuffer[_GPS_MAX_FIELD_SIZE + 1];
  char buffer[_GPS_MAX_FIELD_SIZE + 1];
  unsigned long lastCommitTime;
  bool valid, updated;
  const char *sentenceName;
  int termNumber;
  friend class TinyGPSPlus;
  TinyGPSCustom *next;
};

/// \brief Class to parse NMEA GPS sentences and access the results
class TinyGPSPlus {
public:
  /// Constructor
  TinyGPSPlus();

  /// Process one character received from GPS
  /// \param c input character
  /// \return true is sentence parsed so far is valid false otherwise.
  bool encode(char c); // process one character received from GPS

  /// Check to see if any data has been updated.
  ///
  /// \return true if any of location, date, time, speed, course, altitude,
  /// satellites or hdop have changed, false otherwise.
  bool isUpdated() const;

  /// operator version that wraps call to encode
  /// \param c input character
  /// \return TinyGPSPlus reference to this class.
  TinyGPSPlus &operator<<(char c) {
    encode(c);
    return *this;
  }

  TinyGPSLocation location;  ///< location
  TinyGPSDate date;          ///< date
  TinyGPSTime time;          ///< time
  TinyGPSSpeed speed;        ///< speed
  TinyGPSCourse course;      ///< course
  TinyGPSAltitude altitude;  ///< altitude
  TinyGPSInteger satellites; ///< stellites
  TinyGPSHDOP hdop;          ///< HDOP

  /// static Get library version
  /// \return string containing library version
  static const char *libraryVersion() { return _GPS_VERSION; }

  /// returns distance in meters between two positions, both specified
  /// as signed decimal-degrees latitude and longitude. Uses great-circle
  /// distance computation for hypothetical sphere of radius 6372795 meters.
  /// Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  /// Courtesy of Maarten Lamers.
  /// \param lat1 first latitude value.
  /// \param long1 first longitude value.
  /// \param lat2 second latitude value.
  /// \param long2 second longitude value.
  /// \return distance in meters
  static double distanceBetween(double lat1, double long1, double lat2,
                                double long2);

  /// returns course in degrees (North=0, West=270) from position 1 to position
  /// 2, both specified as signed decimal-degrees latitude and longitude.
  /// Because Earth is no exact sphere, calculated course may be off by a tiny
  /// fraction. Courtesy of Maarten Lamers 
  /// \param lat1 first latitude value.
  /// \param long1 first longitude value.
  /// \param lat2 second latitude value.
  /// \param long2 second longitude value.
  /// \return course in degrees.
  static double courseTo(double lat1, double long1, double lat2, double long2);

  /// Get cardinal direction from input course.
  /// \param course course to use to get cardinal direction
  /// \return cardinal direction. One of
  /// "N",  "NNE", "NE", "ENE", "E",  "ESE",
  /// "SE", "SSE", "S",  "SSW", "SW", "WSW",
  /// "W",  "WNW", "NW", "NNW"
  static const char *cardinal(double course);

  static int32_t parseDecimal(const char *term);
  static void parseDegrees(const char *term, RawDegrees &deg);

  /// Get number of characters processed so far.
  /// \return number of encoded characters
  uint32_t charsProcessed() const { return encodedCharCount; }

  /// Number of sentences with a GPS fix.
  /// \return number of GPS fixes
  uint32_t sentencesWithFix() const { return sentencesWithFixCount; }

  /// Number of failed checksums.
  /// \return count of failed checksums.
  uint32_t failedChecksum() const { return failedChecksumCount; }

  /// Number of good checksums
  /// \return count of passed checksums.
  uint32_t passedChecksum() const { return passedChecksumCount; }

private:
  enum { GPS_SENTENCE_GPGGA, GPS_SENTENCE_GPRMC, GPS_SENTENCE_OTHER };

  // parsing state variables
  uint8_t parity;
  bool isChecksumTerm;
  char term[_GPS_MAX_FIELD_SIZE];
  uint8_t curSentenceType;
  uint8_t curTermNumber;
  uint8_t curTermOffset;
  bool sentenceHasFix;

  // custom element support
  friend class TinyGPSCustom;
  TinyGPSCustom *customElts;
  TinyGPSCustom *customCandidates;
  void insertCustom(TinyGPSCustom *pElt, const char *sentenceName, int index);

  // statistics
  uint32_t encodedCharCount;
  uint32_t sentencesWithFixCount;
  uint32_t failedChecksumCount;
  uint32_t passedChecksumCount;

  // internal utilities
  int fromHex(char a);
  bool endOfTermHandler();
};

#endif // def(__TinyGPSPlus_h)
