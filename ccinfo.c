#include "./ccinfo.h"

#ifdef WIN32
static char *pcsc_stringify_error(LONG rv)
{
 static char out[255];
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
//oneByte 1
//TAG: File Control Information (FCI) Template 
//######
//TEST 2a356ea8: 84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 21 BF 0C 1E 61 1C 4F 07 A0 00 00 00 03 10 10 50 0E 63 6F 6D 64 69 72 65 63 74 20 56 69 73 61 87 01 01 
//oneByte 1
//TAG: Dedicated File (DF) Name 
//######
//TEST2 2a356e48: 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 

struct byteStream test;
BYTE testInput[] = { 0x6F, 0x33 , 0x84 , 0x0E , 0x32 , 0x50 , 0x41 , 0x59 , 0x2E , 0x53 , 0x59 , 0x53 , 0x2E , 0x44 , 0x44 , 0x46 , 0x30 , 0x31 , 0xA5 , 0x21 , 0xBF , 0x0C , 0x1E , 0x61 , 0x1C , 0x4F , 0x07 , 0xA0 , 0x00 , 0x00 , 0x00 , 0x03 , 0x10 , 0x10 , 0x50 , 0x0E , 0x63 , 0x6F , 0x6D , 0x64 , 0x69 , 0x72 , 0x65 , 0x63 , 0x74 , 0x20 , 0x56 , 0x69 , 0x73 , 0x61 , 0x87 , 0x01 , 0x01 , 0x90 , 0x00};

struct byteStream testInputStream;
testInputStream.length=51;
testInputStream.value = testInput;

//
struct byteStream out[64];
int anzAll = 0;
printf("\nSTART FIND ALL TAGS\n");
findAllTags(testInputStream,&out,&anzAll);
for (int i = 0; i < anzAll; i++)
  {
  printf("\n\n");
  printf("TAG: %s\n",out[i].tag.name);
  printf("length: %i\n",out[i].length);
  printf("HEX  : ");
  for (int k = 0; k < out[i].length; k++)
    {
    printf("%02X, ",out[i].value[k]);
    }
  printf("\n");
  printf("TEXT : ");
  for (int k = 0; k < out[i].length; k++)
    {
    if(out[i].value[k] >= 32 && out[i].value[k] <= 126 )
      {
      printf(" %c, ",out[i].value[k]);
      }
    else
      {
      printf("  , ");
      }
    }
  printf("\n");
  }


//
/*
printf("oneByte %i",isOneByteTlv(testInputStream));
printf("\nTAG: %s \n",getEmvTag(testInputStream).name);

getByteStream(&test,testInputStream,getEmvTag(testInputStream));
printf("######\n");
 printf("TEST %0x: ",&test.length);
 for(i=0; i < test.length; i++)
   printf("%02X ",test.value[i]);
 printf("\n");

printf("oneByte %i",isOneByteTlv(test));
printf("\nTAG: %s \n",getEmvTag(test).name);

struct byteStream test2;
getByteStream(&test2,test,getEmvTag(test));
printf("######\n");
 printf("TEST2 %0x: ",&test2.length);
 for(i=0; i < test2.length; i++)
   printf("%02X ",test2.value[i]);
 printf("\n");
*/
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

void findAllTags(struct byteStream ccStream, struct byteStream *outStream, int *anzOutStream)
  {
  if (ccStream.length > 2 && strcmp(getEmvTag(ccStream).name,unknownEmvTag.name) != 0)
    {
    struct byteStream partStream;
    //printf("\n################################################################\n");
    //printf("EMV TAG IN: %s \n",getEmvTag(ccStream).name);
    //printf("ccStream.length: %i\n",ccStream.length);
    //printf("isOneByte %i\n",isOneByteTlv(ccStream));
    //printf("ccStream.length: %i\n",ccStream.length);
    //printf("anzOutStream: %i\n",*anzOutStream); // DEBUG
    getByteStream(&partStream,ccStream,getEmvTag(ccStream));
    //printf("EMV TAG OUT: %s \n",getEmvTag(partStream).name);
    //printf("partStream.length: %i\n",partStream.length);
    //printf("outStream[%i].length: %i\n",*anzOutStream, outStream[*anzOutStream].length);
    //printf("outStream[%i].value = (BYTE*)malloc(%i)\n",*anzOutStream,partStream.length); //DEBUG
    outStream[*anzOutStream].value = (BYTE*)malloc(partStream.length);
    outStream[*anzOutStream].length = partStream.length;
    outStream[*anzOutStream].tag = getEmvTag(ccStream);
    //printf("start cpy part to out stream\n");  //DEBUG
    for (int i=0 ; i < partStream.length ; i++)
      {
      //printf("outStream[%i]->value[%i] = partStream.value[%i]: %02X \n",*anzOutStream,i,i,partStream.value[i]);      
      outStream[*anzOutStream].value[i] = partStream.value[i];
      }
    //printf("END for loop\n");
    ++*(anzOutStream);
    //printf("call findAllTags with result\n");
    findAllTags(partStream,outStream,anzOutStream);

    int restPos = 0;
    if (isOneByteTlv(ccStream))
      {
      restPos = partStream.length + 2;
      }
    else
      {
      restPos = partStream.length + 3;
      }
    //printf("restPos: %i\n",restPos);
    //printf("(ccStream.length - restPos): %i\n",(int)(ccStream.length - restPos));
    if ( (int)(ccStream.length - restPos)  > 0 )
      {
      struct byteStream rest;
      rest.length = ccStream.length - restPos;
      //printf("rest.length: %i\n",rest.length);
      rest.value = (BYTE*)malloc(rest.length);
      for (int i = 0 ; i < rest.length ; i++)
        {
        rest.value[i] = ccStream.value[i + restPos];
        }
      //printf("call findAllTags with rest\n");
      findAllTags(rest,outStream,anzOutStream);
      }
    }  
  }

bool isOneByteTlv (struct byteStream tlvStream)
  {
  int firstByte = (int) tlvStream.value[0]; 
  if (firstByte >= 128 )
    firstByte = firstByte - 128;
  if (firstByte >= 64)
    firstByte=firstByte - 64;
  if (firstByte >= 32)
    firstByte=firstByte - 32;
  
  if (firstByte >= 31)
    return(false);

  return(true);
  }

struct emvTag getEmvTag(struct byteStream ccStream)
  {
  BYTE tag[2];
  for (int i=0 ; i < emvTagCount ; i++)
    {
    tag[0]=ccStream.value[0];
    tag[1]=ccStream.value[1];
    if (isOneByteTlv(ccStream))
      {
      tag[1]=0x00;
      }
    if (tag[0] == emvTags[i].tag0 && tag[1] == emvTags[i].tag1)
      {
      return emvTags[i];
      }
    }
  return unknownEmvTag;
  }

int getByteStream(struct byteStream *ccStream, struct byteStream input, struct emvTag id)
  {
  int lengthPosition = 2;
    if (isOneByteTlv(input))
      {
      lengthPosition = 1;
      if (input.value[0] != id.tag0)
        {
        return(254);
        }
      }
    else // is two byte tlv
      {
      if (input.value[0] != id.tag0 || input.value[1] != id.tag1)
        return(254);
      }
  ccStream->length = (int)input.value[lengthPosition]; // -1 ???
  if (ccStream->length > 0 )
    {
    ccStream->value = (BYTE*)malloc(ccStream->length);
    for (int i=0 ; i < ccStream->length  ; i++)
      {
      ccStream->value[i] = input.value[lengthPosition + 1 + i]; 
      //printf("     ccStream->value[%i] = input.value[%i]: %02X \n",i,i,ccStream->value[i]);      
      }
    return(0);
    }
  }
