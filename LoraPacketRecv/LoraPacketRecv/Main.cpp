/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of Main Module

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <bcm2835.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <RH_RF95.h>
#include "LoraPacket.h"
#include "LoraPayloadDecoder.h"
#include "PacketProcessing.h"
#include "MessageQualification.h"
#include "MessageFileWriter.h"
#include "LibRf95.h"
#include "LibMqtt.h"
#include "GpioIrq.h"
#include "Trace.h"

// Define type of RF95 HAT
#define BOARD_DRAGINO_PIHAT                             // set type to Dragino Raspberry PI HAT (see https://github.com/dragino/Lora)
#include "RasPiBoards.h"                                // include RasPi_Boards.h so this will expose defined constants with CS/IRQ/RESET





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

#define APP_VER_MAIN            1                       // Version 1.xx
#define APP_VER_REL             0                       // Version x.00

#define GPIO_PIN_CS             RF_CS_PIN               // RPI_V2_GPIO_P1_22 -> Slave Select on P1/Pin#22 alias GPIO25
#define GPIO_PIN_IRQ            RF_IRQ_PIN              // RPI_V2_GPIO_P1_07 -> IRQ on P1/Pin#7 alias GPIO4
#define GPIO_PIN_RST            RF_RST_PIN              // RPI_V2_GPIO_P1_11 -> Reset on P1/Pin#11 alias GPIO17

#define RF_TX_POWER             14                      // Transmitter Power Output Level
#define RF_FREQUENCY            868.00                  // Transmitter and Receiver Centre Frequency


static  const  char*            MQTT_DEF_HOST_URL       = "127.0.0.1";
static  const  int              MQTT_DEF_HOST_PORTNUM   = 1883;

static  const  char*            MQTT_CLIENT_NAME        = "LoraPacketRecv";
static  const  unsigned int     MQTT_KEEPALIVE_INTERVAL = 30;
static  const  char*            MQTT_USER_NAME          = "{empty}";
static  const  char*            MQTT_PASSWORD           = "{empty}";

static  const  char*            MQTT_TOPIC_TMPL_BOOTUP  = "LoraAmbMon/Data/DevID%03u/Bootup";
static  const  char*            MQTT_TOPIC_TMPL_ST_DATA = "LoraAmbMon/Data/DevID%03u/StData";
static  const  char*            MQTT_TOPIC_TELEMETRY    = "LoraAmbMon/Status/Telemetry";
static  const  char*            MQTT_TOPIC_KEEPALVIE    = "LoraAmbMon/Status/KeepAlive";



//---------------------------------------------------------------------------
//  Constant definitions
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

static  char                    szServerUrl_l[128];
static  const char*             pszHostAddr_l;          // = MQTT_DEF_HOST_URL
static  int                     iPortNum_l;             // = MQTT_DEF_HOST_PORTNUM
static  const char*             pszMsgFileName_l        = NULL;
static  int                     fProcAllMsg_l           = false;
static  int                     fTelemetryMsg_l         = false;
static  int                     fOffline_l              = false;
static  bool                    fVerbose_l              = false;

static  bool                    fRunMainLoop_l          = false;



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  void  AppSigHandler (int iSignalNum_p);

static  bool  AppEvalCmdlnArgs (int iArgCnt_p, char* apszArg_p[]);
static  void  AppPrintHelpScreen  (const char* pszArg0_p);

static  int  BuildMqttPublishTopic (
    const tJsonMessage* pJsonMessage_p,
    char* pszTopicBuffer_p,
    int iTopicBuffSize_p);

static  const char*  GetLoraPacketTypeName (
    tLoraPacketType PacketType_p);

static  int  GetCurrentDateTime (
    char* pszBuffer_p,
    int iBuffSize_p);

static  int  FormatTimeStamp (
    time_t tmTimeStamp_p,
    char* pszBuffer_p,
    int iBuffSize_p);

static  bool  SplitupUrlString (
    const char* pszUrlString_p,
    const char** ppszIpAddr_p,
    int* piPortNum_p);

static  void  PrintDataBuffer (
    const void* pabDataBuff_p,
    unsigned int uiDataBuffLen_p);

static  void  DumpDataBuffer (
    const void* pabDataBuff_p,
    unsigned int uiDataBuffLen_p);





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Main function of this application
//---------------------------------------------------------------------------

int  main (int iArgCnt_p, char* apszArg_p[])
{

std::vector<tJsonMessage>  vecJsonMessages;
struct pollfd  FdSet[1];
volatile int   iGpioState;
uint8_t        abRxDataBuff[RH_RF95_MAX_PAYLOAD_LEN+1];
uint           uiRxDataBuffLen;
uint           uiRxPacketCntr;
time_t         tmTimeStamp;
char           szTimeStamp[64];
int8_t         i8LastRssi;
tLoraMsgData   LoraMsgData;
tJsonMessage   JsonMessage;
uint           uiMsgID;
bool           fIsKnownLoraMsgFormat;
int            iMessageToBeProcessed;
char           szMqttTopic[64];
char           szMqttMsg[128];
uint8_t*       pabMqttMsgBuff;
uint           uiMqttMsgBuffLen;
bool           fRxValid;
bool           fMqttReconnect;
int            iIdx;
int            iRes;
bool           fRes;


    //-------------------------------------------------------------------
    // Step(1): Setup
    //-------------------------------------------------------------------
    printf("\n");
    printf("********************************************************************\n");
    printf("  LoRa Packet Receiver\n");
    printf("  Version: %u.%02u\n", APP_VER_MAIN, APP_VER_REL);
    printf("  (c) 2021-2023 Ronald Sieber\n");
    printf("********************************************************************\n");
    printf("\n");


    // setup Workspace
    fRunMainLoop_l   = true;
    pszHostAddr_l    = MQTT_DEF_HOST_URL;
    iPortNum_l       = MQTT_DEF_HOST_PORTNUM;
    pszMsgFileName_l = NULL;
    fProcAllMsg_l    = false;
    fTelemetryMsg_l  = false;
    fOffline_l       = false;
    fVerbose_l       = false;
    uiRxPacketCntr   = 0;
    uiMsgID          = 1;
    fMqttReconnect   = false;


    // evaluate Command Line Arguments
    fRes = AppEvalCmdlnArgs(iArgCnt_p, apszArg_p);
    if ( !fRes )
    {
        AppPrintHelpScreen(apszArg_p[0]);
        return (-1);
    }


    // show sytem start time and runtime configuration
    tmTimeStamp = time(NULL);
    FormatTimeStamp(tmTimeStamp, szTimeStamp, sizeof(szTimeStamp));
    printf("Systen Start Time: %s\n", szTimeStamp);
    printf("\n");
    printf("Runtime Configuration:\n");
    printf("  '-a' ProcAllMsg   = %s\n", (fProcAllMsg_l   ? "yes" : "no"));
    printf("  '-t' TelemetryMsg = %s\n", (fTelemetryMsg_l ? "yes" : "no"));
    printf("  '-o' Offline      = %s\n", (fOffline_l      ? "yes" : "no"));
    printf("  '-v' Verbose      = %s\n", (fVerbose_l      ? "yes" : "no"));
    printf("\n");


    // register Ctrl+C Handler
    signal(SIGINT, AppSigHandler);


    // init bcm2835 I/O Library
    printf("Initialize bcm2835 Library... ");
    iRes = bcm2835_init();
    if ( !iRes )
    {
        printf("\nERROR: bcm2835_init() failed!\n\n");
        return (-2);
    }
    printf("done.\n");


    // setup RF95 Board Configuration
    printf("Setup RF95 Board Configuration...\n");
    printf("  CS  = BCM.%d\n", GPIO_PIN_CS);
    printf("  IRQ = BCM.%d\n", GPIO_PIN_IRQ);
    printf("  RST = BCM.%d\n", GPIO_PIN_RST);
    RF95Setup(GPIO_PIN_CS, GPIO_PIN_IRQ, GPIO_PIN_RST);
    printf("done.\n");


    // configure GPIO Pin for IRQ handling
    printf("Configure IRQ Pin BCM.%d... ", GPIO_PIN_IRQ);
    GpioInit();
    GpioOpen(GPIO_PIN_IRQ, GPIO_DIR_IN);
    GpioSetEdge(GPIO_PIN_IRQ, GPIO_EDGE_RISING);
    printf("done.\n");


    // pulse a reset on RF95 Module
    printf("Reset RF95 Module... ");
    RF95ResetModule();
    printf("done.\n");


    // initialize RF95 Module
    printf("Initialize RF95 Module...\n");
    printf("  Tx Power:  %d\n", RF_TX_POWER);
    printf("  Frequency: %3.2fMHz\n", RF_FREQUENCY);
    iRes = RF95InitModule(RF_TX_POWER, RF_FREQUENCY);
    if (iRes != 0)
    {
        printf("\nERROR: RF95InitModule() failed (iRes=%d)!\n\n", iRes);
        return (-3);
    }
    printf("done.\n");


    // print RF95 Module configuration settings
    if ( fVerbose_l )
    {
        printf("\n");
        printf("RF95 Configuration Settings:\n");
        RF95DiagPrintConfig();
        printf("\n");
    }


    // initialize LoRa Message Qualification
    MquInitialize();


    // create/open MessageFile
    if (pszMsgFileName_l != NULL)
    {
        printf("Create/Open MessageFile ('%s')... ", pszMsgFileName_l);
        iRes = MfwOpen(pszMsgFileName_l);
        if (iRes >= 0)
        {
            printf("done.\n");
        }
        else
        {
            printf("failed (iRes=%d)!\n\n", iRes);
            pszMsgFileName_l = NULL;
        }
    }


    // connect to MQTT Broker
    if ( !fOffline_l )
    {
        printf("Connect to MQTT Broker...\n");
        printf("  HostUrl     = '%s'\n",     pszHostAddr_l);
        printf("  HostPortNum = %u\n",       iPortNum_l);
        printf("  ClientName  = '%s'\n",     MQTT_CLIENT_NAME);
        printf("  KeepAlive   = %u [sec]\n", MQTT_KEEPALIVE_INTERVAL);
        printf("  UserName    = '%s'\n",     MQTT_USER_NAME);
        printf("  Password    = '%s'\n",     MQTT_PASSWORD);
        iRes = MqttConnect(pszHostAddr_l, iPortNum_l, MQTT_CLIENT_NAME, MQTT_KEEPALIVE_INTERVAL, MQTT_USER_NAME, MQTT_PASSWORD);
        if (iRes != 0)
        {
            printf("\nERROR: MqttConnect() failed (iRes=%d)!\n\n", iRes);
            return (-4);
        }
        printf("done.\n");
    }
    else
    {
        printf("Running in Offline Mode, without MQTT connection.\n");
    }
    fMqttReconnect = false;


    //-------------------------------------------------------------------
    // Step(2): Main Loop
    //-------------------------------------------------------------------
    // Main Loop
    printf("\n\n---- Entering Main Loop ----\n");
    while ( fRunMainLoop_l )
    {
        FdSet[0].fd = GpioGetFD(GPIO_PIN_IRQ);
        FdSet[0].events = POLLPRI;
        FdSet[0].revents = 0;

        iRes = poll(FdSet, 1, 1000);
        if (iRes < 0)
        {
            // ignore poll() errors if the application is to be terminated with Ctrl + C
            if ( fRunMainLoop_l )
            {
                printf("\nERROR: poll() failed!\n");
                return (-5);
            }
        }
        if (iRes == 0)
        {
            if ( fVerbose_l )
            {
                printf(".");
                fflush(stdout);
            }
        }
        else
        {
            if (FdSet[0].revents & POLLPRI)
            {
                // catch receive TimeStamp
                tmTimeStamp = time(NULL);
                FormatTimeStamp(tmTimeStamp, szTimeStamp, sizeof(szTimeStamp));

                // Reading the GPIO (with an implicitly lssek()) is necessary after an interrupt has been
                // occured to clear the interrupt event of the file descriptor. Without reading the GPIO
                // the generated event will be signaled forever. As a result the poll() function returns
                // immediately on every call without waiting for the next event.
                //
                // see: https://stackoverflow.com/questions/37620578/poll-not-blocking-returns-immediately
                // "After poll(2) returns, either lseek(2) to the beginning of the sysfs file and
                // read the new value or close the file and re-open it to read the value."
                iGpioState = GpioRead(GPIO_PIN_IRQ);
                TRACE3("\n%s : GPIO BCM.%d interrupt occurred -> GpioState=%d\n", szTimeStamp, GPIO_PIN_IRQ, iGpioState);

                // read received LoRa data package from RF95 Module
                uiRxDataBuffLen = sizeof(abRxDataBuff) - 1;
                fRxValid = RF95GetRecvDataPacket(abRxDataBuff, &uiRxDataBuffLen, &i8LastRssi);
                if ( fRxValid )
                {
                    uiRxPacketCntr++;
                    printf("\n%s : LoRa Message received (LoRaPacket: %04u, RSSI: %d [dB])\n", szTimeStamp, uiRxPacketCntr, (int)i8LastRssi);
                    if ( fVerbose_l )
                    {
                        DumpDataBuffer(abRxDataBuff, uiRxDataBuffLen);
                    }

                    // decode and evaluate received LoRa message data package
                    iRes = PprGainLoraDataRecord(uiMsgID, tmTimeStamp, i8LastRssi,
                                                 abRxDataBuff, uiRxDataBuffLen,
                                                 &LoraMsgData, &fIsKnownLoraMsgFormat);
                    if (iRes != 0)
                    {
                        printf("\nERROR: PprGainLoraDataRecord() failed (iRes=%d)!\n\n", iRes);
                        continue;
                    }
                    if ( !fIsKnownLoraMsgFormat )
                    {
                        printf("\nINVALID DATA: Unknown LoRa Message Format\n\n", iRes);
                        continue;
                    }
                    printf("DevID: %02u, LoraPacketType: %s\n", (uint)LoraMsgData.m_iLoraDevID, GetLoraPacketTypeName(LoraMsgData.m_LoraPacketType));
                    if ( fVerbose_l )
                    {
                        PprPrintLoraDataRecord(&LoraMsgData);
                    }

                    // build JSON Message List from received LoRa Packet
                    iRes = PprBuildJsonMessages(&LoraMsgData, &vecJsonMessages);
                    if (iRes < 0)
                    {
                        printf("\nERROR: PprBuildJsonMessages() failed (iRes=%d)!\n\n", iRes);
                        continue;
                    }

                    if ( fVerbose_l )
                    {
                        printf("\n=== JSON Messages ===\n");
                    }

                    // evaluate Message by Messge from List, whether it is to be processed or not
                    // (by passing the loop from the last to the first element, the messages are processed
                    // in the order Gen2/Gen1/Gen0 from LoRa Packet, so that when publishing the MQTT messages
                    // their historical order keeps preserved)
                    for (iIdx=vecJsonMessages.size()-1; iIdx>=0; iIdx--)
                    {
                        JsonMessage = vecJsonMessages.at(iIdx);
                        if ( !fProcAllMsg_l )
                        {
                            // check if Message is to be processed (ignore duplicates)
                            iMessageToBeProcessed = MquIsMessageToBeProcessed(&JsonMessage);
                            if (iMessageToBeProcessed < 1)
                            {
                                if ( fVerbose_l )
                                {
                                    printf(" Ignore JsonMessage[%d]\n", iIdx);
                                }
                                // skip Message to be ignored
                                continue;
                            }
                        }

                        // continue with Message to be processed
                        if ( fVerbose_l )
                        {
                            printf(" Process JsonMessage[%d]:\n", iIdx);
                            PprPrintJsonMessage(&JsonMessage);
                            if ( !fProcAllMsg_l )
                            {
                                MquPrintSequNumHistList();
                            }
                            printf("\n");
                        }
                        // log Message to MessageFile
                        if (pszMsgFileName_l != NULL)
                        {
                            MfwWriteMessage(&JsonMessage);
                        }

                        // send received LoRa Message to MQTT Broker
                        if ( !fOffline_l )
                        {
                            // send Bootup or Data Message to MQTT Broker
                            BuildMqttPublishTopic(&JsonMessage, szMqttTopic, sizeof(szMqttTopic));
                            pabMqttMsgBuff = (uint8_t*)JsonMessage.m_strJsonRecord.c_str();
                            uiMqttMsgBuffLen = (uint)JsonMessage.m_strJsonRecord.length();
                            if ( fVerbose_l )
                            {
                                MqttPrintMessage(szMqttTopic, pabMqttMsgBuff, uiMqttMsgBuffLen);
                            }
                            printf("Send received LoRa Message to MQTT Broker (LoRaPacket[%04u])... ", uiRxPacketCntr);
                            iRes = MqttPublishMessage(szMqttTopic, pabMqttMsgBuff, uiMqttMsgBuffLen, kMqttQoS0, 1);
                            if (iRes != 0)
                            {
                                printf("\nERROR: MqttPublishMessage() failed (iRes=%d)!\n\n", iRes);
                                fMqttReconnect = true;
                            }
                            else
                            {
                                printf("done.\n");
                                if ( fVerbose_l )
                                {
                                    printf("\n");
                                }
                            }

                            // send Telemetry Data Message to MQTT Broker
                            if ( fTelemetryMsg_l )
                            {
                                PprBuildTelemetryMessage(&JsonMessage, (uint8_t*)szMqttMsg, sizeof(szMqttMsg));
                                printf("Send Telemetry Data Message to MQTT Broker (LoRaPacket[%04u])... ", uiRxPacketCntr);
                                iRes = MqttPublishMessage(MQTT_TOPIC_TELEMETRY, (uint8_t*)szMqttMsg, strlen(szMqttMsg), kMqttQoS0, 1);
                                if (iRes != 0)
                                {
                                    printf("\nERROR: MqttPublishMessage() failed (iRes=%d)!\n\n", iRes);
                                    fMqttReconnect = true;
                                }
                                else
                                {
                                    printf("done.\n");
                                    if ( fVerbose_l )
                                    {
                                        printf("\n");
                                    }
                                }
                            }
                        }
                    }

                    uiMsgID++;
                }   // if ( fRxValid )
            }   // if (FdSet[0].revents & POLLPRI)
        }

        // send KeepAlive
        if ( !fOffline_l )
        {
            iRes = MqttKeepAlive(MQTT_TOPIC_KEEPALVIE);
            if (iRes < 0)
            {
                printf("\nERROR: MqttKeepAlive() failed (iRes=%d)!\n\n", iRes);
                fMqttReconnect = true;
            }
            if (iRes > 0)
            {
                if ( fVerbose_l )
                {
                    printf("[KA]");
                }
            }
        }

        // ErrorHandling: reconnet to MQTT Broker if necessary
        if ( !fOffline_l )
        {
            if ( fMqttReconnect )
            {
                printf("\nReanimation: Reconnect to MQTT Broker... ");
                iRes = MqttReconnect();
                if (iRes != 0)
                {
                    printf("\nERROR: MqttReconnect() failed (iRes=%d)!\n\n", iRes);
                    fMqttReconnect = true;
                }
                else
                {
                    printf("done.\n");
                    if ( fVerbose_l )
                    {
                        printf("\n");
                    }
                    fMqttReconnect = false;
                }
            }
        }

        fflush(stdout);
    }


    // disconnect from MQTT Broker
    if ( !fOffline_l )
    {
        printf("Disconnect from MQTT Broker...\n");
        iRes = MqttDisconnect();
        if (iRes != 0)
        {
            printf("\nERROR: MqttDisconnect() failed (iRes=%d)!\n\n", iRes);
        }
        else
        {
            printf("done.\n");
        }
    }

    // close MessageFile
    if (pszMsgFileName_l != NULL)
    {
        printf("Close MessageFile... ");
        MfwClose();
        printf("done.\n");
    }

    // close bcm2835 I/O Library
    printf("Close bcm2835 Library... ");
    bcm2835_close();
    printf("done.\n");


    return (0);

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Application signal handler
//---------------------------------------------------------------------------

static  void  AppSigHandler (
    int iSignalNum_p)
{

    printf("\n\nTerminate Application\n\n");
    fRunMainLoop_l = false;

    return;

}


//---------------------------------------------------------------------------
//  Evaluate command line arguments
//---------------------------------------------------------------------------

static  bool  AppEvalCmdlnArgs (
    int iArgCnt_p,
    char* apszArg_p[])
{

char*  pszArg;
int    iIdx;
bool   fRes;


    fRes = true;

    for (iIdx=1; iIdx<iArgCnt_p; iIdx++)
    {
        pszArg = apszArg_p[iIdx];
        if (pszArg != NULL)
        {
            // argument '-h=' -> Host ('url:port')
            if ( !strncasecmp("-h=", pszArg, sizeof("-h=")-1) )
            {
                pszArg += sizeof("-h=")-1;
                fRes = SplitupUrlString(pszArg, &pszHostAddr_l, &iPortNum_l);
                if ( !fRes )
                {
                    printf("\nERROR: invalid host address!\n");
                    break;
                }
                continue;
            }

            // argument '-l=' -> MessageFile
            if ( !strncasecmp("-l=", pszArg, sizeof("-l=")-1) )
            {
                pszArg += sizeof("-l=")-1;
                pszMsgFileName_l = pszArg;
                continue;
            }

            // argument '-a' -> All Messages (including duplicates)
            if ( !strncasecmp("-a", pszArg, sizeof("-a")-1) )
            {
                fProcAllMsg_l = true;
                continue;
            }

            // argument '-t' -> TelemetryDataMessages
            if ( !strncasecmp("-t", pszArg, sizeof("-t")-1) )
            {
                fTelemetryMsg_l = true;
                continue;
            }

            // argument '-o' -> Offline
            if ( !strncasecmp("-o", pszArg, sizeof("-o")-1) )
            {
                fOffline_l = true;
                continue;
            }

            // argument '-v' -> Verbose
            if ( !strncasecmp("-v", pszArg, sizeof("-v")-1) )
            {
                fVerbose_l = true;
                continue;
            }
        }

        fRes = false;
    }

    return (fRes);

}



//---------------------------------------------------------------------------
//  Show Help Screen
//---------------------------------------------------------------------------

static  void  AppPrintHelpScreen (
    const char* pszArg0_p)
{

    //     |    10   |    20   |    30   |    40   |    50   |    60   |    70   |    80   |
    printf("Usage:\n");
    printf("   sudo %s [OPTION]\n", pszArg0_p);
    printf("   OPTION:\n");
    printf("\n");
    printf("       -h=<host_url>   Host URL of MQTT Broker in format URL[:Port]\n");
    printf("                       (default: %s:%d)\n", MQTT_DEF_HOST_URL, MQTT_DEF_HOST_PORTNUM);
    printf("\n");
    printf("       -l=<msg_file>   Logs all Messages sent via MQTT to the specified file\n");
    printf("                       (the file is opened always in APPEND mode)\n");
    printf("\n");
    printf("       -a              Process all received LoRa Packets, including duplicates\n");
    printf("\n");
    printf("       -t              Send Telemetry Data Messages to MQTT Broker\n");
    printf("\n");
    printf("       -o              Run in Offline Mode, without MQTT connection\n");
    printf("\n");
    printf("       -v              Run in Verbose Mode\n");
    printf("\n");
    printf("       --help          Shows this Help Screen\n");
    printf("\n");
    printf("       Known Bugs:     Running without 'sudo' leads to a segmentation fault in\n");
    printf("                       lib 'RadioHead' (github.com/idreamsi/RadioHead)\n");
    printf("\n");

    return;

}



//---------------------------------------------------------------------------
//  Build MQTT Publish Topic
//---------------------------------------------------------------------------

static  int  BuildMqttPublishTopic (
    const tJsonMessage* pJsonMessage_p,
    char* pszTopicBuffer_p,
    int iTopicBuffSize_p)
{

tLoraPacketType  PacketType;
uint             uiDevID;
const char*      pszTopicTemplate;


    if ( (pJsonMessage_p   == NULL) ||
         (pszTopicBuffer_p == NULL)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    PacketType = pJsonMessage_p->m_PacketType;
    uiDevID    = (uint)pJsonMessage_p->m_ui8DevID;

    if (uiDevID >= LORA_DEVICES)
    {
        TRACE0("ERROR: Invalid DevID!\n");
        return (-2);
    }

    switch (PacketType)
    {
        case kLoraPacketBootup:
        {
            pszTopicTemplate = MQTT_TOPIC_TMPL_BOOTUP;
            break;
        }

        case kLoraPacketDataGen0:
        case kLoraPacketDataGen1:
        case kLoraPacketDataGen2:
        {
            pszTopicTemplate = MQTT_TOPIC_TMPL_ST_DATA;
            break;
        }

        default:
        {
            TRACE1("ERROR: Unexpected PacketType (%d)!\n", (int)pJsonMessage_p->m_PacketType);
            return (-3);
        }
    }

    // build MQTT Publish Topic
    snprintf(pszTopicBuffer_p, iTopicBuffSize_p, pszTopicTemplate, uiDevID);

    return (0);

}



//---------------------------------------------------------------------------
//  Get LoRa Packet Type Name
//---------------------------------------------------------------------------

static  const char*  GetLoraPacketTypeName (
    tLoraPacketType PacketType_p)
{

static const char*  pszPacketTypeName;


    switch (PacketType_p)
    {
        case kLoraPacketBootup:
        {
            pszPacketTypeName = "Bootup";
            break;
        }

        case kLoraPacketDataHeader:
        {
            pszPacketTypeName = "StatonData";
            break;
        }

        case kLoraPacketDataGen0:
        case kLoraPacketDataGen1:
        case kLoraPacketDataGen2:
        {
            pszPacketTypeName = "{DataGen0/1/2}";
            break;
        }

        default:
        {
            pszPacketTypeName = "?unknown?";
            break;
        }
    }

    return (pszPacketTypeName);

}



//---------------------------------------------------------------------------
//  Get current Date/Time as String
//---------------------------------------------------------------------------

static  int  GetCurrentDateTime (
    char* pszBuffer_p,
    int iBuffSize_p)
{

time_t      tmTimeStamp;
struct tm*  LocTime;
int         iStrLen;


    tmTimeStamp = time(NULL);
    iStrLen = FormatTimeStamp(tmTimeStamp, pszBuffer_p, iBuffSize_p);

    return (iStrLen);

}



//---------------------------------------------------------------------------
//  Format TimeStamp as Date/Time String
//---------------------------------------------------------------------------

static  int  FormatTimeStamp (
    time_t tmTimeStamp_p,
    char* pszBuffer_p,
    int iBuffSize_p)
{

struct tm*  LocTime;
int         iStrLen;


    LocTime = localtime(&tmTimeStamp_p);

    snprintf(pszBuffer_p, iBuffSize_p, "%04d/%02d/%02d - %02d:%02d:%02d",
             LocTime->tm_year + 1900, LocTime->tm_mon + 1, LocTime->tm_mday,
             LocTime->tm_hour, LocTime->tm_min, LocTime->tm_sec);

    iStrLen = strlen(pszBuffer_p);

    return (iStrLen);

}



//---------------------------------------------------------------------------
//  Split-up URL string into IP-Address and PortNum
//---------------------------------------------------------------------------

static  bool  SplitupUrlString (
    const char* pszUrlString_p,
    const char** ppszIpAddr_p,
    int* piPortNum_p)
{

const char*  pszIpAddr;
char*        pszSeparator;
int          iPortNum;
int          iRes;


    *ppszIpAddr_p = NULL;
    *piPortNum_p  = 0;

    if (strlen(pszUrlString_p) >= sizeof(szServerUrl_l))
    {
        return (false);
    }

    strncpy(szServerUrl_l, pszUrlString_p, sizeof(szServerUrl_l));
    pszIpAddr = szServerUrl_l;

    pszSeparator = strstr(szServerUrl_l, ":");
    if (pszSeparator != NULL)
    {
        *pszSeparator = '\0';

        pszSeparator++;
        iRes = sscanf(pszSeparator, "%d", &iPortNum);
        if (iRes != 1)
        {
            return (false);
        }
    }
    else
    {
        iPortNum = MQTT_DEF_HOST_PORTNUM;
    }

    *ppszIpAddr_p = pszIpAddr;
    *piPortNum_p  = iPortNum;

    return (true);

}



//---------------------------------------------------------------------------
//  PrintDataBuffer
//---------------------------------------------------------------------------

static  void  PrintDataBuffer (
    const void* pabDataBuff_p,
    unsigned int uiDataBuffLen_p)
{

const unsigned char*  pabBuffData;
uint8_t               ui8DataByte;
uint                  uiIdx;


    pabBuffData = (const unsigned char*)pabDataBuff_p;

    for (uiIdx=0; uiIdx<uiDataBuffLen_p; uiIdx++)
    {
        ui8DataByte = pabBuffData[uiIdx];
        if ((ui8DataByte <= 0x1F) || (ui8DataByte >= 0x7F))
        {
            ui8DataByte = '.';
        }
        printf("%c", ui8DataByte);
    }
    printf("'");

    return;

}



//---------------------------------------------------------------------------
//  DumpDataBuffer
//---------------------------------------------------------------------------

static  void  DumpDataBuffer (
    const void* pabDataBuff_p,
    unsigned int uiDataBuffLen_p)
{

#define COLUMNS_PER_LINE    16

const unsigned char*  pabBuffData;
unsigned int          uiBuffSize;
unsigned char         bData;
int                   nRow;
int                   nCol;

    // get pointer to buffer and length of buffer
    pabBuffData = (const unsigned char*)pabDataBuff_p;
    uiBuffSize  = (unsigned int)uiDataBuffLen_p;


    // dump buffer contents
    for (nRow=0; ; nRow++)
    {
        printf("\n%04lX:   ", (unsigned long)(nRow*COLUMNS_PER_LINE));

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            if ((unsigned int)nCol < uiBuffSize)
            {
                printf("%02X ", (unsigned int)*(pabBuffData+nCol));
            }
            else
            {
                printf("   ");
            }
        }

        printf(" ");

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            bData = *pabBuffData++;
            if ((unsigned int)nCol < uiBuffSize)
            {
                if ((bData >= 0x20) && (bData < 0x7F))
                {
                    printf("%c", bData);
                }
                else
                {
                    printf(".");
                }
            }
            else
            {
                printf(" ");
            }
        }

        if (uiBuffSize > COLUMNS_PER_LINE)
        {
            uiBuffSize -= COLUMNS_PER_LINE;
        }
        else
        {
            break;
        }
    }

    printf("\n");

    return;

}



// EOF






