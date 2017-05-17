#ifdef WIN32
#undef UNICODE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int getByteStreamByOneByteId(struct byteStream *ccStream, BYTE input[], BYTE id);
