/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of MQTT Client Library

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "MQTTPacket.h"
#include "MqttTransport.h"
#include "LibMqtt.h"
#include "Trace.h"





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Configuration
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Macro definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  const char*     pszHostUrl_l                = NULL;     // Host URL
static  unsigned int    uiHostPortNum_l             = 0;        // Host PortNumber
static  const char*     pszClientName_l             = NULL;     // Client Name
static  unsigned int    uiKeepAliveInterval_l       = 0;        // KeepAlive Interval in [sec]
static  const char*     pszUserName_l               = NULL;     // User Authentication - UserName
static  const char*     pszPassword_l               = NULL;     // User Authentication - Password

static  int             iSocket_l                   = -1;
static  int             iPacketId_l                 =  0;
static  time_t          tmKeepAliveInterval_l       =  0;       // KeepAlive Interval in [sec]
static  time_t          tmLastKeepAliveTimestamp_l  =  0;       // Timestamp of last KeepAlive Message



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  int  ConnectMqttBroker (
    const char* pszHostUrl_p,                           // [IN]     Host URL
    unsigned int uiHostPortNum_p,                       // [IN]     Host PortNumber
    const char* pszClientName_p,                        // [IN]     Client Name
    unsigned int uiKeepAliveInterval_p,                 // [IN]     KeepAlive Interval in [sec]
    const char* pszUserName_p,                          // [IN]     User Authentication - UserName
    const char* pszPassword_p);                         // [IN]     User Authentication - Password


static  int  GetPacketId ();


static  int  SetMQTTString (
    MQTTString* pDstMQTTString_p,
    const char* pszSrcString_p);


#ifndef NDEBUG
    static  void  DbgDumpBuffer (
        const unsigned char* pabBuffer_p,
        int iBuffSize_p);
#endif





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  MqttConnect
//---------------------------------------------------------------------------

int  MqttConnect (
    const char* pszHostUrl_p,                           // [IN]     Host URL
    unsigned int uiHostPortNum_p,                       // [IN]     Host PortNumber
    const char* pszClientName_p,                        // [IN]     Client Name
    unsigned int uiKeepAliveInterval_p,                 // [IN]     KeepAlive Interval in [sec]
    const char* pszUserName_p,                          // [IN]     User Authentication - UserName
    const char* pszPassword_p)                          // [IN]     User Authentication - Password
{

int            iRes;


    TRACE0("MqttConnect:\n");
    TRACE2("    pszHostUrl_p='%s', uiHostPortNum_p=%u\n", pszHostUrl_p, uiHostPortNum_p);
    TRACE1("    pszClientName_p='%s'\n", pszClientName_p);
    TRACE1("    uiKeepAliveInterval_p=%u [sec]\n", uiKeepAliveInterval_p);
    TRACE2("    pszUserName_p='%s', pszPassword_p='%s'\n", pszUserName_p, pszPassword_p);


    // check parameter
    if ( (pszHostUrl_p          == NULL) ||
         (pszClientName_p       == NULL) ||
         (uiKeepAliveInterval_p == 0)    ||
         (pszUserName_p         == NULL) ||
         (pszPassword_p         == NULL)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }


    // saving all parameters for a possibly necessary later reconnect
    pszHostUrl_l          = pszHostUrl_p;
    uiHostPortNum_l       = uiHostPortNum_p;
    pszClientName_l       = pszClientName_p;
    uiKeepAliveInterval_l = uiKeepAliveInterval_p;
    pszUserName_l         = pszUserName_p;
    pszPassword_l         = pszPassword_p;


    // connect to MQTT Broker
    iRes = ConnectMqttBroker(pszHostUrl_l,                      // Host URL
                             uiHostPortNum_l,                   // Host PortNumber
                             pszClientName_l,                   // Client Name
                             uiKeepAliveInterval_l,             // KeepAlive Interval in [sec]
                             pszUserName_l,                     // User Authentication - UserName
                             pszPassword_l);                    // User Authentication - Password

    return (iRes);

}



//---------------------------------------------------------------------------
//  MqttReconnect
//---------------------------------------------------------------------------

int  MqttReconnect ()
{

int  iRes;


    // check saved parameter
    if ( (pszHostUrl_l          == NULL) ||
         (pszClientName_l       == NULL) ||
         (uiKeepAliveInterval_l == 0)    ||
         (pszUserName_l         == NULL) ||
         (pszPassword_l         == NULL)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }


    // connect to MQTT Broker
    iRes = ConnectMqttBroker(pszHostUrl_l,                      // Host URL
                             uiHostPortNum_l,                   // Host PortNumber
                             pszClientName_l,                   // Client Name
                             uiKeepAliveInterval_l,             // KeepAlive Interval in [sec]
                             pszUserName_l,                     // User Authentication - UserName
                             pszPassword_l);                    // User Authentication - Password

    return (iRes);

}



//---------------------------------------------------------------------------
//  MqttDisconnect
//---------------------------------------------------------------------------

int  MqttDisconnect ()
{

unsigned char  abMqttRawDataPacketBuff[1024];        // buffer for raw data packet
const int      iBuffSize = sizeof(abMqttRawDataPacketBuff);
int            iUsedBuffLen;
int            iRes;


    TRACE0("MqttDisconnect:\n");

    // disconnect from host
    TRACE0("Send disconnection request to host... ");
    iUsedBuffLen = MQTTSerialize_disconnect(abMqttRawDataPacketBuff, iBuffSize);
    iRes = MqttTransport_SendPacketBuffer(iSocket_l, abMqttRawDataPacketBuff, iUsedBuffLen);
    if (iRes == iUsedBuffLen)
    {
        TRACE0("successful\n");
    }
    else
    {
        TRACE0("FAILED!\n");
        return (-1);
    }


    // close connection to host (broker)
    TRACE0("Close connection to host...\n");
    MqttTransport_Close(iSocket_l);


    return (0);

}



//---------------------------------------------------------------------------
//  MqttPublishMessage
//---------------------------------------------------------------------------

int  MqttPublishMessage (
    const char* pszTopic_p,                             // [IN]     Topic string
    const unsigned char* pabPayloadBuff_p,              // [IN]     Address of Payload buffer
    unsigned int uiPayloadLen_p,                        // [IN]     Size of Payload buffer
    tMqttQoSLevel PublishQos_p,                         // [IN]     MQTT QoS value
    unsigned int fRetainedFlag_p)                       // [IN]     MQTT retained flag
{

MQTTString     MqttTopicString = MQTTString_initializer;
MQTTTransport  MqttNonBlockTransportSession;

unsigned char  abMqttRawDataPacketBuff[1024];        // buffer for raw data packet
const int      iBuffSize = sizeof(abMqttRawDataPacketBuff);
int            iUsedBuffLen;
int            iPacketId;
unsigned char  bDupFlag;
int            iPublishQos;
unsigned char  bRetainedFlag;
int            iRes;


    TRACE0("MqttPublishMessage:\n");
    TRACE2("    pszTopic_p=0x%08lX -> '%s'\n", (unsigned long)pszTopic_p, ((pszTopic_p != NULL) ? pszTopic_p : "(NULL)"));
    TRACE2("    pabPayloadBuff_p=0x%08lX, uiPayloadLen_p=%u\n", (unsigned long)pabPayloadBuff_p, uiPayloadLen_p);
    TRACE1("    PublishQos_p=%u\n", PublishQos_p);
    TRACE1("    fRetainedFlag_p=%u\n", fRetainedFlag_p);


    // check parameter
    if ( (pszTopic_p       == NULL) ||
         (pabPayloadBuff_p == NULL) ||
         (uiPayloadLen_p   == 0)    ||
         (PublishQos_p > kMqttQoS2)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    // check requested QoS level
    if (PublishQos_p > kMqttQoS0)
    {
        TRACE1("ERROR: Requested QoS Level (%d) not supported!\n", PublishQos_p);
        return (-2);
    }

    tmLastKeepAliveTimestamp_l = time(NULL);                // Timestamp of last KeepAlive Message (-> Timestamp of Data Message)


    // generate 'Publishing Information' (MQTT Flags, Topic and Payload) data packet
    SetMQTTString(&MqttTopicString, pszTopic_p);
    iPacketId     = GetPacketId();
    iPublishQos   = (int)PublishQos_p;
    bRetainedFlag = (unsigned char)fRetainedFlag_p;
    bDupFlag      = 0;
    memset(abMqttRawDataPacketBuff, 0, iBuffSize);
    iUsedBuffLen = MQTTSerialize_publish(abMqttRawDataPacketBuff,
                                         iBuffSize,
                                         bDupFlag,              // MQTT dup flag
                                         iPublishQos,           // MQTT QoS value
                                         bRetainedFlag,         // MQTT retained flag
                                         iPacketId,             // MQTT packet identifier
                                         MqttTopicString,
                                         (unsigned char*)pabPayloadBuff_p,
                                         uiPayloadLen_p);
    #ifndef NDEBUG
    {
        DbgDumpBuffer(abMqttRawDataPacketBuff, iUsedBuffLen);
    }
    #endif


    // write the encoded data packet to the socket connected with host
    TRACE0("Send data packet to host... ");
    iRes = MqttTransport_SendPacketBuffer(iSocket_l, abMqttRawDataPacketBuff, iUsedBuffLen);
    if (iRes == iUsedBuffLen)
    {
        TRACE0("successful\n");
    }
    else
    {
        TRACE0("FAILED!\n");
        return (-3);
    }


    // configure transport session for non-blocking subscribed data receiption
    MqttNonBlockTransportSession.sck   = &iSocket_l;
    MqttNonBlockTransportSession.getfn = MqttTransport_GetDataNonBlock;
    MqttNonBlockTransportSession.state = 0;


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //
    // Hier in Abhängigkeit von QoS auf Antwort vom Server warten:
    //      QoS 0       None
    //      QoS 1       PUBACK Packet
    //      QoS 2       PUBREC Packet
    //
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    return (0);

}



//---------------------------------------------------------------------------
//  MqttKeepAlive
//---------------------------------------------------------------------------

int  MqttKeepAlive (
    const char* pszTopic_p)                             // [IN]     Topic string
{

time_t  tmCurrTimestamp;
char    szKeepAliveMsg[64];
int     iUsedBuffLen;
int     iRes;


    iRes = 0;

    tmCurrTimestamp = time(NULL);
    if ((tmCurrTimestamp - tmLastKeepAliveTimestamp_l) >= tmKeepAliveInterval_l)
    {
        TRACE0("\nMqttKeepAlive:\n");
        TRACE2("    pszTopic_p=0x%08lX -> '%s'\n", (unsigned long)pszTopic_p, ((pszTopic_p != NULL) ? pszTopic_p : "(NULL)"));

        tmLastKeepAliveTimestamp_l = tmCurrTimestamp;

        snprintf(szKeepAliveMsg, sizeof(szKeepAliveMsg), "%lu", (unsigned long)tmCurrTimestamp);
        iUsedBuffLen = strlen(szKeepAliveMsg);

        iRes = MqttPublishMessage(pszTopic_p, (const unsigned char*)szKeepAliveMsg, iUsedBuffLen, kMqttQoS0, 0);
        if (iRes == 0)
        {
            TRACE0("successful\n\n\n");
            iRes = 1;
        }
        else
        {
            TRACE0("FAILED!\n\n\n");
            iRes = -1;
        }
    }


    return (iRes);

}



//---------------------------------------------------------------------------
//  MqttPrintMessage
//---------------------------------------------------------------------------

void  MqttPrintMessage (
    const char* pszTopic_p,                             // [IN]     Topic string
    const unsigned char* pabPayloadBuff_p,              // [IN]     Address of Payload buffer
    unsigned int uiPayloadLen_p)                        // [IN]     Size of Payload buffer
{

char          ui8DataByte;
unsigned int  uiIdx;


    printf("MQTT Message:\n");

    if (pszTopic_p != NULL)
    {
        printf("  Topic:         '%s'\n", pszTopic_p);
    }

    printf("  Payload Len:   %u\n", uiPayloadLen_p);
    printf("  Payload Data:  ");
    if (pabPayloadBuff_p != NULL)
    {
        printf("'");
        for (uiIdx=0; uiIdx<uiPayloadLen_p; uiIdx++)
        {
            ui8DataByte = pabPayloadBuff_p[uiIdx];
            if ((ui8DataByte <= 0x1F) || (ui8DataByte >= 0x7F))
            {
                ui8DataByte = '.';
            }
            printf("%c", ui8DataByte);
        }
        printf("'");
    }
    else
    {
        printf("{null}");
    }
    printf("\n\n");


    return;

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  ConnectMqttBroker
//---------------------------------------------------------------------------

static  int  ConnectMqttBroker (
    const char* pszHostUrl_p,                           // [IN]     Host URL
    unsigned int uiHostPortNum_p,                       // [IN]     Host PortNumber
    const char* pszClientName_p,                        // [IN]     Client Name
    unsigned int uiKeepAliveInterval_p,                 // [IN]     KeepAlive Interval in [sec]
    const char* pszUserName_p,                          // [IN]     User Authentication - UserName
    const char* pszPassword_p)                          // [IN]     User Authentication - Password
{

MQTTPacket_connectData  MqttConnectionData = MQTTPacket_connectData_initializer;
MQTTString              MqttLwtTopicString = MQTTString_initializer;
MQTTString              MqttLwtPayloadData = MQTTString_initializer;

unsigned char  abMqttRawDataPacketBuff[1024];           // buffer for raw data packet
const int      iBuffSize = sizeof(abMqttRawDataPacketBuff);
int            iUsedBuffLen;
int            iSocket;
unsigned int   fTlsMode;
unsigned char  bSessionPresentFlag;
unsigned char  bConnAckRes;
int            iOldSocket;
int            iRes;


    // initialize workspace
    iSocket_l   = -1;
    iPacketId_l =  0;
    tmKeepAliveInterval_l = (time_t)uiKeepAliveInterval_p;
    tmLastKeepAliveTimestamp_l = time(NULL);                // Timestamp of last KeepAlive Message (-> Timestamp of Connect Message)


    // open connection to host (broker)
    TRACE2("Open connection to host '%s', port %u... ", pszHostUrl_p, uiHostPortNum_p);
    iSocket = MqttTransport_Open((char*)pszHostUrl_p, (int)uiHostPortNum_p);
    if (iSocket >= 0)
    {
        TRACE1("ok (iSocket=%d)\n", iSocket);
        iSocket_l = iSocket;
    }
    else
    {
        TRACE1("FAILED! (iSocket=%d)\n", iSocket);
        return (-2);
    }


    // setup connection information
    MqttConnectionData.MQTTVersion       = 4;
    MqttConnectionData.clientID.cstring  = (char*)pszClientName_p;
    MqttConnectionData.keepAliveInterval = uiKeepAliveInterval_p;
    MqttConnectionData.cleansession      = 1;
    MqttConnectionData.username.cstring  = (char*)pszUserName_p;
    MqttConnectionData.password.cstring  = (char*)pszPassword_p;
    TRACE3("User identification: ClientID='%s', User='%s', Passwd='%s'\n",
           MqttConnectionData.clientID.cstring,
           MqttConnectionData.username.cstring,
           MqttConnectionData.password.cstring);


    // put serialized connection information of data buffer
    iUsedBuffLen = MQTTSerialize_connect(abMqttRawDataPacketBuff,
                                         iBuffSize,
                                         &MqttConnectionData);
    #ifndef NDEBUG
    {
        DbgDumpBuffer(abMqttRawDataPacketBuff, iUsedBuffLen);
    }
    #endif


    // send connection request to host
    TRACE0("Send connection request to host... ");
    iRes = MqttTransport_SendPacketBuffer(iSocket, abMqttRawDataPacketBuff, iUsedBuffLen);
    if (iRes == iUsedBuffLen)
    {
        TRACE0("successful\n");
    }
    else
    {
        TRACE0("FAILED!\n");
        return (-3);
    }


    // wait for connection acknowledge from host
    TRACE0("Wait for connection acknowledge from host... ");
    iOldSocket = MqttTransport_SetGetDataSocket(iSocket);          // $$$$$$ später einfach rausschmeißen
    iRes = MQTTPacket_read(abMqttRawDataPacketBuff, iBuffSize, MqttTransport_GetData);
    MqttTransport_SetGetDataSocket(iOldSocket);          // $$$$$$ später einfach rausschmeißen
    if (iRes != CONNACK)
    {
        TRACE0("FAILED!\n");
        return (-5);
    }
    iRes = MQTTDeserialize_connack(&bSessionPresentFlag,
                                   &bConnAckRes,
                                   abMqttRawDataPacketBuff,
                                   iBuffSize);
    if ((iRes == 1) && (bConnAckRes == 0))
    {
        TRACE0("successful (CONACK)\n");
    }
    else
    {
        TRACE2("REFUSED! (iRes=%d, ConnAckRes=%d)\n", iRes, (int)bConnAckRes);
        return (-4);
    }


    return (0);

}



//---------------------------------------------------------------------------
//  Get next Packet ID
//---------------------------------------------------------------------------

static  int  GetPacketId ()
{

    if (iPacketId_l == 0)
    {
        iPacketId_l = 1;
    }
    else if (iPacketId_l >= 0xFFFE)
    {
        iPacketId_l = 1;
    }
    else
    {
        iPacketId_l++;
    }


    return (iPacketId_l);

}



//---------------------------------------------------------------------------
//  Set String to MQTT String
//---------------------------------------------------------------------------

static  int  SetMQTTString (
    MQTTString* pDstMQTTString_p,
    const char* pszSrcString_p)
{

    pDstMQTTString_p->cstring        = (char*)pszSrcString_p;
    pDstMQTTString_p->lenstring.data = (char*)pszSrcString_p;
    pDstMQTTString_p->lenstring.len  = strlen(pszSrcString_p);


    return (pDstMQTTString_p->lenstring.len);

}



//---------------------------------------------------------------------------
//  DEBUG: Dump buffer contents
//---------------------------------------------------------------------------

#ifndef NDEBUG

static  void  DbgDumpBuffer (
    const unsigned char* pabBuffer_p,
    int iBuffSize_p)
{

const unsigned char*  pabBuffer;
unsigned int          uiBuffSize;
unsigned char         bData;
unsigned int          nRow;
unsigned int          nCol;


    // get pointer to buffer and length of buffer
    pabBuffer  = pabBuffer_p;
    uiBuffSize = (unsigned int)iBuffSize_p;


    // dump buffer contents
    for (nRow=0; ; nRow++)
    {
        TRACE1("\n%04X:   ", nRow*0x10);

        for (nCol=0; nCol<16; nCol++)
        {
            if (nCol < uiBuffSize)
            {
                TRACE1("%02X ", (unsigned int)*(pabBuffer+nCol));
            }
            else
            {
                TRACE0("   ");
            }
        }

        TRACE0(" ");

        for (nCol=0; nCol<16; nCol++)
        {
            bData = *pabBuffer++;
            if (nCol < uiBuffSize)
            {
                if ((bData >= 0x20) && (bData < 0x7F))
                {
                    TRACE1("%c", bData);
                }
                else
                {
                    TRACE0(".");
                }
            }
            else
            {
                TRACE0(" ");
            }
        }

        if (uiBuffSize > 16)
        {
            uiBuffSize -= 16;
        }
        else
        {
            break;
        }
    }

    TRACE0("\n\n");

}

#endif  // #ifndef NDEBUG




// EOF


