#ifndef SEGMENTCONTAINER_H
#define SEGMENTCONTAINER_H

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <fstream>
#include <omp.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#ifdef WITH_COCIP
#include <CoCiP++/CoCiP.h>
#include <CoCiP++/params.h>
#include "SegmentCoCiP.h"
#include "SerializeCoCiP.h"
#endif
#include "PlumeModels.h"
#include "Domain.h"
#include "mapTypes.h"
#include "timekeeping.h"
#include "FlightInputs.h"
#include "SerializeSegment.h"
#include "CMLog.h"

// Virtual contrail segment container structure
struct ISegmentContainer {
    // Maximum age of a contrail segment (s); set by ContrailManager
    float maxContrailAge_s;
    // Maximum ratio of double-counted water vapour mass in contrail plume to water vapour mass in
    // grid cell (); set by ContrailManager
    float maxAccumVapRatio;

    std::shared_ptr<Domain> domain; // Pointer to the Contrail Manager's domain

#ifdef WITH_COCIP
    std::shared_ptr<Params> cocipParams; // Pointer to CoCiP Params object if using
#endif

    // Virtual destructor
    virtual ~ISegmentContainer() = default;

    // Return number of segments in container
    virtual size_t getSize() const = 0;

    // Add segment to container; assumes segment has been checked to be inside domain
    virtual void addItem(const FlightInputs& flightInputs) = 0;
    
    // Evolve all segment plumes
    virtual void evolvePlumes(const CMTime& startTime, const CMTime& stopTime) = 0;

    // Advect all segments and remove if out of bounds
    virtual void advectSegments(const CMTime& startTime, const CMTime& stopTime) = 0;

    // Dump old, massive, or dead segments
    virtual void dump(const CMTime& stopTime) = 0;

    // Construct QIcontrail using live segment data
    virtual void constructQIcontrail() = 0;

    // Save segments to file with currTime in name
    virtual void save(const CMTime& currTime, const PlumeModels::Model plumeModel) = 0;

    // Load segments from file with time in name
    virtual void load(const CMTime& time, const PlumeModels::Model plumeModel) = 0;
};

// Plume model-specific segment container structure
// Holds segments in a private vector of SegmentType
template <typename SegmentType>
struct SegmentContainer : public ISegmentContainer {
private:
    std::vector<SegmentType> vec;

    // Returns a new object of SegmentType; allows plume model-specific initialisation
    inline SegmentType newSegmentInstance(const FlightInputs& flightInputs);

    // Update isOld flag for each segment if past age threshold (maxContrailAge_s) at a given time
    // (parallelised)
    void flagOldSegments(const CMTime& time) {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            if ((time - seg.birthTime).to_s() > maxContrailAge_s) {
                seg.isOld = true;
            }
        }
    }

    // Updates isTooMassive flag for each segment if past size threshold (parallelised)
    void flagTooMassiveSegments() {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            IDX3<int> ijkCurr = domain->loc_to_ijk(seg.centre);

            double gridVapourMass = domain->QV.get(ijkCurr)
                                    * domain->DRYMASS.get(ijkCurr);

            if (seg.M_v_accum / gridVapourMass > maxAccumVapRatio) {
                seg.isTooMassive = true;
            }
        }
    }

public:
    // Return number of segments in container
    size_t getSize() const override {
        return vec.size();
    }

    // Add segment to container; assumes segment has been checked to be inside domain
    void addItem(const FlightInputs& flightInputs) override {
        // Add to vector
        #pragma omp critical
        {
            vec.push_back(newSegmentInstance(flightInputs));
        }

        //CM_LogWrite("Segment created with birth time: " + newSeg.birthTime.asString());
        //CM_LogWrite("Centre location: " + newSeg.centre.asString());
        //CM_LogWrite("Length: " + std::to_string(newSeg.length));
    }

    // Evolve all segment plumes (parallelised)
    void evolvePlumes(const CMTime& startTime, const CMTime& stopTime) override {
        // Using schedule(guided) to balance work when segments take different computational time
        // to evolve
        #pragma omp parallel for schedule(guided)
        for (SegmentType& seg : vec) {
            // Check segment is in bounds in case findDependentLocs failed to find centre
            if (!seg.outOfBounds) {
                seg.evolve(startTime, stopTime);
            }
        }
    }

    // Advect all segments and remove if out of bounds (parallelised)
    void advectSegments(const CMTime& startTime, const CMTime& stopTime) override {
        CM_LogWrite("Advecting segments");
        
        size_t numOOB = 0;
        #pragma omp parallel for reduction(+ : numOOB)
        for (SegmentType& seg : vec) {
            seg.advect(startTime, stopTime);
            if (seg.outOfBounds) {
                numOOB += 1;
            }
        }

        CM_LogWrite(std::format("Number out of bounds: {}", numOOB));

        std::erase_if(
            vec,
            [](const SegmentType& seg) {
                return seg.outOfBounds;
            }
        );
    }

    // Dump old, massive, or dead segments (parallelised)
    void dump(const CMTime& stopTime) override {
        CM_LogWrite("Dumping old, massive, and dead segments");
        
        // Flag old segments
        flagOldSegments(stopTime);

        // Flag massive segments
        flagTooMassiveSegments();

        size_t numOld = 0, numMassive = 0, numDead = 0;
        #pragma omp parallel for reduction(+ : numOld, numMassive, numDead)
        for (const SegmentType& seg : vec) {
            if (seg.isOld) {
                numOld++;
            }
            if (seg.isTooMassive) {
                numMassive++;
            }
            if (seg.isDead) {
                numDead++;
            }
        }

        CM_LogWrite(std::format("Number of old: {}, number of massive: {}, number of dead: {}",
            numOld, numMassive, numDead));

        if (domain->twoWayCoupling) {
            // Dump if old, massive, or dead
            #pragma omp parallel for
            for (SegmentType& seg : vec) {
                if (seg.shouldBeDumped()) {
                    seg.dump();
                }
            }
        }

        size_t numBefore = vec.size();

        // Remove old or dead
        std::erase_if(
            vec,
            [](const SegmentType& seg) {
                return (seg.shouldBeDumped());
            }
        );
        
        size_t numAfter = vec.size();
        
        CM_LogWrite(std::format("Number removed: {}", numBefore - numAfter));
    }

    // Construct QIcontrail using live segment data (parallelised)
    void constructQIcontrail() override {
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            seg.addToQIcontrail();
        }
    }

    // Save segments to file with currTime in name
    void save(const CMTime& currTime, const PlumeModels::Model plumeModel) override {
        std::string filename = "ContrailManagerSegments_" + currTime.asFileFriendlyString() + ".bin";

        CM_LogWrite("Saving segments to " + filename);

        {
            std::ofstream os(filename, std::ios::binary);
            if (!os) {
                CM_RaiseError("Cannot open file for writing: " + filename, __FILE__, __LINE__);
            }
            cereal::BinaryOutputArchive archive(os);
            // Write plume model ID
            archive(plumeModel.ID);
            // Then vector
            archive(vec);
        }
    }

    // Load segments from file with time in name
    void load(const CMTime& time, const PlumeModels::Model plumeModel) {
        std::string filename = "ContrailManagerSegments_" + time.asFileFriendlyString() + ".bin";

        CM_LogWrite("Loading segments from " + filename);

        int loadedPlumeModelID;

        {
            std::ifstream is(filename, std::ios::binary);
            if (!is) {
                CM_RaiseError("Cannot open file for reading: " + filename, __FILE__, __LINE__);
            }
            cereal::BinaryInputArchive archive(is);
            archive(loadedPlumeModelID);

            if (loadedPlumeModelID != plumeModel.ID) {
                CM_RaiseError(
                    std::format(
                        "File was written with plume model ID {}, but config specifies {}",
                        loadedPlumeModelID, plumeModel.name
                    ),
                    __FILE__, __LINE__
                );
            }

            archive(vec);
        }

        size_t numLoaded = vec.size();

        CM_LogWrite(std::format("Loaded {} segments from file.", numLoaded));

        // Set domain pointer for each segment, then check if out of bounds
        #pragma omp parallel for
        for (SegmentType& seg : vec) {
            seg.domain = domain;

            IDX3<int> ijk;
            if (!(domain->loc_to_ijk(seg.centre, ijk)
                  && domain->can_do_interp(seg.front)
                  && domain->can_do_interp(seg.back))) {
                seg.outOfBounds = true;
            }
        }

        std::erase_if(
            vec,
            [](const SegmentType& seg) {
                return seg.outOfBounds;
            }
        );

        size_t numAfter = vec.size();

        CM_LogWrite(std::format("Number out of bounds: {}", numLoaded - numAfter));

        // Find highest ID
        uint64_t maxID = 0;
        if (!vec.empty()) {
            auto it = std::ranges::max_element(vec, {}, &SegmentType::ID);
            maxID = it->ID;
        }
        // Set Segment struct ID counter to the next value
        Segment::setIDCounter(maxID + 1);

    }
};

#ifdef WITH_COCIP
template <>
inline SegmentCoCiP SegmentContainer<SegmentCoCiP>::newSegmentInstance(
    const FlightInputs& flightInputs
) {
    // Initialise with additional pointer to common params
    return SegmentCoCiP(flightInputs, domain, cocipParams);
}
#endif

#endif