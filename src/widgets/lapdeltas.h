#pragma once

#include "imgui.h"
#include "implot.h"

#include "constants.h"
#include "f123constants.h"
#include "packets/lapdata.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace F123;

namespace
{
    constexpr int sMaxDataCount = 10000;
}

class CLapDeltas
{
public:
    CLapDeltas()
    {
        // Axis config
        mAxisFlagsX |= ImPlotAxisFlags_AutoFit;
        mAxisFlagsX |= ImPlotAxisFlags_NoGridLines;
        mAxisFlagsX |= ImPlotAxisFlags_NoTickMarks;
        mAxisFlagsX |= ImPlotAxisFlags_NoMenus;
        mAxisFlagsX |= ImPlotCond_Always;

        mAxisFlagsY |= ImPlotAxisFlags_NoMenus;
        mAxisFlagsY |= ImPlotCond_Always;
        // For testing
        // SRaceWeekend dummyRace1;
        // dummyRace1.trackName = "TrackName1";
        // SRaceWeekend dummyRace2;
        // dummyRace2.trackName = "TrackName2";

        // SSessionData practiceSession1;
        // practiceSession1.sessionType = ESessionType::P1;

        // SSessionData practiceSession2;
        // practiceSession2.sessionType = ESessionType::P2;

        // SSessionData raceSession;
        // raceSession.sessionType = ESessionType::R;

        // for (int i = 1; i < 10; ++i)
        // {
        //     SLap newLap;
        //     newLap.lapNumber = i;
        //     newLap.sector1MS = i * 10000;
        //     newLap.sector2MS = i * 11000;
        //     newLap.totalLapTime = i * 20000;

        //     practiceSession1.laps.push_back(newLap);
        //     practiceSession1.fastestLapNum = 1;
        //     practiceSession2.laps.push_back(newLap);
        //     practiceSession2.fastestLapNum = 1;

        //     raceSession.laps.push_back(newLap);
        //     raceSession.fastestLapNum = 1;
        // }

        // dummyRace1.sessions.push_back(practiceSession1);
        // dummyRace1.sessions.push_back(practiceSession2);
        // dummyRace1.sessions.push_back(raceSession);
        // dummyRace2.sessions.push_back(practiceSession1);
        // dummyRace2.sessions.push_back(practiceSession2);
        // dummyRace2.sessions.push_back(raceSession);

        // mRaces.push_back(dummyRace1);
        // mRaces.push_back(dummyRace2);
    }

    void SetLapData(const SLapData &myLapData, const SLapData &carBehindLapData)
    {
        SetDeltaData(myLapData, carBehindLapData);
    }

    void ResetLapData()
    {
        mDataCount = 0;
        mMaxDelta = 0.01f;
        mMinDelta = FLT_MAX;
        memset(mXaxis, 0.0f, sizeof(mXaxis));
        memset(mDeltas, 0.0f, sizeof(mDeltas));
    }

    void ShowDeltas(const ImVec2 spaceAvail) const
    {
        if (mCarRacePosition > 1)
        {
            if (ImPlot::BeginPlot("Delta leader", ImVec2((spaceAvail.x / 2), (spaceAvail.y / 2)), ImPlotFlags_NoLegend))
            {
                // Configure
                ImPlot::SetupAxisLimits(ImAxis_Y1, mMinDelta == FLT_MAX ? 0.0f : mMinDelta / 2.0f, mMaxDelta * 1.50f, ImPlotCond_Always);
                ImPlot::SetupAxis(ImAxis_Y1, "Seconds", mAxisFlagsY);
                ImPlot::SetupAxis(ImAxis_X1, nullptr, mAxisFlagsX);
                ImPlot::SetNextLineStyle(red, 3);
                ImPlot::PlotLine("Delta Leader (s)", mXaxis, mDeltas, mDataCount);
                ImPlot::EndPlot();
            }
        }
        else
        {
            if (ImPlot::BeginPlot("Delta to car behind", ImVec2((spaceAvail.x / 2), (spaceAvail.y / 2)), ImPlotFlags_NoLegend))
            {
                // Configure
                ImPlot::SetupAxisLimits(ImAxis_Y1, mMinDelta == FLT_MAX ? 0.0f : mMinDelta / 2.0f, mMaxDelta * 1.50f, ImPlotCond_Always);
                ImPlot::SetupAxis(ImAxis_Y1, "Seconds", mAxisFlagsY);
                ImPlot::SetupAxis(ImAxis_X1, nullptr, mAxisFlagsX);
                ImPlot::SetNextLineStyle(green, 3);
                ImPlot::PlotLine("Delta Car behind (s)", mXaxis, mDeltas, mDataCount, (ImPlotLineFlags_SkipNaN));
                ImPlot::EndPlot();
            }
        }
    }

private:
    ImPlotAxisFlags mAxisFlagsX{0};
    ImPlotAxisFlags mAxisFlagsY{0};
    // Deltas
    uint8_t mCarRacePosition{1};
    uint32_t mCurrentLapTimeInMS{0};
    float mDeltas[sMaxDataCount]{};
    float mXaxis[sMaxDataCount]{};
    int mDataCount{0};

    float mMaxDelta{0.01f};
    float mMinDelta{FLT_MAX};

    void SetDeltaData(const SLapData &myLapData, const SLapData &carBehindLapData)
    {
        const auto currPosition = myLapData.carPosition;
        const auto currLapTime = myLapData.currentLapTimeInMS;

        // Took or lost leader position, new lap, max data reached
        if ((currPosition == 1 && mCarRacePosition > 1) || (mCarRacePosition == 1 && currPosition > 1) || (mDataCount >= sMaxDataCount) || (currLapTime < mCurrentLapTimeInMS))
        {
            ResetLapData();
        }
        mCarRacePosition = currPosition;
        mCurrentLapTimeInMS = currLapTime;

        uint16_t deltaMS;
        if (mCarRacePosition > 1)
        {
            deltaMS = myLapData.deltaToRaceLeaderInMS;
        }
        else
        {
            deltaMS = carBehindLapData.deltaToCarInFrontInMS;
        }

        float deltaSecs = deltaMS / 1000.0f;
        float currLapTimeSecs = currLapTime / 1000.0f;

        // Outlier when car behind gets overtaken
        if (deltaMS == 0.0f || (mDataCount != 0 && deltaSecs > 5.0f && ((deltaSecs > (mDeltas[mDataCount - 1] * 2.5f)))))
        {
            return;
        }

        mDeltas[mDataCount] = deltaSecs;
        mXaxis[mDataCount] = currLapTimeSecs;

        // Chart Y-axis min/max
        if (deltaSecs > mMaxDelta)
        {
            mMaxDelta = deltaSecs;
        }
        if (deltaSecs < mMinDelta)
        {
            mMinDelta = deltaSecs;
        }
        mDataCount++;
    }
};