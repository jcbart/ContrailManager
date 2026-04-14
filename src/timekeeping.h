#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <chrono>
#include <string>
#include <format>
#include <algorithm>

// Time interval structure
// Wrapper for a std::chrono::duration
struct CMTimeInterval {
    // Duration (seconds)
    std::chrono::duration<double> duration;

    // Empty constructor
    CMTimeInterval() {}

    // Constructor from duration
    explicit CMTimeInterval(std::chrono::duration<double> duration) : duration(duration) {}

    // Constructor from values
    CMTimeInterval(int days, int hours, int minutes, double seconds) {
        set(days, hours, minutes, seconds);
    }

    // Set from duration
    void set(std::chrono::duration<double> duration) {
        this->duration = duration;
    }

    // Set from values
    void set(int days, int hours, int minutes, double seconds) {
        duration = std::chrono::days{days}
            + std::chrono::hours{hours}
            + std::chrono::minutes(minutes)
            + std::chrono::duration<double>{seconds};
    }

    constexpr bool operator==(const CMTimeInterval& other) const {
        return duration == other.duration;
    }

    constexpr bool operator!=(const CMTimeInterval& other) const {
        return duration != other.duration;
    }

    CMTimeInterval operator+(const CMTimeInterval& other) const {
        return CMTimeInterval(duration + other.duration);
    }

    CMTimeInterval operator-(const CMTimeInterval& other) const {
        return CMTimeInterval(duration - other.duration);
    }

    CMTimeInterval& operator+=(const CMTimeInterval& other) {
        duration += other.duration;
        return *this;
    }

    double operator/(const CMTimeInterval& other) const {
        return duration/other.duration;
    }

    // Turns time interval into seconds
    constexpr double to_s() const {
        return duration.count();
    }

    // Return time interval as a string with seconds decimal places given by dp
    std::string asString() const {
        std::chrono::days d = std::chrono::duration_cast<std::chrono::days>(duration);
        std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(duration - d);
        std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(duration - d - h);
        std::chrono::duration<double> s = (duration - d - h - m);
        return std::format(
            "{}d {:02}h {:02}m {:06.3f}s",
            d.count(), h.count(), m.count(), s.count()
        );
    }
};

// Non-member operators for CMTimeInterval

inline CMTimeInterval operator*(const CMTimeInterval& timeInt, double scalar) {
    return CMTimeInterval(timeInt.duration * scalar);
}

inline CMTimeInterval operator*(double scalar, const CMTimeInterval& timeInt) {
    return timeInt * scalar;
}

// Bare CMTime structure (only data members) to be compatible with Fortran type
struct CMTime_F {
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    float s;
};

// Time structure
// Wrapper for a std::chrono::sys_time
struct CMTime {
    // Time point - uses milliseconds as precision
    std::chrono::sys_time<std::chrono::milliseconds> timepoint;

    // Empty constructor
    CMTime() {}

    // Constructor from timepoint
    template<typename Duration>
    explicit CMTime(std::chrono::sys_time<Duration> timepoint)
        : timepoint(std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint)) {}

    // Constructor from values
    CMTime(int year, int month, int day, int hours, int minutes, double seconds) {
        set(year, month, day, hours, minutes, seconds);
    }

    // Set from timepoint
    template<typename Duration>
    void set(std::chrono::sys_time<Duration> timepoint) {
        this->timepoint = std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint);
    }

    // Set from values
    void set(int year, int month, int day, int hours, int minutes, float seconds) {
        std::chrono::year_month_day ymd = std::chrono::year{year} / month / day;
        std::chrono::sys_days date_part{ymd};
        auto time_part = std::chrono::hours{hours}
            + std::chrono::minutes(minutes)
            + std::chrono::duration<double>{seconds};
        timepoint = std::chrono::time_point_cast<std::chrono::milliseconds>(date_part + time_part);
    }

    // Sets the internal variables from CMTime_F input
    void set(const CMTime_F& fortranTime) {
        set(
            fortranTime.yy,
            fortranTime.mm,
            fortranTime.dd,
            fortranTime.h,
            fortranTime.m,
            fortranTime.s
        );
    }

    bool operator==(const CMTime& other) const {
        return timepoint == other.timepoint;
    }

    bool operator!=(const CMTime& other) const {
        return timepoint != other.timepoint;
    }

    // CMTime + CMTimeInterval = CMTime
    CMTime operator+(const CMTimeInterval& interval) const {
        return CMTime(
            std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint + interval.duration)
        );
    }

    // CMTime - CMTime = CMTimeInterval
    CMTimeInterval operator-(const CMTime& other) const {
        return CMTimeInterval(timepoint - other.timepoint);
    }

    CMTime& operator+=(const CMTimeInterval& interval) {
        this->set(
            std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint + interval.duration)
        );
        return *this;
    }

    constexpr bool operator>(const CMTime& other) const {
        return timepoint > other.timepoint;
    }

    constexpr bool operator>=(const CMTime& other) const {
        return timepoint >= other.timepoint;
    }

    constexpr bool operator<(const CMTime& other) const {
        return timepoint < other.timepoint;
    }

    constexpr bool operator<=(const CMTime& other) const {
        return timepoint <= other.timepoint;
    }

    // Return time as a string with seconds decimal places given by dp
    std::string asString() const {
        auto datepoint = std::chrono::floor<std::chrono::days>(timepoint);
        std::chrono::year_month_day ymd{datepoint};
        auto time_of_day = timepoint - datepoint;
        std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(time_of_day);
        std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(time_of_day - h);
        std::chrono::duration<double> s = (time_of_day - h - m);
        return std::format(
            "{:04}-{:02}-{:02} {:02}:{:02}:{:06.3f}",
            static_cast<int>(ymd.year()),
            static_cast<unsigned>(ymd.month()),
            static_cast<unsigned>(ymd.day()),
            h.count(), m.count(), s.count()
        );
    }

    // Return a file name-friendly string (no hyphens or colons)
    std::string asFileFriendlyString() const {
        std::string str = asString();
        // Find (final) .
        auto pos = str.rfind('.');
        // Remove it and everything after
        if (pos != std::string::npos) {
            str.erase(pos);
        }
        // Replace spaces and colons
        std::replace(str.begin(), str.end(), ' ', '_');
        std::replace(str.begin(), str.end(), ':', '-');
        return str;
    }
};

#endif