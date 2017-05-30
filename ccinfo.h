#ifdef WIN32
#undef UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#else
#include <winscard.h>
#endif

static BYTE fileControlInformationId = 0x6F;
static BYTE dedicatedFileName = 0x84;
static BYTE FciProp = 0xA5;
static BYTE unknown01 = 0x61; 
static BYTE unknown02 = 0x4F; 
static BYTE unknown03 = 0x50; 

static BYTE issuerDiscretinaryData[] = { 0xBF , 0x0C }; 

BYTE pbRecvBuffer[258];
BYTE cla = 0x00; //iso 7816
BYTE ins = 0xA4;
BYTE p1 = 0x04;
BYTE p2 = 0x00;
BYTE lc = 0x0E;
BYTE le = 0x00;

struct byteStream {
  BYTE *value;
  unsigned int length;
};

struct emvTag {
  char name[255];
  BYTE tag[1];
};

struct emvTag emvTags[6] = {
    {"File Control Information (FCI) Template", 0x6f},
    {"Dedicated File (DF) Name", 0x84},
    {"File Control Information (FCI) Proprietary Template", 0xa5},
    {"Application Template", 0x61},
    {"Application Identifier (AID) – card", 0x4F},
    {"Application Label", 0x50},
  };

int getByteStreamByOneByteId(struct byteStream *ccStream, BYTE input[], BYTE id);
bool isOneByteTlv (struct byteStream *tlvStream);
