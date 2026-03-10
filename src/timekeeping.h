#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <string>
#include <sstream>

constexpr int DAYS_IN_MONTH_NO_LEAP[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Forward declarations
struct CMTime;
constexpr long long fullDaysBetween(const CMTime& time1, const CMTime& time2);

// Returns true if supplied year is a leap year
constexpr bool isLeap(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// Returns number of days in month (with January = 1) accounting for leap days
constexpr int daysInMonth(int year, int month) {
    if (month == 2) {
        return isLeap(year) ? DAYS_IN_MONTH_NO_LEAP[1] + 1 : DAYS_IN_MONTH_NO_LEAP[1];
    }
    return DAYS_IN_MONTH_NO_LEAP[month - 1];
}

// Returns number of days in year accounting for leap days
constexpr int daysInYear(int year) {
    return isLeap(year) ? 366 : 365;
}

// Time interval structure
// Does not contain month or year members, but days can be arbitrarily large
// Time intervals may be negative
struct CMTimeInterval {
private:
    // Adjust values within their limits
    void adjust() {
        while (s >= 60) {
            s -= 60;
            m += 1;
        }
        while (m > 0 && s < 0) {
            s += 60;
            m -= 1;
        }
        while (m >= 60) {
            m -= 60;
            h += 1;
        }
        while (h > 0 && m < 0) {
            m += 60;
            h -= 1;
        }
        while (h >= 24) {
            h -= 24;
            dd += 1;
        }
        while (dd > 0 && h < 0) {
            h += 24;
            dd -= 1;
        }
    }

public:
    int dd;
    int h;
    int m;
    float s;

    // Constructor without values
    CMTimeInterval() {}

    // Constructor with values
    CMTimeInterval(int dd, int h, int m, float s)
        : dd(dd), h(h), m(m), s(s) {
        
        adjust();
    }

    // Sets the internal variables
    void set(int dd, int h, int m, float s) {
        this->dd = dd;
        this->h = h;
        this->m = m;
        this->s = s;
        adjust();
    }

    constexpr bool operator==(const CMTimeInterval& other) const {
        return (this->dd == other.dd &&
                this->h == other.h &&
                this->m == other.m &&
                this->s == other.s);
    }

    constexpr bool operator!=(const CMTimeInterval& other) const {
        return !(operator==(other));
    }

    CMTimeInterval operator+(const CMTimeInterval& other) const {
        return CMTimeInterval(
            this->dd + other.dd,
            this->h + other.h,
            this->m + other.m,
            this->s + other.s
        );
    }

    CMTimeInterval operator-(const CMTimeInterval& other) const {
        return CMTimeInterval(
            this->dd - other.dd,
            this->h - other.h,
            this->m - other.m,
            this->s - other.s
        );
    }

    CMTimeInterval& operator+=(const CMTimeInterval& other) {
        this->set(this->dd + other.dd,
                  this->h + other.h,
                  this->m + other.m,
                  this->s + other.s);
        return *this;
    }

    // Turns time interval into seconds
    constexpr double to_s() const {
        return (dd*86400. + h*3600. + m*60. + s);
    }

    // Return time interval as a string
    std::string asString() const {
        std::stringstream ss;
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

// Non-member operators for CMTimeInterval

inline CMTimeInterval operator*(const CMTimeInterval& timeInt, double scalar) {
    return CMTimeInterval(
        0,
        0,
        0,
        timeInt.to_s() * scalar
    );
}

inline CMTimeInterval operator*(double scalar, const CMTimeInterval& timeInt) {
    return timeInt*scalar;
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
// Times cannot have day or month equal to 0
struct CMTime {
private:
    // Adjust the internal variables so they are within their limits
    void adjust() {
        while (s >= 60) {
            s -= 60;
            m += 1;
        }
        while (s < 0) {
            s += 60;
            m -= 1;
        }
        while (m >= 60) {
            m -= 60;
            h += 1;
        }
        while (m < 0) {
            m += 60;
            h -= 1;
        }
        while (h >= 24) {
            h -= 24;
            dd += 1;
        }
        while (h < 0) {
            h += 24;
            dd -= 1;
        }
        bool dmySatisfied = false;
        while (!dmySatisfied) {
            if (mm > 12) {
                mm -= 12;
                yy += 1;
            }
            else if (mm <= 0) {
                mm += 12;
                yy -= 1;
            }
            else if (dd > daysInMonth(yy, mm)) {
                dd -= daysInMonth(yy, mm);
                mm += 1;
            }
            else if (dd <= 0) {
                dd += daysInMonth(yy, mm);
                mm -= 1;
            }
            else {
                dmySatisfied = true;
            }
        }
    }

public:
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    float s;

    // Constructor without values
    CMTime() {}

    // Constructor with values
    CMTime(int yy, int mm, int dd, int h, int m, float s)
        : yy(yy), mm(mm), dd(dd), h(h), m(m), s(s) {
        
        adjust();
    }

    // Sets the internal variables from separate inputs
    void set(int yy, int mm, int dd, int h, int m, float s) {
        this->yy = yy;
        this->mm = mm;
        this->dd = dd;
        this->h = h;
        this->m = m;
        this->s = s;
        adjust();
    }

    // Returns true if internal year is a leap year
    constexpr bool isLeap() const {
        return ::isLeap(yy);
    }

    // Sets the internal variables from CMTime_F input
    void set(const CMTime_F& fortranTime) {
        this->yy = fortranTime.yy;
        this->mm = fortranTime.mm;
        this->dd = fortranTime.dd;
        this->h = fortranTime.h;
        this->m = fortranTime.m;
        this->s = fortranTime.s;
        adjust();
    }

    bool operator==(const CMTime& other) const {
        return (this->yy == other.yy &&
                this->mm == other.mm &&
                this->dd == other.dd &&
                this->h == other.h &&
                this->m == other.m &&
                this->s == other.s);
    }

    bool operator!=(const CMTime& other) const {
        return !(operator==(other));
    }

    constexpr int fullDaysPassedInYear() const {
        int fullDays = 0;
        for (int month = 1; month < mm; month++) {
            fullDays += daysInMonth(yy, month);
        }
        fullDays += (dd - 1);
        return fullDays;
    }

    constexpr int fullDaysRemainingInYear() const {
        int fullDays = 0;
        fullDays += (daysInMonth(yy, mm) - dd);
        for (int month = mm + 1; month <= 12; month++) {
            fullDays += daysInMonth(yy, month);
        }
        return fullDays;
    }

    // Calculates the number of full days between (the same time on) two dates (this - other)
    constexpr long long fullDaysBetween(const CMTime& other) const {
        return (*this >= other)
            ? ::fullDaysBetween(*this, other)
            : -::fullDaysBetween(other, *this);
    }

    // CMTime + CMTimeInterval = CMTime
    CMTime operator+(const CMTimeInterval& other) const {
        return CMTime(
            this->yy,
            this->mm,
            this->dd + other.dd,
            this->h + other.h,
            this->m + other.m,
            this->s + other.s
        );
    }

    // CMTime - CMTime = CMTimeInterval
    CMTimeInterval operator-(const CMTime& other) const {
        return CMTimeInterval(
            fullDaysBetween(other),
            this->h - other.h,
            this->m - other.m,
            this->s - other.s
        );
    }

    CMTime& operator+=(const CMTimeInterval& other) {
        this->set(this->yy,
                  this->mm,
                  this->dd + other.dd,
                  this->h + other.h,
                  this->m + other.m,
                  this->s + other.s);
        return *this;
    }

    constexpr bool operator>(const CMTime& other) const {
        if (this->yy > other.yy) {
            return true;
        }
        else if (this->yy < other.yy) {
            return false;
        }
        // else, years are same; must check next
        else if (this->mm > other.mm) {
            return true;
        }
        else if (this->mm < other.mm) {
            return false;
        }
        // else, months are same; must check next
        else if (this->dd > other.dd) {
            return true;
        }
        else if (this->dd < other.dd) {
            return false;
        }
        // else, days are same; must check next
        else if (this->h > other.h) {
            return true;
        }
        else if (this->h < other.h) {
            return false;
        }
        // else, hours are same; must check next
        else if (this->m > other.m) {
            return true;
        }
        else if (this->m < other.m) {
            return false;
        }
        // else, minutes are same; must check next
        else if (this->s > other.s) {
            return true;
        }
        else {
            return false;
        }
    }

    constexpr bool operator>=(const CMTime& other) const {
        return (operator>(other) || operator==(other));
    }

    constexpr bool operator<(const CMTime& other) const {
        return !operator>=(other);
    }

    constexpr bool operator<=(const CMTime& other) const {
        return !operator>(other);
    }

    // Return time as a string
    std::string asString() const {
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

// Calculates the number of full days between (the same time on) two dates (time1 - time2)
// time1 must be after time2
constexpr long long fullDaysBetween(const CMTime& time1, const CMTime& time2) {
    long long fullDays = 0;

    if (time1.yy > time2.yy) {
        fullDays += time2.fullDaysRemainingInYear();
        for (int year = time2.yy + 1; year < time1.yy; year++) {
            fullDays += daysInYear(year);
        }
        fullDays += time1.fullDaysPassedInYear();
    }
    else if (time1.mm > time2.mm) {
        fullDays += (daysInMonth(time2.yy, time2.mm) - time2.dd);
        for (int month = time2.mm + 1; month < time1.mm; month++) {
            fullDays += daysInMonth(time2.yy, month);
        }
        fullDays += (time1.dd - 1);
    }
    else {
        fullDays += (time1.dd - time2.dd);
    }

    return fullDays;
}

#endif