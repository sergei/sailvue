#ifndef SAILVUE_UTCTIME_H
#define SAILVUE_UTCTIME_H


#include <cstdint>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>
#include "Quantity.h"

class UtcTime : public Quantity {
public:
    static UtcTime INVALID;
    explicit UtcTime(): Quantity(false, 0) {};
    // We ignore the time stamp when check validity of UTC time itself
    [[nodiscard]] bool isValid(uint64_t ) const override { return m_bValid; }

    static UtcTime fromUnixTimeMs(uint64_t ms) {
        return UtcTime(ms);
    }
    explicit operator std::string() const {
        std::stringstream ss;
        if( isValid(m_uiUnixMilliSecs))
            ss << m_uiUnixMilliSecs;
        else
            ss << "";
        return ss.str();
    }
    [[nodiscard]] uint64_t getUnixTimeMs() const {
        return m_uiUnixMilliSecs;
    }
  /**
   * @brief Creates a UtcTime object from a time string with automatic timezone detection
   *
   * Parses a time string and converts it to UTC, automatically detecting and handling
   * various timezone formats including named timezones (PDT, PST) and ISO 8601
   * timezone offsets (+HH:MM, -HH:MM).
   *
   * @param timeStr The time string to parse. Supports formats:
   *                - ISO 8601 with timezone offset: "2025-08-17T09:55:00+07:00"
   *                - ISO 8601 with named timezone: "2025-08-17T09:55:00 PDT"
   *                - ISO 8601 without timezone: "2025-08-17T09:55:00" (treated as local time)
   *
   * @return UtcTime object representing the parsed time in UTC, or UtcTime::INVALID if parsing fails
   *
   * @note Supported named timezones:
   *       - PDT (Pacific Daylight Time): UTC-7
   *       - PST (Pacific Standard Time): UTC-8
   *
   * @note For timezone offsets, the method correctly handles both positive and negative
   *       offsets and converts them to UTC by applying the opposite offset.
   *
   * @example
   * @code
   * UtcTime time1 = UtcTime::fromString("2025-08-17T09:55:00 PDT");
   * UtcTime time2 = UtcTime::fromString("2025-08-17T09:55:00-07:00");
   * UtcTime time3 = UtcTime::fromString("2025-08-17T09:55:00");
   * @endcode
   */
  static UtcTime fromString(const std::string& timeStr) {
      std::tm tm = {};
      std::string timezone;
      std::istringstream ss;
      bool parseSuccess = false;

      // Try to parse with timezone suffix
      std::size_t tzPos = timeStr.find_last_of("+-");
      if (tzPos != std::string::npos && tzPos > 10) {
          // Has timezone offset like "+07:00" or "-08:00"
          timezone = timeStr.substr(tzPos);
          std::string timeOnly = timeStr.substr(0, tzPos);
          ss.str(timeOnly);
          ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
          parseSuccess = !ss.fail();
      } else {
          // Check for named timezone suffixes
          if (timeStr.find("PDT") != std::string::npos) {
              timezone = "PDT";
              std::string timeOnly = timeStr.substr(0, timeStr.find(" PDT"));
              ss.str(timeOnly);
              ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
              parseSuccess = !ss.fail();
          } else if (timeStr.find("PST") != std::string::npos) {
              timezone = "PST";
              std::string timeOnly = timeStr.substr(0, timeStr.find(" PST"));
              ss.str(timeOnly);
              ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
              parseSuccess = !ss.fail();
          } else {
              // Assume local time
              ss.str(timeStr);
              ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
              timezone = "LOCAL";
              parseSuccess = !ss.fail();
          }
      }

      if (!parseSuccess) {
          return UtcTime::INVALID;
      }

      auto tp = std::chrono::system_clock::from_time_t(timegm(&tm));

      // Convert to UTC based on detected timezone
      if (timezone == "PDT") {
          tp += std::chrono::hours(7);  // PDT is UTC-7
      } else if (timezone == "PST") {
          tp += std::chrono::hours(8);  // PST is UTC-8
      } else if (!timezone.empty() && (timezone[0] == '+' || timezone[0] == '-')) {
          // Parse offset like "+07:00" or "-08:00"
          int sign = (timezone[0] == '+') ? -1 : 1;  // Opposite sign for conversion to UTC
          int hours = std::stoi(timezone.substr(1, 2));
          int minutes = std::stoi(timezone.substr(4, 2));
          tp += std::chrono::hours(sign * hours) + std::chrono::minutes(sign * minutes);
      }

      auto unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
          tp.time_since_epoch()).count();

      return UtcTime::fromUnixTimeMs(unix_ms);
  }

private:
    explicit UtcTime(uint64_t ms):Quantity(true, ms) {
        m_uiUnixMilliSecs = ms;
    }

    uint64_t m_uiUnixMilliSecs=0;

};


#endif //SAILVUE_UTCTIME_H

