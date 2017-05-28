#include "./ccinfo.h"

#ifdef WIN32
static char *pcsc_stringify_error(LONG rv)
{
 static char out[20];
 sprintf_s(out, sizeof(out), "0x%08X", rv);

 return out;
}
#endif

#define CHECK(f, rv) \
 if (SCARD_S_SUCCESS != rv) \
 { \
  printf(f ": %s\n", pcsc_stringify_error(rv)); \
  return -1; \
 }

int main(void)
{
 LONG rv;

 SCARDCONTEXT hContext;
 LPTSTR mszReaders;
 SCARDHANDLE hCard;
 DWORD dwReaders, dwActiveProtocol, dwRecvLength;

 SCARD_IO_REQUEST pioSendPci;

 unsigned int i;

//#################
struct byteStream test;
BYTE testInput[] = { 0x6F, 0x33 , 0x84 , 0x0E , 0x32 , 0x50 , 0x41 , 0x59 , 0x2E , 0x53 , 0x59 , 0x53 , 0x2E , 0x44 , 0x44 , 0x46 , 0x30 , 0x31 , 0xA5 , 0x21 , 0xBF , 0x0C , 0x1E , 0x61 , 0x1C , 0x4F , 0x07 , 0xA0 , 0x00 , 0x00 , 0x00 , 0x03 , 0x10 , 0x10 , 0x50 , 0x0E , 0x63 , 0x6F , 0x6D , 0x64 , 0x69 , 0x72 , 0x65 , 0x63 , 0x74 , 0x20 , 0x56 , 0x69 , 0x73 , 0x61 , 0x87 , 0x01 , 0x01 , 0x90 , 0x00};

getByteStreamByOneByteId(&test,testInput,fileControlInformationId);
printf("######\n");
 printf("TEST %0x: ",&test.length);
 for(i=0; i < test.length; i++)
   printf("%02X ",test.value[i]);
 printf("\n");

struct byteStream test2;
getByteStreamByOneByteId(&test2,test.value,dedicatedFileName);
printf("######\n");
 printf("TEST2 %0x: ",&test2.length);
 for(i=0; i < test2.length; i++)
   printf("%02X ",test2.value[i]);
 printf("\n");

//#################

 rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
 CHECK("SCardEstablishContext", rv)

#ifdef SCARD_AUTOALLOCATE
 dwReaders = SCARD_AUTOALLOCATE;

 rv = SCardListReaders(hContext, NULL, (LPTSTR)&mszReaders, &dwReaders);
 CHECK("SCardListReaders", rv)
#else
 rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
 CHECK("SCardListReaders", rv)

 mszReaders = calloc(dwReaders, sizeof(char));
 rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
 CHECK("SCardListReaders", rv)
#endif
 printf("reader name: %s\n", mszReaders);

 rv = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED,
  SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
 CHECK("SCardConnect", rv)

 switch(dwActiveProtocol)
 {
  case SCARD_PROTOCOL_T0:
   pioSendPci = *SCARD_PCI_T0;
   break;

  case SCARD_PROTOCOL_T1:
   pioSendPci = *SCARD_PCI_T1;
   break;
 }

 BYTE aidPPSE[] = {0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E , 0x44, 0x44, 0x46, 0x30, 0x31};
 BYTE selectPPSE[sizeof(aidPPSE) + 6] = { cla , ins, p1, p2, lc};
 memcpy(selectPPSE+5 , aidPPSE, sizeof(aidPPSE));


 dwRecvLength = sizeof(pbRecvBuffer);
 rv = SCardTransmit(hCard, &pioSendPci, selectPPSE, sizeof(selectPPSE),
  NULL, pbRecvBuffer, &dwRecvLength);
 CHECK("SCardTransmit", rv)

 printf("cmd: ");
 for(i=0; i<sizeof(selectPPSE); i++)
  printf("%02X ", selectPPSE[i]);

 printf("response: ");
 for(i=0; i<dwRecvLength; i++)
  printf("%02X ", pbRecvBuffer[i]);
 printf("\n");

 rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
 CHECK("SCardDisconnect", rv)

#ifdef SCARD_AUTOALLOCATE
 rv = SCardFreeMemory(hContext, mszReaders);
 CHECK("SCardFreeMemory", rv)

#else
 free(mszReaders);
#endif

 rv = SCardReleaseContext(hContext);

 CHECK("SCardReleaseContext", rv)

 return 0;
}

bool isOneByteTlv (struct byteStream *tlvStream) {

  BYTE firstByte = tlvStream->value[0]; 

  if (firstByte >= 128 )
     firstByte=firstByte-128;
  if (firstByte >= 64)
    firstByte=firstByte-64;
  if (firstByte=firstByte-64)
    firstByte=firstByte-32;
 
  if (firstByte > 31)
    return(false);

  return(true);
}

int getByteStreamByOneByteId(struct byteStream *ccStream, BYTE input[], BYTE id)
  {
  static int lengthPosition = 1;
  if (input[0] == id)
    {
    ccStream->length = (int)input[lengthPosition];
    if (ccStream->length > 0 )
      {
      ccStream->value = (BYTE*)malloc(ccStream->length);
      for (int i=0 ; i < ccStream->length ; i++)
        {
        ccStream->value[i] = input[lengthPosition + 1 + i];
        }
      return(0);
      }
    }
  return(255);
  }
