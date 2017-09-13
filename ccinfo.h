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
static BYTE bytesAvailable = 0x61;
static BYTE wrongLength = 0x6C;

static BYTE issuerDiscretinaryData[] = { 0xBF , 0x0C }; 

static BYTE getResponse[]  = {0x00, 0xc0, 0x00, 0x00, 0x00};
static int getResponseLength = 5;

SCARDHANDLE hCard;
SCARD_IO_REQUEST pioSendPci;
DWORD dwReaders,dwActiveProtocol, dwRecvLength;
BYTE pbRecvBuffer[258];
BYTE cla = 0x00; //iso 7816
BYTE ins = 0xA4;
BYTE p1 = 0x04;
BYTE p2 = 0x00;
BYTE lcPPSE = 0x0E;
BYTE le = 0x00;

struct emvTag {
  char name[255];
  BYTE tag0;
  BYTE tag1;
};

static struct emvTag unknownEmvTag = {"unknwon", 0x00, 0x00};

static unsigned int emvTagCount = 7;
static struct emvTag emvTags[7] = {
    // one byte tags
    {"File Control Information (FCI) Template", 0x6f, 0x00},
    {"Dedicated File (DF) Name", 0x84, 0x00},
    {"File Control Information (FCI) Proprietary Template", 0xa5, 0x00},
    {"Application Template", 0x61, 0x00},
    {"Application Identifier (AID) – card", 0x4F, 0x00},
    {"Application Label", 0x50, 0x00},
    // two byte tags
    {"Issuer Discretinary Data", 0xBF, 0x0C}
  };

struct byteStream {
  BYTE *value;
  unsigned int length;
  struct emvTag tag;
};


void printAllTags(int anzPPSE, struct byteStream outPPSE[]);
int getByteStream(struct byteStream *ccStream, struct byteStream input, struct emvTag id);
void getMoreBytes();
bool isOneByteTlv (struct byteStream tlvStream);
struct emvTag getEmvTag(struct byteStream ccStream);
void findAllTags(struct byteStream ccStream, struct byteStream *outStream, int *anzOutStream);
