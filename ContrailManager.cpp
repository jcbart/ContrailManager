#include <ESMC.h>
#include <iostream>
#include <string>
#include "ContrailManager.h"
#include "timekeeping.h"
#include "variables.h"
#include "segment.h"
#include "flight.h"
#include "projection.h"
#include "mapUtils.h"

void ContrailManager::init() {
    int rc;
    std::string msg;
    rc = ESMC_LogWrite("Initialising Contrail Manager:", ESMC_LOGMSG_INFO);
    timeStep_s = 10; // Read from CM config
    timeStep.set(0, 0, 0, 0, 0, timeStep_s);
    msg = "Contrail Manager internal time step set to " + timeStep.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    // Read maxInitialSegLen and maxContrailAge_s from CM config
    // Read flight data etc
    Flight test_flight;
    test_flight.ID = 1;
    CMTime time1 = {2025, 4, 1, 0, 0, 10};
    CMTime time2 = {2025, 4, 1, 0, 2, 0};
    test_flight.wpTimes.push_back(time1);
    test_flight.wpTimes.push_back(time2);
    Geo3D loc1 = {-0.71, 51.73, 10e3};
    Geo3D loc2 = {-1.05, 51.76, 11e3};
    test_flight.wpLocs.push_back(loc1);
    test_flight.wpLocs.push_back(loc2);
    test_flight.numWps = 2;
    flights.push_back(test_flight);
    rc = ESMC_LogWrite("Contrail Manager initialised", ESMC_LOGMSG_INFO);
}

void ContrailManager::init_vars(int ids, int ide, int jds, int jde, int kds, int kde) {
    int rc;
    std::string msg;

    XLONG.init("XLONG", ids, ide, jds, jde);
    XLAT.init("XLAT", ids, ide, jds, jde);
    Z.init("Z", ids, ide, jds, jde, kds, kde);
    Z_AT_W.init("Z_AT_W", ids, ide, jds, jde, kds, kde+1);
    U.init("U", ids, ide, jds, jde, kds, kde);
    V.init("V", ids, ide, jds, jde, kds, kde);
    W.init("W", ids, ide, jds, jde, kds, kde);
    QV.init("QV", ids, ide, jds, jde, kds, kde);
    varsInitd = true;

    msg = "Contrail Manager variables initialised with dimensions:";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ids = " + std::to_string(ids) + ", jds = " + std::to_string(jds) + ", kds = " + std::to_string(kds);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "ide = " + std::to_string(ide) + ", jde = " + std::to_string(jde) + ", kde = " + std::to_string(kde);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    msg = "Note: Contrail Manager does not use a staggered grid except for Z_AT_W where kde += 1.";
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Integrate between times
void ContrailManager::run(CMTime& startTime, CMTime& stopTime) {
    int rc;
    std::string msg;

    if (firstRunCall) {
        setup_on_first_run(startTime);
        firstRunCall = false;
    }
    
    // Check startTime matches expected time
    if (currTime != startTime) {
        std::cerr << "Error: currTime (" << currTime.asString() << ") does not match "
                  << "integration startTime (" << startTime.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Check there are a whole number of time steps between startTime and stopTime
    CMTimeInterval timeInterval = stopTime-startTime;
    if (timeInterval.dhms_to_s() % timeStep.dhms_to_s() != 0) {
        std::cerr << "Error: Integration time interval (" << timeInterval.asString()
                  << ") is not an integer multiple of time step ("
                  << timeStep.asString() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }

    msg = "Integrating between " + startTime.asString() + " and "
          + stopTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    currTime = startTime;
    while (currTime+timeStep <= stopTime) {
        CMTime timeStepStart = currTime;
        CMTime timeStepEnd = currTime + timeStep;
        /*
        1. Create new segments
        2. Integrate all segment plumes (aggregate vapour delta based on start location,
           mark dead segments)
        3. Dump old/dead segments before advection (aggregate leftover crystals)
        4. Advect all segments (update dependent segments locs and find new length and width)
        5. Increment currTime
        */
        
        // 1. Create segments
        create_segments(timeStepStart, timeStepEnd);

        // 2. Integrate plumes
        integrate_plumes(timeStepStart, timeStepEnd);

        // 3. Dump old or dead segments in the same location they were integrated (check if timeStepEnd)
        for (int i = segments.size()-1; i >= 0; i--) {
            if ((timeStepEnd - segments[i].birthTime).dhms_to_s() > maxContrailAge_s) {
                // Dump, then
                segments.erase(segments.begin() + i);
            }
        }

        // 4. Advect segments
        advect_segments(timeStepStart, timeStepEnd);

        // 5. Increment currTime
        currTime = timeStepEnd;
        msg = "Current time: " + currTime.asString();
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
    }
}


// Completes the setup required on the first run call (i.e. after getting external data)
void ContrailManager::setup_on_first_run(CMTime& startTime) {
    int rc;
    std::string msg;

    if (!varsInitd) {
        std::cerr << "ContrailManager run called before vars have been initialised. Stopping."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!proj.isInitd) {
        std::cerr << "ContrailManager run called before projection has been initialised. Stopping."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    currTime = startTime;
    msg = "Contrail Manager current time set to " + currTime.asString();
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

    // Take sizes from Z
    ids = Z.get_ids();
    ide = Z.get_ide();
    jds = Z.get_jds();
    jde = Z.get_jde();
    kds = Z.get_kds();
    kde = Z.get_kde();
    lonSize = Z.get_i_size();
    latSize = Z.get_j_size();
    altSize = Z.get_k_size();
}

// Create new segments from flights
void ContrailManager::create_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    int rc;
    std::string msg;

    int num_created = 0;
    for (const Flight& flight : flights) {
        msg = "Creating segments for flight: " + std::to_string(flight.ID);
        rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
        // Find start and end locations of flight in time step
        Geo3D flStartLoc, flEndLoc;
        bool lastWpStart, lastWpEnd;
        bool startFound = find_flight_loc(flight, timeStepStart, flStartLoc, lastWpStart);
        if (!startFound) continue;
        bool endFound = find_flight_loc(flight, timeStepEnd, flEndLoc, lastWpEnd);
        if (!endFound) continue;

        // n iterates each leg since the route may be sectioned by waypoints
        for (int n = 0; n <= lastWpEnd - lastWpStart; n++) {
            // Find start and end locs and times for leg
            Geo3D legStartloc, legEndLoc;
            CMTime legStartTime, legEndTime;
            // If first leg
            if (n == 0) {
                legStartloc = flStartLoc;
                legStartTime = timeStepStart;
            }
            else {
                legStartloc = flight.wpLocs[lastWpStart+n];
                legStartTime = flight.wpTimes[lastWpStart+n];
            }
            // If last leg
            if (lastWpStart + n == lastWpEnd) {
                legEndLoc = flEndLoc;
                legEndTime = timeStepEnd;
            }
            else {
                legEndLoc = flight.wpLocs[lastWpStart+n+1];
                legEndTime = flight.wpTimes[lastWpStart+n+1];
            }

            // Create as many segments as needed between
            float distInLeg = great_circle_dist(legStartloc, legEndLoc);
            int numNewSegments = ceil(distInLeg / maxInitialSegLen);
            float segLen = distInLeg / numNewSegments;
            Geo3D backLoc = legStartloc;
            for (int i = 0; i < numNewSegments; i++) {
                msg = "Segment " + std::to_string(i) + ":";
                rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);

                Segment newSeg;
                newSeg.back = backLoc;
                // Find new front loc
                // Fraction of the total distance where the front of the segment is
                float f_front = (i+1.)/numNewSegments;
                newSeg.front = great_circle_interp(f_front, legStartloc, legEndLoc);

                // Set back loc for next segment
                backLoc = newSeg.front;

                // Use find_interp to find if in grid
                // If any segment location is not in the grid, don't add the segment
                bool inGrid;
                Interp interpTemp;
                inGrid = find_interp(newSeg.back, interpTemp);
                if (!inGrid) {continue;}
                inGrid = find_interp(newSeg.front, interpTemp);
                if (!inGrid) {continue;}

                // Give segment its centre location
                find_dependent_locs(newSeg);

                newSeg.length = segLen;

                // Find birth time
                // Fraction of leg duration passed at centre of segment
                float f_centre = (i+0.5)/numNewSegments;
                newSeg.birthTime = legStartTime + f_centre * (legEndTime - legStartTime);

                // Add emissions info

                segments.push_back(newSeg);
                msg = "Segment created with birth time: " + newSeg.birthTime.asString();
                rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
                msg = "Centre location: (" + std::to_string(newSeg.centre.lon) + ", " + std::to_string(newSeg.centre.lat) + ", " + std::to_string(newSeg.centre.alt) + ")";
                rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
                msg = "Length: " + std::to_string(newSeg.length);
                rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
                num_created++;
            }
        }
    }
    msg = "Number of segments created: " + std::to_string(num_created);
    rc = ESMC_LogWrite(msg.c_str(), ESMC_LOGMSG_INFO);
}

// Integrate each segment plume using plume model
void ContrailManager::integrate_plumes(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    for (Segment& seg : segments) {
        // Integrate
        // If birthTime > timeStepStart, integrate from birthTime to timeStepEnd
        // Aggregate
        // Mark dead segments
        continue;
    }
}

// Advect front and back locations of all segments, find new length, width, and dependent locs
void ContrailManager::advect_segments(const CMTime& timeStepStart, const CMTime& timeStepEnd) {
    for (int i = segments.size()-1; i >= 0; i--) {
        Segment& seg = segments[i];
        float duration_s;
        // If birthTime > timeStepStart, integrate from birthTime to timeStepEnd
        if (seg.birthTime > timeStepStart) {
            duration_s = (timeStepStart - seg.birthTime).dhms_to_s();
        }
        else {
            duration_s = timeStep_s;
        }
        bool inGridBack, inGridFront;

        inGridBack = advect_loc(seg.back, duration_s);
        inGridFront = advect_loc(seg.front, duration_s);

        // If either end of segment has drifted out of grid, remove segment 
        if (!(inGridBack && inGridFront)) {
            segments.erase(segments.begin() + i);
            continue;
        }
        seg.length = great_circle_dist(seg.back, seg.front);
        // width too
        find_dependent_locs(seg);
    }
}

// Updates ij with the lon/lat grid cell indices loc lies within
// Calls the method in ContrailManager::proj and removes ids = jds = 1 assumption
// Returns false if loc is not in grid
bool ContrailManager::loc_to_ij(const Geo2D& loc, IDX2& ij) {
    bool inGrid = false;
    ij = proj.loc_to_ij(loc);
    // Correct assumption that i and j start at 1
    ij.i += ids - 1;
    ij.j += jds - 1;
    if (ij.i >= ids && ij.i <= ide && ij.j >= jds && ij.j <= jde) {
        inGrid = true;
    }
    return inGrid;
}

// Updates ijk with the lon/lat/alt grid cell indices loc lies within
// Returns false if loc is not in grid
bool ContrailManager::loc_to_ijk(const Geo3D& loc, IDX3& ijk) {
    bool inGrid;
    // Get ij
    IDX2 ij;
    inGrid = loc_to_ij(loc, ij);
    if (!inGrid) return inGrid;
    // Turn IDX2 object into IDX3
    ijk = ij;
    // Get k
    inGrid = find_k_inside(loc, ijk, ijk.k);
    return inGrid;
}

// Returns the lon/lat grid values at indices ij
Geo2D ContrailManager::ij_to_loc(const IDX2& ij) {
    Geo2D loc;
    loc.lon = *XLONG.get(ij.i, ij.j);
    loc.lat = *XLAT.get(ij.i, ij.j);
    return loc;
}

// Returns the grid values at indices ijk
Geo3D ContrailManager::ijk_to_loc(const IDX3& ijk) {
    Geo3D loc;
    loc.lon = *XLONG.get(ijk.i, ijk.j);
    loc.lat = *XLAT.get(ijk.i, ijk.j);
    loc.alt = *Z.get(ijk);
    return loc;
}

// Finds the flight location at the given time with a great circle interpolation between
// neighbouring waypoints
// loc is given the location
// lastWpIDX is given the index of the last waypoint passed (e.g. 0 for 0th waypoint)
// Returns false if flight is before first or after last waypoint at time
bool ContrailManager::find_flight_loc(const Flight& flight, const CMTime& time, Geo3D& loc, int lastWpIDX) {
    if (time < flight.wpTimes[0] || time > flight.wpTimes[flight.numWps-1]) {
        // Flight is before first waypoint or after last waypoint
        return false;
    }
    // Else, flight is within waypoint route
    // Find last waypoint passed
    lastWpIDX = 0;
    for (int i = 0; i < flight.numWps-1; i++) {
        if (time >= flight.wpTimes[i] && time < flight.wpTimes[i+1]) {
            lastWpIDX = i;
            break;
        }
    }
    // Flight is between lastWpPassed and lastWpPassed+1
    loc = great_circle_interp(time,
                              flight.wpTimes[lastWpIDX], flight.wpLocs[lastWpIDX],
                              flight.wpTimes[lastWpIDX+1], flight.wpLocs[lastWpIDX+1]);
    return true;
}

// Determines the dependent segment locations (everything except front and back) based on front and back
void ContrailManager::find_dependent_locs(Segment& seg) {
    seg.centre = great_circle_interp(0.5, seg.back, seg.front);
}

// Returns location at a fraction f [0,1] along a great circle by interpolating
// between two waypoints
// If f = 0, the returned location will be loc1 and vice versa for f = 1
Geo3D ContrailManager::great_circle_interp(const float f,
                                           const Geo3D& loc1, const Geo3D& loc2) {
    // Lat/lon interpolation
    // Pass Geo2D version of Geo3D objects to function
    Cart3D loc1_Cart = Geo2D_to_Cart3D(loc1);
    Cart3D loc2_Cart = Geo2D_to_Cart3D(loc2);
    // Dot product is clamped in range [-1, 1] to prevent precision errors
    float delta = acos(std::max(-1.F, std::min(1.F, dot_prod(loc1_Cart, loc2_Cart))));
    Cart3D slerpResult;
    // If delta is tiny, resort to LERP
    if (delta < 1e-9) {
        slerpResult.x = (1-f)*loc1_Cart.x + f*loc2_Cart.x;
        slerpResult.y = (1-f)*loc1_Cart.y + f*loc2_Cart.y;
        slerpResult.z = (1-f)*loc1_Cart.z + f*loc2_Cart.z;
    }
    else {
        float slerp1 = sin((1-f) * delta) / sin(delta);
        float slerp2 = sin(f * delta) / sin(delta);
        slerpResult.x = slerp1*loc1_Cart.x + slerp2*loc2_Cart.x;
        slerpResult.y = slerp1*loc1_Cart.y + slerp2*loc2_Cart.y;
        slerpResult.z = slerp1*loc1_Cart.z + slerp2*loc2_Cart.z;
    }
    // Pass Geo2D into Geo3D object
    Geo3D result = Cart3D_to_Geo2D(slerpResult);

    // Altitude interpolation
    result.alt = (1-f) * loc1.alt + f * loc2.alt;
    return result;
}

// Returns location at time by interpolating between two waypoints given times
// If time = time1, the returned location will be loc1 and vice versa
Geo3D ContrailManager::great_circle_interp(const CMTime& time,
                                           const CMTime& time1, const Geo3D& loc1,
                                           const CMTime& time2, const Geo3D& loc2) {
    float time_s = time.dhms_to_s();
    float time1_s = time1.dhms_to_s();
    float time2_s = time2.dhms_to_s();
    float f = (time_s - time1_s)/(time2_s - time1_s);
    return great_circle_interp(f, loc1, loc2);
}

// Fills an Interp object with indices and weights of grid points to interpolate for a given loc
// Returns false if interpolation points cannot be found (loc outside grid)
bool ContrailManager::find_interp(const Geo3D& loc, Interp& interp) {
    bool inGrid = find_interp_points(loc, interp);
    if (!inGrid) {return inGrid;}
    find_interp_weights(loc, interp);
    return inGrid;
}

// Finds interpolation points for a location and updates interp
// Leaves weights untouched; use find_interp_weights to update them
// Returns true if location is in grid
// If false, interp contains garbage
bool ContrailManager::find_interp_points(const Geo3D& loc, Interp& interp) {
    bool inGrid = false;
    IDX2 ijCentre;
    inGrid = loc_to_ij(loc, ijCentre);

    // If inGrid is still false, loc is not inside a grid cell
    if (!inGrid) {return inGrid;}
    
    // Determine existence of neighbouring quadrilaterals
    bool doLeft = true, doRight = true, doLower = true, doUpper = true;
    if (ijCentre.i == ids) {doLeft = false;}
    if (ijCentre.i == ide) {doRight = false;}
    if (ijCentre.j == jds) {doLower = false;}
    if (ijCentre.j == jde) {doUpper = false;}
    IDX2 ij1, ij2, ij3, ij4;
    // Set to true if loc is inside a quad (also to avoid excess computation)
    bool inQuad = false;
    if (!inQuad && doLeft && doLower) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i-1, ijCentre.j};
        ij3 = {ijCentre.i-1, ijCentre.j-1};
        ij4 = {ijCentre.i, ijCentre.j-1};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doLeft && doUpper) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i, ijCentre.j+1};
        ij3 = {ijCentre.i-1, ijCentre.j+1};
        ij4 = {ijCentre.i-1, ijCentre.j};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doRight && doUpper) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i+1, ijCentre.j};
        ij3 = {ijCentre.i+1, ijCentre.j+1};
        ij4 = {ijCentre.i, ijCentre.j+1};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    if (!inQuad && doRight && doLower) {
        ij1 = ijCentre;
        ij2 = {ijCentre.i, ijCentre.j-1};
        ij3 = {ijCentre.i+1, ijCentre.j-1};
        ij4 = {ijCentre.i+1, ijCentre.j};
        inQuad = loc_in_quad(loc, ij_to_loc(ij1), ij_to_loc(ij2), ij_to_loc(ij3), ij_to_loc(ij4));
    }
    // If inQuad is still false, no quad has been found with loc inside
    if (!inQuad) {
        return inQuad;
    }
    // Find k for each of the four grid points
    // Return false if no k found; else, update interp point
    int k;
    // Point 1
    inQuad = find_k_below(loc, ij1, k);
    if (!inQuad) {return inQuad;}
    else interp.points[0] = {ij1.i, ij1.j, k};
    // Point 2
    inQuad = find_k_below(loc, ij2, k);
    if (!inQuad) {return inQuad;}
    else interp.points[1] = {ij2.i, ij2.j, k};
    // Point 3
    inQuad = find_k_below(loc, ij3, k);
    if (!inQuad) {return inQuad;}
    else interp.points[2] = {ij3.i, ij3.j, k};
    // Point 4
    inQuad = find_k_below(loc, ij4, k);
    if (!inQuad) {return inQuad;}
    else interp.points[3] = {ij4.i, ij4.j, k};
    // All points found, return true
    return inQuad;
}

// Finds the index k such that loc.alt is inside grid cell ijk
// Updates k in argument
// Returns false if no valid k found
bool ContrailManager::find_k_inside(const Geo3D& loc, const IDX2& ij, int& k) {
    for (int kTrial = kds; kTrial < kde+1; kTrial++) {
        if (loc.alt >= *Z_AT_W.get(ij.i, ij.j, kTrial)
            && loc.alt < *Z_AT_W.get(ij.i, ij.j, kTrial+1)) {
            k = kTrial;
            return true;
        }
    }
    // Else, no valid k found
    return false;
}

// Finds the index k such that grid centre altitude at k is less than loc.alt and grid centre
// altitude at k+1 is greater than loc.alt
// Updates k in argument
// Returns false if no valid k found
bool ContrailManager::find_k_below(const Geo3D& loc, const IDX2& ij, int& k) {
    for (int kTrial = kds; kTrial < kde; kTrial++) {
        if (loc.alt >= *Z.get(ij.i, ij.j, kTrial)
            && loc.alt < *Z.get(ij.i, ij.j, kTrial+1)) {
            k = kTrial;
            return true;
        }
    }
    // Else, no valid k found
    return false;
}

// Finds inverse-distance weights for the Interp::points and updates interp
void ContrailManager::find_interp_weights(const Geo3D& loc, Interp& interp) {
    int numInterpPoints = interp.points.size();
    std::vector<float> dists(numInterpPoints);

    // Find distances
    bool anyZero = false;
    for (int i = 0; i < numInterpPoints; i++) {
        dists[i] = cart_dist(loc, ijk_to_loc(interp.points[i]));
        if (dists[i] == 0) anyZero = true;
    }
    
    // Find weights
    float totalWeight = 0;
    if (anyZero) {
        for (int i = 0; i < numInterpPoints; i++) {
            interp.weights[i] = (dists[i] == 0) ? 1 : 0;
            totalWeight += interp.weights[i];
        }
    }
    else {
        for (int i = 0; i < numInterpPoints; i++) {
            interp.weights[i] = 1/dists[i];
            totalWeight += interp.weights[i];
        }
    }
    // Scale weights
    for (int i = 0; i < numInterpPoints; i++) {
        interp.weights[i] /= totalWeight;
    }
}

// Advect a location (loc) for duration in seconds
// Updates loc
// Returns false if loc is outside or goes outside the grid
bool ContrailManager::advect_loc(Geo3D& loc, const float duration_s) {
    bool inGrid;
    // Find interp points
    Interp interp;
    inGrid = find_interp(loc, interp);
    if (!inGrid) {return inGrid;}

    int numInterpPoints = interp.points.size();
    // Values at loc
    float u_loc, v_loc, w_loc = 0;
    // Find values at loc
    for (int i = 0; i < numInterpPoints; i++) {
        u_loc += *U.get(interp.points[i]) * interp.weights[i];
        v_loc += *V.get(interp.points[i]) * interp.weights[i];
        w_loc += *W.get(interp.points[i]) * interp.weights[i];
    }
    // Advect in longitude
    loc.lon += u_loc * duration_s / ((EARTH_RADIUS_M + loc.alt) * cos(loc.lat)) * DEG_PER_RAD;
    // Wrap around the Earth
    wrap_WE(loc.lon);

    // Advect in latitude
    loc.lat += v_loc * duration_s / (EARTH_RADIUS_M + loc.alt) * DEG_PER_RAD;
    // Reflect at poles
    wrap_SN(loc.lon, loc.lat);

    // Advect in altitude
    loc.alt += w_loc * duration_s;

    // Check if still in grid
    inGrid = find_interp(loc, interp);
    return inGrid;
}