/*
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
/*
localport
192.168.51.122
7624
192.168.51.51
1956
1929
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <libnova/libnova.h>

#include <memory>

#include "telescope_sitech.h"

using namespace std;
static std::unique_ptr<ScopeSiTech> telescope_sitech(new ScopeSiTech());
#define MYSCOPE "SiTechScopeA"
#define RA_AXIS         0
#define DEC_AXIS        1
#define GUIDE_NORTH     0
#define GUIDE_SOUTH     1
#define GUIDE_WEST      0
#define GUIDE_EAST      1
/*
 * Six mandatory INDI functions called by the INDI framework that must be declared in ALL INDI drivers:
 */
/**************************************************************************************
** Return properties of device.
***************************************************************************************/
void ISGetProperties(const char *dev)
{
        telescope_sitech->ISGetProperties(dev);
}
/**************************************************************************************
** Process new switch from client
***************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        telescope_sitech->ISNewSwitch(dev, name, states, names, num);
}
/**************************************************************************************
** Process new text from client
***************************************************************************************/
void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        telescope_sitech->ISNewText(dev, name, texts, names, num);
}
/**************************************************************************************
** Process new number from client
***************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        telescope_sitech->ISNewNumber(dev, name, values, names, num);
}
/**************************************************************************************
** Process new blob from client
***************************************************************************************/
void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}
/**************************************************************************************
** Process new blob from client
***************************************************************************************/
void ISSnoopDevice (XMLEle *root)
{
   telescope_sitech->ISSnoopDevice(root);
}
const char * ScopeSiTech::getDefaultName()
{
    fprintf(stderr,"ScopeSiTech::getDefaultName called\n");
    return (char *) MYSCOPE;
}
ScopeSiTech::ScopeSiTech()
{
    fprintf(stderr,"Hello there! ScopeSiTech::ScopeSitech here.\n");
    currentRA=0;
    currentDEC=60;

    DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");   

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT,4);
    setTelescopeConnection(CONNECTION_TCP);

    /* initialize random seed: */
      srand ( time(NULL) );
}
ScopeSiTech::~ScopeSiTech()
{

}
bool ScopeSiTech::initProperties()
{
    fprintf(stderr,"initProperties enter\n");
    INDI::Telescope::initProperties();

    /* How fast do we guide compared to sidereal rate */
    IUFillNumber(&GuideRateN[RA_AXIS], "GUIDE_RATE_WE", "W/E Rate", "%g", 0, 1, 0.1, 0.3);
    IUFillNumber(&GuideRateN[DEC_AXIS], "GUIDE_RATE_NS", "N/S Rate", "%g", 0, 1, 0.1, 0.3);
    IUFillNumberVector(&GuideRateNP, GuideRateN, 2, getDeviceName(), "GUIDE_RATE", "Guiding Rate", MOTION_TAB, IP_RW, 0, IPS_IDLE);

    IUFillSwitch(&SlewRateS[SLEW_GUIDE], "SLEW_GUIDE", "Guide", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_CENTERING], "SLEW_CENTERING", "Centering", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_FIND], "SLEW_FIND", "Find", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_MAX], "SLEW_MAX", "Max", ISS_ON);
    IUFillSwitchVector(&SlewRateSP, SlewRateS, 4, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Tracking Mode
    IUFillSwitch(&TrackModeS[TRACK_SIDEREAL], "TRACK_SIDEREAL", "Sidereal", ISS_OFF);
    IUFillSwitch(&TrackModeS[TRACK_SOLAR], "TRACK_SOLAR", "Solar", ISS_OFF);
    IUFillSwitch(&TrackModeS[TRACK_LUNAR], "TRACK_LUNAR", "Lunar", ISS_OFF);
    IUFillSwitch(&TrackModeS[TRACK_CUSTOM], "TRACK_CUSTOM", "Custom", ISS_OFF);
    IUFillSwitchVector(&TrackModeSP, TrackModeS, 4, getDeviceName(), "TELESCOPE_TRACK_MODE", "Track Mode", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Custom Tracking Rate
    IUFillNumber(&TrackRateN[0],"TRACK_RATE_RA","RA (arcsecs/s)","%.6f",-16384.0, 16384.0, 0.000001, 15.041067);
    IUFillNumber(&TrackRateN[1],"TRACK_RATE_DE","DE (arcsecs/s)","%.6f",-16384.0, 16384.0, 0.000001, 0);
    IUFillNumberVector(&TrackRateNP, TrackRateN,2,getDeviceName(),"TELESCOPE_TRACK_RATE","Track Rates", MAIN_CONTROL_TAB, IP_RW,60,IPS_IDLE);

    initGuiderProperties(getDeviceName(), MOTION_TAB);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);

    return true;
}
void ScopeSiTech::ISGetProperties (const char *dev)
{
    /* First we let our parent populate */
    INDI::Telescope::ISGetProperties(dev);

    fprintf(stderr,"From ISGetProperties, is device connected?\n");
    if(isConnected())
    {
	fprintf(stderr,"Absolutely!\n");
        defineProperty(&GuideNSNP);
        defineProperty(&GuideWENP);
        defineProperty(&GuideRateNP);
    }
    else fprintf(stderr,"No!\n");


    return;
}
bool ScopeSiTech::updateProperties()
{
    INDI::Telescope::updateProperties();

    fprintf(stderr,"From updateProperties, is device connected?\n");
    if (isConnected())
    {
        if (IsTracking)
        {
	    fprintf(stderr,"Indeed!\n");
            TrackModeSP.s = IPS_OK;
        }
        else
        {
            TrackModeSP.s = IPS_IDLE;
        }

        defineProperty(&TrackModeSP);
        defineProperty(&TrackRateNP);

        defineProperty(&GuideNSNP);
        defineProperty(&GuideWENP);
        defineProperty(&GuideRateNP);

    }
    else
    {
        fprintf(stderr,"Unfortunately not!\n");
        deleteProperty(TrackModeSP.name);
        deleteProperty(TrackRateNP.name);

        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);
        deleteProperty(GuideRateNP.name);
    }

    return true;
}
#define MAXSOCKETBUFLEN 512
#define SITECHTIMEOUT 3
static char SendBuf[MAXSOCKETBUFLEN];
static char RcvBuf[MAXSOCKETBUFLEN];
bool ScopeSiTech::Handshake()
{
    int rc=0, nbytes_written=0, nbytes_read=0;
    memset(SendBuf,'\0',MAXSOCKETBUFLEN);
    memset(RcvBuf,'\0',MAXSOCKETBUFLEN);
    strncpy(SendBuf, "ReadScopeStatus\r\n", MAXSOCKETBUFLEN);
    fprintf(stderr,"in Handshake: SendBuf=%s", SendBuf);

    if ( (rc = tty_write_string(PortFD, SendBuf, &nbytes_written)) != TTY_OK)
    {
        fprintf(stderr,"in Handshake Write Error nbytes written=%d", nbytes_written);
        return false;
    }
    fprintf(stderr,"In Handshake success:\n nbytes written=%d rc=%d\n", nbytes_written, rc);

    int rcr = tty_read_section(PortFD, RcvBuf, '\n', SITECHTIMEOUT, &nbytes_read);
    if ( rcr != TTY_OK)
    {
        fprintf(stderr,"in Handshake Error: nbytes read=%d rcr=%d rs=%s", nbytes_read,rcr,RcvBuf);
        return false;
    }
    fprintf(stderr,"in Handshake B4 SetupVars:\n nbytesread=%d rcr=%d rs=%s\n", nbytes_read,rcr,RcvBuf);
    SetUpVarsFromReturnString(RcvBuf, true);
    fprintf(stderr,"inHandshake AfterSetupVars:\n RA=%f rcvBuf=%s\n",currentRA,RcvBuf);
    if (!IsCommunicatingWithController)
    {
        fprintf(stderr,"In Handshake: No Communication From SiTechExe To Controller!\n");
    }
    else if(IsInBlinky)
    {
        fprintf(stderr,"In Handshake: Controller in manual (Blinky) mode!\n");
    }
    else
    {
        fprintf(stderr,"In Handshake: SiTech is in %d mode!\n", TrackState );
    }

    SetParked(IsParked);
    SetTimer(POLLMS);

    return true;
}
char * ScopeSiTech::GetStringFromSerial(char * Send)
{
    int rc=0, nbytes_written=0, nbytes_read=0;
    memset(SendBuf,'\0',MAXSOCKETBUFLEN);
    memset(RcvBuf,'\0',MAXSOCKETBUFLEN);
    strncpy(SendBuf, Send, MAXSOCKETBUFLEN-2);
    strcat(SendBuf,"\r\n");

    if ( (rc = tty_write_string(PortFD, SendBuf, &nbytes_written)) != TTY_OK)
    {
        fprintf(stderr,"Error writing to the SiTechExe TCP server.");
        return NULL;
    }
    int rcr = tty_read_section(PortFD, RcvBuf, '\n', SITECHTIMEOUT, &nbytes_read);
    if ( rcr != TTY_OK)
    {
        fprintf(stderr,"GetStringFromSerial ErrRead nbytsrd=%d rc=%drcr=%d RcvStrg=%s",nbytes_read,rc,rcr,RcvBuf);

        return NULL;
    }
    if(nbytes_read > 0 && nbytes_read < 4)
    {
        rcr = tty_read_section(PortFD, RcvBuf, '\n', SITECHTIMEOUT, &nbytes_read);
        if ( rcr != TTY_OK)
        {
            fprintf(stderr,"GetStringFromSerial ErrRead2 nbytsrd=%d rc=%drcr=%d RcvStrg=[%s]",nbytes_read,rc,rcr,RcvBuf);
            return NULL;
        }
    }
    fprintf(stderr,"GSFS Second Normal Read:\n nbytsrd=%d rc=%drcr=%d RcvStrg=[%s]\n",nbytes_read,rc,rcr,RcvBuf);

    return RcvBuf;
}
static char MessageFromScope[1500];
static char ErrorMessage[2000];
static int LastScopeStt = -1;
bool ScopeSiTech::SetUpVarsFromReturnString(char * ScopeAnswer, bool PrintBools)
{
    if(ScopeAnswer == NULL)
    {
	fprintf(stderr,"No response for %s.\n",ScopeAnswer);
    	return false;
    }
    int Len = strlen(ScopeAnswer);
    if(Len < 3) return false;
    int ScopeStt = atoi(ScopeAnswer);
    IsInitialized = ((ScopeStt & 1) > 0);
    IsTracking = ((ScopeStt & 2) > 0);
    IsSlewing = ((ScopeStt & 4) > 0);
    IsParking = ((ScopeStt & 8) > 0);
    IsParked = ((ScopeStt & 16) > 0);
    IsLookingEast = ((ScopeStt & 32) > 0);
    IsInBlinky = ((ScopeStt & 64) > 0);
    IsCommunicatingWithController = ((ScopeStt & 128) == 0);

    if(PrintBools && LastScopeStt != ScopeStt)
    {
        fprintf(stderr,"In SetUpVarsFromReturnString:\nScopeStt=%d IsInit=%d IsTrack=%d IsSlew=%d IsParking=%d IsParked=%d IsLookingEast=%d IsInBlinky=%d IsComm=%d\n", ScopeStt, IsInitialized, IsTracking, IsSlewing, IsParking, IsParked, IsLookingEast, IsInBlinky, IsCommunicatingWithController );
    }

    char * ptr = ScopeAnswer;
    int NextSemColonLoc=0;
    while(NextSemColonLoc < Len)
    {
       if(ScopeAnswer[NextSemColonLoc] == ';') break;
       NextSemColonLoc++;
    }
    ptr = ScopeAnswer + NextSemColonLoc;
    Len --; if (Len < 1) Len = 1;
    size_t offset = 0;
    if(ptr - ScopeAnswer < Len) currentRA = stod(ptr + 1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) currentDEC = stod(ptr + 1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) currentAlt = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) currentAz = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) axisPositionDegsPrimary = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) axisPositionDegsSecondary = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) scopeSiderealTime = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) scopeJulianDay = stod(ptr+1, &offset); 
    ptr += offset + 1;
    if(ptr - ScopeAnswer < Len) scopeTime = stod(ptr+1,  &offset); 
    ptr += offset + 1;
    if(strlen(ptr) > 3)strncpy(MessageFromScope,ptr,499);
    if (IsParking) TrackState = SCOPE_PARKING;
    else if (IsParked) TrackState = SCOPE_PARKED;
    else if(IsSlewing) TrackState = SCOPE_SLEWING;
    else if(IsTracking) TrackState = SCOPE_TRACKING;
    else TrackState = SCOPE_IDLE;
    if(PrintBools && LastScopeStt != ScopeStt)
    {
        fprintf(stderr,"TrackState=%d", TrackState );
    }
    LastScopeStt = ScopeStt;
    return true;
}
static bool inReadScopeStatus = false;
bool ScopeSiTech::ReadScopeStatus()
{
    fprintf(stderr,"Entering ReadScopeStatus!\n");
    if(inReadScopeStatus) return false;
    inReadScopeStatus = true;
    SetUpVarsFromReturnString(GetStringFromSerial((char *)"ReadScopeStatus"), true);
    static struct timeval ltv;
    struct timeval tv;
    double dt=0, dx=0, dy=0, ra_guide_dt=0, dec_guide_dt=0;
    static double last_dx=0, last_dy=0;
    char RA_DISP[64], DEC_DISP[64], RA_GUIDE[64], DEC_GUIDE[64], RA_TARGET[64], DEC_TARGET[64];

    fprintf(stderr,"From ReadScopeStatus: Are we tracking?\n");
    if (IsTracking)
    {
        fprintf(stderr,"Yes, we are tracking.\n");
        IUResetSwitch(&TrackModeSP);
        TrackModeSP.s = IPS_OK;
    }
    else
    {
        fprintf(stderr,"No, we are not tracking.\n");
        IUResetSwitch(&TrackModeSP);
        TrackModeSP.s = IPS_IDLE;
    }
    /* update elapsed time since last poll, don't presume exactly POLLMS */
    gettimeofday (&tv, NULL);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0) ltv = tv;

    dt = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec)/1e6;
    ltv = tv;

    fs_sexa(RA_DISP, fabs(dx), 2, 3600 );
    fs_sexa(DEC_DISP, fabs(dy), 2, 3600 );

    fs_sexa(RA_GUIDE, fabs(ra_guide_dt), 2, 3600 );
    fs_sexa(DEC_GUIDE, fabs(dec_guide_dt), 2, 3600 );

    fs_sexa(RA_TARGET, targetRA, 2, 3600);
    fs_sexa(DEC_TARGET, targetDEC, 2, 3600);


    if ((dx!=last_dx || dy!=last_dy || ra_guide_dt || dec_guide_dt))
    {
        last_dx=dx;
        last_dy=dy;
    }

    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, currentRA, 2, 3600);
    fs_sexa(DecStr, currentDEC, 2, 3600);

    fprintf(stderr, "From ReadScopeStatus: Current RA: %s Current DEC: %s\n", RAStr, DecStr);

    NewRaDec(currentRA, currentDEC);
    inReadScopeStatus = false;
    return true;
}
bool ScopeSiTech::Goto(double r,double d)
{
    fprintf(stderr,"Entering Goto function.\n");
    if (IsParked) return false;

    targetRA=r;
    targetDEC=d;
    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, targetRA, 2, 3600);
    fs_sexa(DecStr, targetDEC, 2, 3600);

   ln_equ_posn lnradec;
   lnradec.ra = (currentRA * 360) / 24.0;
   lnradec.dec =currentDEC;

   char sStr[128];
   sprintf(sStr,"GoTo %f %f",r,d);
   SetUpVarsFromReturnString(GetStringFromSerial((char *)sStr), true);

   EqNP.s    = IPS_BUSY;

   return true;
}
bool ScopeSiTech::Sync(double ra, double dec)
{
    char cStr[1164];
    //the zero is call up the sitech init window.  1 = offset instantaneous, 2 = load calibration instantaneos
    sprintf(cStr,"Sync %f %f 0",ra,dec);
    SetUpVarsFromReturnString(GetStringFromSerial((char *)cStr), true);
    if(strstr(MessageFromScope,"Accepted") == NULL)
    {
        fprintf(stderr, "Sync is rejected. Reason=%s",MessageFromScope);
        DEBUG(INDI::Logger::DBG_SESSION,ErrorMessage);
        return false;
    }
    currentRA  = ra;
    currentDEC = dec;
    fprintf(stderr, "Sync is successful.\n");
    DEBUG(INDI::Logger::DBG_SESSION,"Sync is successful.");
    EqNP.s    = IPS_OK;
    return true;
}
bool ScopeSiTech::Park()
{
    //the zero is regular park, could do 1 or 2 as well.
    SetUpVarsFromReturnString(GetStringFromSerial((char *)"Park 0"), true);
    if(strstr(MessageFromScope,"Park") == NULL)
    {
        fprintf(stderr, "Park is rejected. Reason=%s",MessageFromScope);
        DEBUG(INDI::Logger::DBG_SESSION,ErrorMessage);
        return false;
    }
    fprintf(stderr, "ParkOk. Mess=%s",MessageFromScope);
    DEBUG(INDI::Logger::DBG_SESSION, ErrorMessage);
    return true;
}
bool ScopeSiTech::UnPark()
{
    if (INDI::Telescope::isLocked())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Cannot unpark mount when dome is locking. See: Dome parking policy, in options tab");
        return false;
    }
    if (!IsParked)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Cannot unpark mount when already unparked");
        return false;
    }

    SetUpVarsFromReturnString(GetStringFromSerial((char *)"UnPark"), true);
    if(strstr(MessageFromScope,"UnPark") == NULL)
    {
        sprintf(ErrorMessage, "UnPark is rejected. Reason=%s",MessageFromScope);
        DEBUG(INDI::Logger::DBG_SESSION,ErrorMessage);
        return false;
    }
    fprintf(stderr, "UnParkOk. Mess=%s",MessageFromScope);
    DEBUG(INDI::Logger::DBG_SESSION, ErrorMessage);
    SetParked(false);
    return true;
}
bool ScopeSiTech::setSiTechTracking(bool enable, bool isSidereal, double raRate, double deRate)
{
    int on      = enable ? 1 : 0;
    int ignore  = isSidereal ? 1 : 0;

    char pCMD[MAXRBUF];

    snprintf(pCMD,MAXRBUF, "SetTrackMode %d %d %f %f", on, ignore, raRate, deRate);
    SetUpVarsFromReturnString(GetStringFromSerial((char *)pCMD), true);

    if(strstr(MessageFromScope, "SetTrackMode") == NULL) return false;
    return true;
}
bool ScopeSiTech::ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if(strcmp(dev,getDeviceName())==0)
    {
        // Tracking Rate
        if (!strcmp(name, TrackRateNP.name))
        {
            IUUpdateNumber(&TrackRateNP, values, names, n);
            if (currentTrackMode != TRACK_CUSTOM)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Can only set tracking rate if track mode is custom.");
                TrackRateNP.s = IPS_ALERT;
            }
            else
            {
                setSiTechTracking(true, false, TrackRateN[RA_AXIS].value, TrackRateN[DEC_AXIS].value);
                if(strstr(MessageFromScope,"SetTrackMode") == NULL)
                {
                    fprintf(stderr, "SetTrackMode is rejected. Reason=%s",MessageFromScope);
                    DEBUG(INDI::Logger::DBG_SESSION,ErrorMessage);
                    TrackRateNP.s = IPS_ALERT;
                }
                else
                {
                    TrackRateNP.s = IPS_OK;
                    sprintf(ErrorMessage, "SetTrackMode OK. Mess=%s",MessageFromScope);
                    DEBUG(INDI::Logger::DBG_SESSION, ErrorMessage);
                }
            }

            IDSetNumber(&TrackRateNP, NULL);
            return true;
        }

         if(strcmp(name,"GUIDE_RATE")==0)
         {
             IUUpdateNumber(&GuideRateNP, values, names, n);
             GuideRateNP.s = IPS_OK;
             IDSetNumber(&GuideRateNP, NULL);
             return true;
         }

         if (!strcmp(name,GuideNSNP.name) || !strcmp(name,GuideWENP.name))
         {
             processGuiderProperties(name, values, names, n);
             return true;
         }

    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev,name,values,names,n);
}
bool ScopeSiTech::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0)
    {
        // Tracking Mode
        if (!strcmp(TrackModeSP.name, name))
        {
            int previousTrackMode = IUFindOnSwitchIndex(&TrackModeSP);

            IUUpdateSwitch(&TrackModeSP, states, names, n);

            currentTrackMode = IUFindOnSwitchIndex(&TrackModeSP);

            // Engage tracking?
            bool enable     = (currentTrackMode != -1);
            bool isSidereal = (currentTrackMode == TRACK_SIDEREAL);
            double dRA=0, dDE=0;
            if (currentTrackMode == TRACK_SOLAR)
                dRA = TRACKRATE_SOLAR;
            else if (currentTrackMode == TRACK_LUNAR)
                dRA = TRACKRATE_LUNAR;
            else if (currentTrackMode == TRACK_CUSTOM)
            {
                dRA = TrackRateN[RA_AXIS].value;
                dDE = TrackRateN[DEC_AXIS].value;
            }

            setSiTechTracking(enable, isSidereal, dRA, dDE);

            if(strstr(MessageFromScope,"SetTrackMode") == NULL)
            {
                sprintf(ErrorMessage, "SetTrackMode is rejected. Reason=%s",MessageFromScope);
                DEBUG(INDI::Logger::DBG_SESSION,ErrorMessage);
                TrackModeSP.s = IPS_ALERT;
                if (previousTrackMode != -1)
                {
                    IUResetSwitch(&TrackModeSP);
                    TrackModeS[previousTrackMode].s = ISS_ON;
                }
            }
            else
            {
                sprintf(ErrorMessage, "SetTrackMode OK. Mess=%s",MessageFromScope);
                DEBUG(INDI::Logger::DBG_SESSION, ErrorMessage);
                if(IsTracking) TrackModeSP.s = IPS_OK; else TrackModeSP.s = IPS_IDLE;
            }

            IDSetSwitch(&TrackModeSP, NULL);
            return true;
        }

        // Slew mode
        if (!strcmp (name, SlewRateSP.name))
        {

          if (IUUpdateSwitch(&SlewRateSP, states, names, n) < 0)
              return false;

          SlewRateSP.s = IPS_OK;
          IDSetSwitch(&SlewRateSP, NULL);
          return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev,name,states,names,n);
}
bool ScopeSiTech::Abort()
{
    SetUpVarsFromReturnString(GetStringFromSerial((char *)"Abort"), true);//Stop all motion.
    if(strstr(MessageFromScope,"Abort") == NULL)
    {
        fprintf(stderr, "Abort is rejected. Reason=%s",MessageFromScope);
        return false;
    }
    fprintf(stderr, "Abort OK. Mess=%s",MessageFromScope);
    IUResetSwitch(&TrackModeSP);
    TrackModeSP.s = IPS_IDLE;
    return true;
}
IPState ScopeSiTech::GuideNorth(uint32_t ms)
{
    guiderNSTarget[GUIDE_NORTH] = ms;
    guiderNSTarget[GUIDE_SOUTH] = 0;
    return IPS_BUSY;
}

IPState ScopeSiTech::GuideSouth(uint32_t ms)
{
    guiderNSTarget[GUIDE_SOUTH] = ms;
    guiderNSTarget[GUIDE_NORTH] = 0;
    return IPS_BUSY;

}

IPState ScopeSiTech::GuideEast(uint32_t ms)
{

    guiderEWTarget[GUIDE_EAST] = ms;
    guiderEWTarget[GUIDE_WEST] = 0;
    return IPS_BUSY;

}

IPState ScopeSiTech::GuideWest(uint32_t ms)
{
    guiderEWTarget[GUIDE_WEST] = ms;
    guiderEWTarget[GUIDE_EAST] = 0;
    return IPS_BUSY;

}
bool ScopeSiTech::SetCurrentPark()
{
    SetAxis1Park(currentRA);
    SetAxis2Park(currentDEC);

    return true;
}
bool ScopeSiTech::SetDefaultPark()
{
    // By default set RA to HA
    SetAxis1Park(ln_get_apparent_sidereal_time(ln_get_julian_from_sys()));

    // Set DEC to 90 or -90 depending on the hemisphere
    SetAxis2Park( (LocationN[LOCATION_LATITUDE].value > 0) ? 90 : -90);

    return true;

}
