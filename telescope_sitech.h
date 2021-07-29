/*******************************************************************************
 Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#ifndef SCOPESITECH_H
#define SCOPESITECH_H
#include "libindi/indiguiderinterface.h"
#include "libindi/inditelescope.h"
#include "libindi/indicontroller.h"
#include "libindi/indidevapi.h"
#include "libindi/indicom.h"
#include "libindi/baseclient.h"

class ScopeSiTech : public INDI::Telescope, public INDI::GuiderInterface
{
public:
    ScopeSiTech();
    virtual ~ScopeSiTech();
    virtual bool initProperties();
    virtual bool ReadScopeStatus();
    virtual void ISGetProperties (const char *dev);
    virtual bool updateProperties();

    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);

protected:
    virtual bool Abort();

    virtual IPState GuideNorth(uint32_t ms) override;
    virtual IPState GuideSouth(uint32_t ms) override;
    virtual IPState GuideEast(uint32_t ms) override;
    virtual IPState GuideWest(uint32_t ms) override;

    bool Goto(double,double);
    bool Park();
    bool UnPark();
    bool Sync(double ra, double dec);

    const char *getDefaultName();
    bool Handshake() override;

    // Parking
    virtual bool SetCurrentPark();
    virtual bool SetDefaultPark();

private:
    bool SetUpVarsFromReturnString(char * ScopeAnswer, bool PrintBools);
    bool setSiTechTracking(bool enable, bool isSidereal, double raRate, double deRate);
    int currentTrackMode;
    unsigned int DBG_SCOPE;
    char * GetStringFromSerial(char * Send);

    double currentRA;
    double currentDEC;
    double currentAlt;
    double currentAz;
    double axisPositionDegsPrimary;
    double axisPositionDegsSecondary;
    double scopeSiderealTime;
    double scopeJulianDay;
    double scopeTime;

    double targetRA;
    double targetDEC;
    bool IsInitialized;
    bool IsTracking;
    bool IsSlewing;
    bool IsParking;
    bool IsParked;
    bool IsLookingEast;
    bool IsInBlinky;
    bool IsCommunicatingWithController;

    double guiderNSTarget[2];
    double guiderEWTarget[2];

    // Tracking Mode
    ISwitch TrackModeS[4];
    ISwitchVectorProperty TrackModeSP;
    // Tracking Rate
    INumber GuideRateN[2];
    INumberVectorProperty GuideRateNP;
};

#endif // SCOPESITECH_H
