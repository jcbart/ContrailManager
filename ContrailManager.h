#ifndef CONTRAILMANAGER
#define CONTRAILMANAGER

#include "variables.h"

// ESMF does not have well-established time classes in C, so I'm writing my own
const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int DAYS_IN_MONTH_LEAP[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class CMTime {
private:
    void adjust() {
        while (s >= 60) {
            s -= 60;
            m += 1;
        }
        while (m >= 60) {
            m -= 60;
            h += 1;
        }
        while (h >= 24) {
            h -= 24;
            dd += 1;
        }
        bool dmySatisfied = false;
        while (!dmySatisfied) {
            if (mm > 12) {
                mm -= 12;
                yy += 1;
            }
            else if (isLeap() && dd > DAYS_IN_MONTH_LEAP[mm-1]) {
                dd -= DAYS_IN_MONTH_LEAP[mm-1];
                mm += 1;
            }
            else if (!isLeap() && dd > DAYS_IN_MONTH[mm-1]) {
                dd -= DAYS_IN_MONTH[mm-1];
                mm += 1;
            }
            else {
                dmySatisfied = true;
            }
        }
    }

public:
    int yy, mm, dd, h, m, s;

    bool isLeap() {
        if (yy % 4 == 0) {
            if (yy % 100 == 0) {
                return (yy % 400 == 0);
            }
            return true;
        }
        return false;
    }

    void set(int yy, int mm, int dd, int h, int m, int s) {
        this->yy = yy;
        this->mm = mm;
        this->dd = dd;
        this->h = h;
        this->m = m;
        this->s = s;

        adjust();
    }

    CMTime operator+(const CMTime& other) const {
        CMTime newTime;
        newTime.set(this->yy + other.yy,
                    this->mm + other.mm,
                    this->dd + other.dd,
                    this->h + other.h,
                    this->m + other.m,
                    this->s + other.s);
        newTime.adjust();
        return newTime;
    }
    bool operator<=(const CMTime& other) const {
        if (this->yy <= other.yy) {
            return true;
        }
        else if (this->mm <= other.mm) {
            return true;
        }
        else if (this->dd <= other.dd) {
            return true;
        }
        else if (this->h <= other.h) {
            return true;
        }
        else if (this->m <= other.m) {
            return true;
        }
        else if (this->s <= other.s) {
            return true;
        }
        return false;
    }
};

class ContrailManager {
private:
    CMTime timeStep;
    CMTime currTime;

public:
    Variable2D XLAT;
    Variable2D XLONG;

    void init();

    void run(CMTime& startTime, CMTime& stopTime);

    void init_XLAT(int ids, int ide, int jds, int jde);
    void init_XLONG(int ids, int ide, int jds, int jde);
};

#endif