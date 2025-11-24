#ifndef CONTRAILMANAGER
#define CONTRAILMANAGER

#include "timekeeping.h"
#include "variables.h"

struct Location;

// A structure to store 2 indices
struct IDX2 {
    int i;
    int j;
};

// A structure to store 3 indices
struct IDX3 {
    int i;
    int j;
    int k;
};

class ContrailManager {
private:
    int dummy1 = 0;
    int dummy2 = 0;
    int dummy3 = 0;
    int dummy4 = 0;
    int dummy5 = 0;
    int dummy6 = 0;
    int dummy7 = 0;
    int dummy8 = 0;
    CMTimeInterval timeStep;
    CMTime currTime;
    bool firstRunCall = true;
    int latSize, lonSize, altSize = 0;

public:
    Variable2D XLAT;
    Variable2D XLONG;
    Variable3D Z;

    void init();

    void setStartTime(CMTime& startTime);

    void run(CMTime& startTime, CMTime& stopTime);

    void init_XLAT(int ids, int ide, int jds, int jde);
    void init_XLONG(int ids, int ide, int jds, int jde);
    void init_Z(int ids, int ide, int jds, int jde, int kds, int kde);

    IDX3 find_corner_idxs(Location& loc);
    IDX2 find_ij(Location& loc);
    int find_k(Location& loc, IDX2& ij);
};

#endif