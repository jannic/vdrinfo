/*
 * vdrinfo.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information 
 *
 *  vdrinfo.c   2008/01/30  Werner Sigrist www.cl-si.de
 */

#include <vdr/plugin.h>
#include <vdr/status.h>


static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Infos from VDR over SVDRP";

class cInfoStatus : public cStatus {
private:
  int iNumDevices;

protected:
  virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);

public:
  cInfoStatus(void);
  int GetDeviceUsedStatus(int iCardIndex);
  int GetDeviceRecStatus(int iCardIndex);
  int GetPrimaryDevice(void);
  int Transferring(int iCardIndex);
  int MaySwitchTransponder(int iCardIndex);
  int IsPrimaryDevice(int nDevice);
  int GetRecStatus(void);
  int iCardIsRecording[MAXDEVICES-1];
  int iCardIsSwitchedTo[MAXDEVICES-1];
  int iPrimaryDevice;
};

cInfoStatus::cInfoStatus(void) {
  for(int i=0; i<MAXDEVICES; i++) {
     iCardIsRecording[i] = 0;
     iCardIsSwitchedTo[i] = 0;
  } 
  iPrimaryDevice = -1;  
}

int cInfoStatus::GetDeviceUsedStatus(int iCardIndex) {
  if ( iCardIndex < MAXDEVICES ) {
     return iCardIsSwitchedTo[iCardIndex];
    } else {
     return -1;
  }     
}

int cInfoStatus::GetDeviceRecStatus(int iCardIndex) {
  if ( iCardIndex < MAXDEVICES ) {
     return iCardIsRecording[iCardIndex];
    } else {
     return -1;
  }     
}

int cInfoStatus::GetPrimaryDevice(void){
   int i;
   iNumDevices = cDevice::NumDevices();

   for(i=0;i<iNumDevices;i++) {
     if (cDevice::GetDevice(i) == cDevice::PrimaryDevice() ) iPrimaryDevice = i ;
  }
  return iPrimaryDevice;
}

int cInfoStatus::IsPrimaryDevice(int nDevice){
   int i;
   i = 0;
   iNumDevices = cDevice::NumDevices();

   if ( nDevice<iNumDevices) {
     if (cDevice::GetDevice(nDevice) == cDevice::PrimaryDevice()) i = 1;
  }
  return i;
}

int cInfoStatus::Transferring(int iCardIndex) {
   int i;
//   cDevice *cActDevice;
   i = 0;
   iNumDevices = cDevice::NumDevices();

   if ( iCardIndex<iNumDevices) {
    i = (cDevice::GetDevice(iCardIndex))->Transferring();
 //    cActDevice = (cDevice::GetDevice(iCardIndex));
 //    i = cActDevice->Transferring();
  }
  return i;
}

int cInfoStatus::MaySwitchTransponder(int iCardIndex) {
   int i;
   i = 0;
   iNumDevices = cDevice::NumDevices();

   if ( iCardIndex<iNumDevices) {
     i = (cDevice::GetDevice(iCardIndex))->MaySwitchTransponder();
  }
  return i;
}

int cInfoStatus::GetRecStatus(void) {
  int iOccupiedDevices = 0;
  int i;

  iNumDevices = cDevice::NumDevices();

  for(i=0;i<iNumDevices;i++) {
    if (iCardIsRecording[i]>0) iOccupiedDevices++;
 }

  if(iNumDevices == iOccupiedDevices) {                  // alle Karten nehmen auf
    return 2; 
  } else if(iOccupiedDevices > 0) {                      // Aufnahme läuft
    return 1;
  } else {                                               // keine Aufnahme läuft
    return 0;        
  }
}

void cInfoStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber) {
  int iCardIndex = Device->CardIndex();
  if(iCardIndex < MAXDEVICES) iCardIsSwitchedTo[iCardIndex] = ChannelNumber;
}

void cInfoStatus::Recording(const cDevice *Device, const char *Name, const char *FileName, bool On) {
  int iCardIndex = Device->CardIndex();
  iNumDevices = Device->NumDevices();
  
  if(iCardIndex < MAXDEVICES) {
    if(Name && Name[0]) iCardIsRecording[iCardIndex]++;
    else                iCardIsRecording[iCardIndex]--;
  }
}

class cPluginVdrInfo : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cInfoStatus *cInfo; 
public:
  cPluginVdrInfo(void);
  virtual ~cPluginVdrInfo();
  virtual bool Start(void);
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  int iCardIsRecording[MAXDEVICES-1];
  };

cPluginVdrInfo::cPluginVdrInfo(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  cInfo = new cInfoStatus();
}

cPluginVdrInfo::~cPluginVdrInfo()
{
  // Clean up after yourself!
}
    
const char **cPluginVdrInfo::SVDRPHelpPages(void)
{
  static const char *HelpPages[] = {
    "DEVICES\n"
    "    Counts the DVB-Devices.",
    "PRIMDEV\n"
    "    Returns the number of the Primary Device.",
    "RECSTATUS\n"
    "    Shows the status of current recordings ( not the timers ) .\n"
    "    0->no recordings; 1->recording; 2->recording on all devices .",
    "TEST\n"
    "    Testzone xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx .\n"
    "    ========================================= .",
   "DEVSTATUS <device>\n"
    "    Shows the status of current recordings  .\n"
    "    and current channel of the selected device."
    "    default: device0  ",
    NULL
    };
  return HelpPages;
}

cString cPluginVdrInfo::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  int iCardIndex = 0;
  
  if (strcasecmp(Command, "DEVICES") == 0) {
      ReplyCode = 201;
      return cString::sprintf("Devices found=%d", cDevice::NumDevices());
     }
  else if (strcasecmp(Command, "RECSTATUS") == 0) {
      ReplyCode = 202;
      return cString::sprintf("Recording status=%d", cInfo->GetRecStatus());
     }
  else if (strcasecmp(Command, "DEVSTATUS") == 0) {
      if ( Option ) iCardIndex = atoi(Option);
      ReplyCode = 203;
      return cString::sprintf("Device %d: Rec=%d Used=%d", iCardIndex, cInfo->GetDeviceRecStatus(iCardIndex), cInfo->GetDeviceUsedStatus(iCardIndex) );
     }
  else if (strcasecmp(Command, "PRIMDEV") == 0) {
      ReplyCode = 204;
      return cString::sprintf("Primary device=%d", cInfo->GetPrimaryDevice()+1);
     }
  else if (strcasecmp(Command, "TEST") == 0) {
      if ( Option ) iCardIndex = atoi(Option);
      ReplyCode = 205;
      return cString::sprintf("Transferring=%d - MaySwitchTransponder=%d", cInfo->Transferring(iCardIndex), cInfo->MaySwitchTransponder(iCardIndex) );
     }
  return NULL;
}

bool cPluginVdrInfo::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

VDRPLUGINCREATOR(cPluginVdrInfo); // Don't touch this!
