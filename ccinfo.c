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
 unsigned int i;

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

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
 BYTE selectPPSE[sizeof(aidPPSE) + 6] = { cla , ins, p1, p2, lcPPSE};
 memcpy(selectPPSE+5 , aidPPSE, sizeof(aidPPSE));


 dwRecvLength = sizeof(pbRecvBuffer);
 rv = SCardTransmit(hCard, &pioSendPci, selectPPSE, sizeof(selectPPSE),
  NULL, pbRecvBuffer, &dwRecvLength);
 CHECK("SCardTransmit", rv)

 //DEBUG 
 //printByteArray(selectPPSE,sizeof(selectPPSE),"cmd");
 //printByteArray(pbRecvBuffer,dwRecvLength, "response");

 if (pbRecvBuffer[dwRecvLength -2] == bytesAvailable)
   getMoreBytes();

 //DEBUG
 //printByteArray(pbRecvBuffer,dwRecvLength, "response");

  struct byteStream selectPpseResponse;
  selectPpseResponse.length=dwRecvLength;
  selectPpseResponse.value = (BYTE*)malloc(selectPpseResponse.length);
  memcpy(selectPpseResponse.value,pbRecvBuffer,dwRecvLength);

  struct byteStream outPPSE[64];
  int anzPPSE = 0;

  findAllTags(selectPpseResponse,&outPPSE,&anzPPSE);
  //DEBUG
  //printAllTags(anzPPSE, outPPSE);

  //print name of credit institute
  printApplicationLabel(anzPPSE, outPPSE);
 
  for (int i = 0; i < anzPPSE; i++)
    {
    if(strcmp(outPPSE[i].tag.name,"Application Identifier (AID) – card") == 0) 
      {
      BYTE aid[outPPSE[i].length];
      memcpy(aid,outPPSE[i].value,outPPSE[i].length);
      
      BYTE selectFileCC[outPPSE[i].length + 6]; 
      selectFileCC[0] = cla; 
      selectFileCC[1] = ins;
      selectFileCC[2] = p1;
      selectFileCC[3] = p2;
      selectFileCC[4] = (BYTE) outPPSE[i].length;
      memcpy(selectFileCC+5 , outPPSE[i].value, outPPSE[i].length);
      
     
      dwRecvLength=sizeof(pbRecvBuffer);
      rv = SCardTransmit(hCard, &pioSendPci, selectFileCC, sizeof(selectFileCC), NULL,
      pbRecvBuffer,&dwRecvLength);

      //DEBUG 
      //printByteArray(selectFileCC,sizeof(selectFileCC),"cmd");
      //printByteArray(pbRecvBuffer,dwRecvLength, "response");
      
      printf("\n");
      // Pruefen auf 90 00 !!!! TODO

      if (pbRecvBuffer[dwRecvLength -2] == bytesAvailable)
        getMoreBytes();

      BYTE readRecordCC[] = {0x00, 0xB2, 0x02, 0x0C, 0x00};
      dwRecvLength = sizeof(pbRecvBuffer);
      rv = SCardTransmit(hCard, &pioSendPci, readRecordCC, sizeof(readRecordCC), NULL,
      pbRecvBuffer,&dwRecvLength);

      //DEBUG 
      //printByteArray(readRecordCC,sizeof(readRecordCC),"cmd");
      //printByteArray(pbRecvBuffer,dwRecvLength, "response");

       if (pbRecvBuffer[dwRecvLength -2] == wrongLength){
         BYTE readRecordCC[] = {0x00, 0xB2, 0x02, 0x0C, pbRecvBuffer[dwRecvLength-1]};
         dwRecvLength = sizeof(pbRecvBuffer);
         rv = SCardTransmit(hCard, &pioSendPci, readRecordCC, sizeof(readRecordCC), NULL,
         pbRecvBuffer,&dwRecvLength);
     
         //DEBUG 
         //printByteArray(readRecordCC,sizeof(readRecordCC),"cmd");
         //printByteArray(pbRecvBuffer,dwRecvLength, "response");
       }

      if (pbRecvBuffer[dwRecvLength -2] == bytesAvailable)
        getMoreBytes();

     for (int k = 0; k < dwRecvLength; k++)
      {
      if(pbRecvBuffer[k] >= 32 && pbRecvBuffer[k] <= 126 )
        {
        printf(" %c, ",pbRecvBuffer[k]);
        }
      else
        {
        printf("  , ");
        }
      }
    printf("\n");

    }
  }

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

void printByteArray(BYTE cmd[], int size, char name[])
  {
  printf("%s: ",name);
  for(int i=0; i< size; i++)
  printf("%02X ", cmd[i]);
  printf("\n");
  }

void printApplicationLabel(int anzPPSE, struct byteStream outPPSE[])
  {
  for (int i = 0; i < anzPPSE; i++)
      {
      if (strcmp(outPPSE[i].tag.name,"Application Label") == 0)
        {
        printf("\n");
        printf("PPSE TAG: %s (name of credit institute)\n",outPPSE[i].tag.name);
        printf("PPSE TEXT : ");
        for (int k = 0; k < outPPSE[i].length; k++)
          {
          if(outPPSE[i].value[k] >= 32 && outPPSE[i].value[k] <= 126 )
            {
            printf(" %c",outPPSE[i].value[k]);
            }
          else
            {
            printf("  ");
            }
          }
        printf("\n");
        }
      }
  }

void printAllTags(int anzPPSE, struct byteStream outPPSE[])
  {
  for (int i = 0; i < anzPPSE; i++)
      {
      printf("\n\n");
      printf("PPSE TAG: %s\n",outPPSE[i].tag.name);
      printf("PPSE length: %i\n",outPPSE[i].length);
      printf("PPSE HEX  : ");
      for (int k = 0; k < outPPSE[i].length; k++)
        {
        printf("%02X, ",outPPSE[i].value[k]);
        }
      printf("\n");
      printf("PPSE TEXT : ");
      for (int k = 0; k < outPPSE[i].length; k++)
        {
        if(outPPSE[i].value[k] >= 32 && outPPSE[i].value[k] <= 126 )
          {
          printf(" %c, ",outPPSE[i].value[k]);
          }
        else
          {
          printf("  , ");
          }
        }
      printf("\n");
      }
  }

void getMoreBytes()
  {
   int i=0;
   BYTE myGetResponse[5];
   memcpy(myGetResponse,getResponse,getResponseLength);
   myGetResponse[4]=pbRecvBuffer[dwRecvLength-1];
   dwRecvLength=sizeof(pbRecvBuffer);
   SCardTransmit(hCard, &pioSendPci, myGetResponse, getResponseLength, NULL,
     pbRecvBuffer,&dwRecvLength);
   //DEBUG
   //printByteArray(myGetResponse,getResponseLength,"cmd");
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
