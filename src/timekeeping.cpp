#include "timekeeping.h"

// ---------------------------------------------
// CMTimeInterval
// ---------------------------------------------

// Adjust only seconds, minutes, and hours within their limits
void CMTimeInterval::adjust() {
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

// Sets the internal variables
void CMTimeInterval::set(int yy, int mm, int dd, int h, int m, float s) {
    this->yy = yy;
    this->mm = mm;
    this->dd = dd;
    this->h = h;
    this->m = m;
    this->s = s;
    adjust();
}

bool CMTimeInterval::operator==(const CMTimeInterval& other) const {
    if (this->yy == other.yy &&
        this->mm == other.mm &&
        this->dd == other.dd &&
        this->h == other.h &&
        this->m == other.m &&
        this->s == other.s) {
        return true;
    }
    else {
        return false;
    }
}

bool CMTimeInterval::operator!=(const CMTimeInterval& other) const {
    return !(operator==(other));
}

CMTimeInterval CMTimeInterval::operator+(const CMTimeInterval& other) const {
    CMTimeInterval newTimeInt;
    newTimeInt.set(this->yy + other.yy,
                    this->mm + other.mm,
                    this->dd + other.dd,
                    this->h + other.h,
                    this->m + other.m,
                    this->s + other.s);
    return newTimeInt;
}

CMTimeInterval CMTimeInterval::operator-(const CMTimeInterval& other) const {
    CMTimeInterval newTimeInt;
    newTimeInt.set(this->yy - other.yy,
                    this->mm - other.mm,
                    this->dd - other.dd,
                    this->h - other.h,
                    this->m - other.m,
                    this->s - other.s);
    return newTimeInt;
}

CMTimeInterval& CMTimeInterval::operator+=(const CMTimeInterval& other) {
    this->set(this->yy + other.yy,
                this->mm + other.mm,
                this->dd + other.dd,
                this->h + other.h,
                this->m + other.m,
                this->s + other.s);
    return *this;
}

// Non-member operators for CMTimeInterval

CMTimeInterval operator*(const CMTimeInterval& timeInt, float scalar) {
    CMTimeInterval newTimeInt;
    newTimeInt.set(timeInt.yy*scalar,
                   timeInt.mm*scalar,
                   timeInt.dd*scalar,
                   timeInt.h*scalar,
                   timeInt.m*scalar,
                   timeInt.s*scalar);
    return newTimeInt;
}

CMTimeInterval operator*(float scalar, const CMTimeInterval& timeInt) {
    return timeInt*scalar;
}

// ---------------------------------------------
// CMTime
// ---------------------------------------------

// Adjust the internal variables so they are within their limits
void CMTime::adjust() {
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

// Sets the internal variables from separate inputs
void CMTime::set(int yy, int mm, int dd, int h, int m, float s) {
    this->yy = yy;
    this->mm = mm;
    this->dd = dd;
    this->h = h;
    this->m = m;
    this->s = s;
    adjust();
}

// Sets the internal variables from CMTime_F input
void CMTime::set(const CMTime_F& fortranTime) {
    this->yy = fortranTime.yy;
    this->mm = fortranTime.mm;
    this->dd = fortranTime.dd;
    this->h = fortranTime.h;
    this->m = fortranTime.m;
    this->s = fortranTime.s;
    adjust();
}

bool CMTime::operator==(const CMTime& other) const {
    if (this->yy == other.yy &&
        this->mm == other.mm &&
        this->dd == other.dd &&
        this->h == other.h &&
        this->m == other.m &&
        this->s == other.s) {
        return true;
    }
    else {
        return false;
    }
}

bool CMTime::operator!=(const CMTime& other) const {
    return !(operator==(other));
}

// CMTime + CMTimeInterval = CMTime
CMTime CMTime::operator+(const CMTimeInterval& other) const {
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
CMTimeInterval CMTime::operator-(const CMTime& other) const {
    CMTimeInterval timeInt;
    timeInt.set(this->yy - other.yy,
                this->mm - other.mm,
                this->dd - other.dd,
                this->h - other.h,
                this->m - other.m,
                this->s - other.s);
    return timeInt;
}

CMTime& CMTime::operator+=(const CMTimeInterval& other) {
    this->set(this->yy + other.yy,
                this->mm + other.mm,
                this->dd + other.dd,
                this->h + other.h,
                this->m + other.m,
                this->s + other.s);
    return *this;
}

bool CMTime::operator>(const CMTime& other) const {
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

bool CMTime::operator<(const CMTime& other) const {
    if (this->yy < other.yy) {
        return true;
    }
    else if (this->yy > other.yy) {
        return false;
    }
    // else, years are same; must check next
    else if (this->mm < other.mm) {
        return true;
    }
    else if (this->mm > other.mm) {
        return false;
    }
    // else, months are same; must check next
    else if (this->dd < other.dd) {
        return true;
    }
    else if (this->dd > other.dd) {
        return false;
    }
    // else, days are same; must check next
    else if (this->h < other.h) {
        return true;
    }
    else if (this->h > other.h) {
        return false;
    }
    // else, hours are same; must check next
    else if (this->m < other.m) {
        return true;
    }
    else if (this->m > other.m) {
        return false;
    }
    // else, minutes are same; must check next
    else if (this->s < other.s) {
        return true;
    }
    else {
        return false;
    }
}

bool CMTime::operator<=(const CMTime& other) const {
    return !(*this > other);
}

bool CMTime::operator>=(const CMTime& other) const {
    return !(*this < other);
}