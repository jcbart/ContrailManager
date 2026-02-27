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
    // Adjust only seconds, minutes, and hours within their limits
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
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    float s;

    // Sets the internal variables
    void set(int yy, int mm, int dd, int h, int m, float s) {
        this->yy = yy;
        this->mm = mm;
        this->dd = dd;
        this->h = h;
        this->m = m;
        this->s = s;
        adjust();
    }

    constexpr bool operator==(const CMTimeInterval& other) const {
        return (this->yy == other.yy &&
                this->mm == other.mm &&
                this->dd == other.dd &&
                this->h == other.h &&
                this->m == other.m &&
                this->s == other.s);
    }

    bool operator!=(const CMTimeInterval& other) const {
        return !(operator==(other));
    }

    CMTimeInterval operator+(const CMTimeInterval& other) const {
        CMTimeInterval newTimeInt;
        newTimeInt.set(this->yy + other.yy,
                       this->mm + other.mm,
                       this->dd + other.dd,
                       this->h + other.h,
                       this->m + other.m,
                       this->s + other.s);
        return newTimeInt;
    }

    CMTimeInterval operator-(const CMTimeInterval& other) const {
        CMTimeInterval newTimeInt;
        newTimeInt.set(this->yy - other.yy,
                       this->mm - other.mm,
                       this->dd - other.dd,
                       this->h - other.h,
                       this->m - other.m,
                       this->s - other.s);
        return newTimeInt;
    }

    CMTimeInterval& operator+=(const CMTimeInterval& other) {
        this->set(this->yy + other.yy,
                  this->mm + other.mm,
                  this->dd + other.dd,
                  this->h + other.h,
                  this->m + other.m,
                  this->s + other.s);
        return *this;
    }

    // Turns days, hours, minutes, and seconds into seconds
    constexpr double dhms_to_s() const {
        return (dd*86400. + h*3600. + m*60. + s);
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

// Non-member operators for CMTimeInterval

inline CMTimeInterval operator*(const CMTimeInterval& timeInt, float scalar) {
    CMTimeInterval newTimeInt;
    newTimeInt.set(timeInt.yy*scalar,
                   timeInt.mm*scalar,
                   timeInt.dd*scalar,
                   timeInt.h*scalar,
                   timeInt.m*scalar,
                   timeInt.s*scalar);
    return newTimeInt;
}

inline CMTimeInterval operator*(float scalar, const CMTimeInterval& timeInt) {
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
            else if (isLeap() && dd > DAYS_IN_MONTH_LEAP[mm-1]) {
                dd -= DAYS_IN_MONTH_LEAP[mm-1];
                mm += 1;
            }
            else if (isLeap() && dd <= 0) {
                dd += DAYS_IN_MONTH_LEAP[mm-1];
                mm -= 1;
            }
            else if (!isLeap() && dd > DAYS_IN_MONTH[mm-1]) {
                dd -= DAYS_IN_MONTH[mm-1];
                mm += 1;
            }
            else if (!isLeap() && dd <= 0) {
                dd += DAYS_IN_MONTH[mm-1];
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

    // Returns true if internal year is a leap year
    bool isLeap() const {
        if (yy % 4 == 0) {
            if (yy % 100 == 0) {
                return (yy % 400 == 0);
            }
            return true;
        }
        return false;
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

    // CMTime + CMTimeInterval = CMTime
    CMTime operator+(const CMTimeInterval& other) const {
        CMTime newTime;
        newTime.set(this->yy + other.yy,
                    this->mm + other.mm,
                    this->dd + other.dd,
                    this->h + other.h,
                    this->m + other.m,
                    this->s + other.s);
        return newTime;
    }

    // CMTime - CMTime = CMTimeInterval
    CMTimeInterval operator-(const CMTime& other) const {
        CMTimeInterval timeInt;
        timeInt.set(this->yy - other.yy,
                    this->mm - other.mm,
                    this->dd - other.dd,
                    this->h - other.h,
                    this->m - other.m,
                    this->s - other.s);
        return timeInt;
    }

    CMTime& operator+=(const CMTimeInterval& other) {
        this->set(this->yy + other.yy,
                  this->mm + other.mm,
                  this->dd + other.dd,
                  this->h + other.h,
                  this->m + other.m,
                  this->s + other.s);
        return *this;
    }

    bool operator>(const CMTime& other) const {
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

    bool operator>=(const CMTime& other) const {
        return (operator>(other) || operator==(other));
    }

    bool operator<(const CMTime& other) const {
        return !operator>=(other);
    }

    bool operator<=(const CMTime& other) const {
        return !operator>(other);
    }

    // Turns days, hours, minutes, and seconds into seconds
    constexpr double dhms_to_s() const {
        return (dd*86400. + h*3600. + m*60. + s);
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

#endif