#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <string>
#include <sstream>

constexpr int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int DAYS_IN_MONTH_LEAP[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Time interval structure
// Time intervals may be negative
// Time intervals can have day and month equal to 0
// Given that years have varying numbers of days, their value is always relative to
// a CMTime
struct CMTimeInterval {
private:
    void adjust();

public:
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    float s;

    void set(int yy, int mm, int dd, int h, int m, float s);

    bool operator==(const CMTimeInterval& other) const;

    bool operator!=(const CMTimeInterval& other) const;

    CMTimeInterval operator+(const CMTimeInterval& other) const;

    CMTimeInterval operator-(const CMTimeInterval& other) const;

    CMTimeInterval& operator+=(const CMTimeInterval& other);

    // Turns days, hours, minutes, and seconds into seconds
    constexpr double dhms_to_s() const {
        return (dd*86400. + h*3600. + m*60. + s);
    }

    // Return time as a string
    inline std::string asString() const {
        std::stringstream ss;
        ss << yy;
        ss << "-";
        (mm < 10) ? ss << "0" << mm : ss << mm;
        ss << "-";
        (dd < 10) ? ss << "0" << dd : ss << dd;
        ss << " ";
        (h < 10) ? ss << "0" << h : ss << h;
        ss << ":";
        (m < 10) ? ss << "0" << m : ss << m;
        ss << ":";
        (s < 10) ? ss << "0" << s : ss << s;
        return ss.str();
    }
};

CMTimeInterval operator*(const CMTimeInterval& timeInt, float scalar);

CMTimeInterval operator*(float scalar, const CMTimeInterval& timeInt);

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
// Times cannot have day or month equal to 0
struct CMTime {
private:
    void adjust();

public:
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    float s;

    // Returns true if internal year is a leap year
    inline bool isLeap() const {
        if (yy % 4 == 0) {
            if (yy % 100 == 0) {
                return (yy % 400 == 0);
            }
            return true;
        }
        return false;
    }

    void set(int yy, int mm, int dd, int h, int m, float s);

    void set(const CMTime_F& fortranTime);

    bool operator==(const CMTime& other) const;

    bool operator!=(const CMTime& other) const;

    CMTime operator+(const CMTimeInterval& other) const;

    CMTimeInterval operator-(const CMTime& other) const;

    CMTime& operator+=(const CMTimeInterval& other);

    bool operator>(const CMTime& other) const;

    bool operator<(const CMTime& other) const;

    bool operator<=(const CMTime& other) const;

    bool operator>=(const CMTime& other) const;

    // Turns days, hours, minutes, and seconds into seconds
    constexpr double dhms_to_s() const {
        return (dd*86400. + h*3600. + m*60. + s);
    }

    // Return time as a string
    inline std::string asString() const {
        std::stringstream ss;
        ss << yy;
        ss << "-";
        (mm < 10) ? ss << "0" << mm : ss << mm;
        ss << "-";
        (dd < 10) ? ss << "0" << dd : ss << dd;
        ss << " ";
        (h < 10) ? ss << "0" << h : ss << h;
        ss << ":";
        (m < 10) ? ss << "0" << m : ss << m;
        ss << ":";
        (s < 10) ? ss << "0" << s : ss << s;
        return ss.str();
    }
};

#endif