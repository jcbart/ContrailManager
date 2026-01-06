#ifndef TIMEKEEPING_H
#define TIMEKEEPING_H

#include <string>

const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int DAYS_IN_MONTH_LEAP[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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
    int s;

    void set(int yy, int mm, int dd, int h, int m, int s);

    bool operator==(const CMTimeInterval& other) const;

    bool operator!=(const CMTimeInterval& other) const;

    CMTimeInterval operator+(const CMTimeInterval& other) const;

    CMTimeInterval operator-(const CMTimeInterval& other) const;

    CMTimeInterval& operator+=(const CMTimeInterval& other);

    int dhms_to_s() const;

    // Return time as a string
    std::string asString();
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
    int s;
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
    int s;

    bool isLeap();

    void set(int yy, int mm, int dd, int h, int m, int s);

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

    int dhms_to_s() const;

    // Return time as a string
    std::string asString();
};

#endif