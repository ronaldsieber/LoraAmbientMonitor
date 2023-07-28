/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of LoRa Packet Processor

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#ifndef _WIN64
    #include <RH_RF95.h>
#else
    #define _CRT_SECURE_NO_WARNINGS
    typedef  unsigned int  uint;
    #define RH_RF95_MAX_PAYLOAD_LEN 255
#endif
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <algorithm>
#include <time.h>
#include "LoraPacket.h"
#include "LoraPayloadDecoder.h"
#include "PacketProcessing.h"
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



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  int  PprReconstructStationData (
    tLoraMsgData* pLoraMsgData_p);                      // [IN]     Ptr to LoRa Data Record


static  std::string  PprLogReconstructStationData (
    tLoraMsgData* pLoraMsgData_p);                      // [IN]     Ptr to LoRa Data Record


static  int  PprBuildJsonMessagesStationBootup (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p);     // [IN/OUT] Ptr to Vector with Json Messages


static  int  PprBuildJsonMessagesStationData (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p);     // [IN/OUT] Ptr to Vector with Json Messages


static  std::string  PprFormatTimeStamp (
    time_t tmTimeStamp_p);


static  int  PprFormatTimeStamp (
    time_t tmTimeStamp_p,
    char* pszBuffer_p,
    int iBuffSize_p);


static  std::string  PprFormatUptime (
    uint32_t ui32Uptime_p,
    bool fForceDay_p = true,
    bool fForceTwoDigitsHours_p = true);


static  int  PprFormatUptime (
    uint32_t ui32Uptime_p,
    char* pszBuffer_p,
    int iBuffSize_p,
    bool fForceDay_p = true,
    bool fForceTwoDigitsHours_p = true);





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  PprGainLoRaDataRecord
//---------------------------------------------------------------------------

int  PprGainLoraDataRecord (
    uint uiMsgID_p,                                     // [IN]     MessageID (e.g. RxPacketCntr)
    time_t tmTimeStamp_p,                               // [IN]     Message Receive TimeStamp
    int8_t i8Rssi_p,                                    // [IN]     Message Receive RSSI Level
    const uint8_t* pabRxDataBuff_p,                     // [IN]     Ptr to Message to decode
    uint uiRxDataBuffLen_p,                             // [IN]     Length of Message to decode
    tLoraMsgData* pLoraMsgData_p,                       // [IN/OUT] Ptr to LoRa Data Record to fill out
    bool* pfIsKnownLoraMsgFormat_p)                     // [IN/OUT] Ptr to Flag to signal Known LoRa Data Format or not
{

LoraPayloadDecoder  LoraPayloadDec;
tLoraDataPacket*    pLoraDataPacket;
tLoraPacketType     LoraPacketType;
uint                uiRxDataBuffLen;
bool                fIsKnownLoraMsgFormat;


    TRACE0("MsgGainLoRaDataRecord:\n");
    TRACE1("    uiMsgID_p=%u\n", uiMsgID_p);
    TRACE2("    pabRxDataBuff_p=0x%08lX, uiRxDataBuffLen_p=%u\n", pabRxDataBuff_p, uiRxDataBuffLen_p);


    // check parameter
    if ( (pabRxDataBuff_p          == NULL) ||
         (pLoraMsgData_p           == NULL) ||
         (pfIsKnownLoraMsgFormat_p == NULL) ||
         (uiRxDataBuffLen_p        < 5)      )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }


    // set initial values
    fIsKnownLoraMsgFormat = false;
    *pfIsKnownLoraMsgFormat_p = fIsKnownLoraMsgFormat;
    pLoraMsgData_p->m_LoraPacketType = kLoraPacketInvalid;
    pLoraMsgData_p->m_iLoraDevID = -1;
    pLoraMsgData_p->m_strLogLoraMsgData = "";


    // save LoRa message MetaData
    pLoraMsgData_p->m_uiMsgID = uiMsgID_p;
    pLoraMsgData_p->m_tmTimeStamp = tmTimeStamp_p;
    pLoraMsgData_p->m_i8Rssi = i8Rssi_p;


    // save LoRa message RawData
    uiRxDataBuffLen = uiRxDataBuffLen_p;
    if (uiRxDataBuffLen > sizeof(pLoraMsgData_p->m_abRawLoraMsg))
    {
        uiRxDataBuffLen = sizeof(pLoraMsgData_p->m_abRawLoraMsg);
    }
    memcpy(pLoraMsgData_p->m_abRawLoraMsg, pabRxDataBuff_p, uiRxDataBuffLen);
    pLoraMsgData_p->m_uiRawLoraMsgLen = uiRxDataBuffLen;


    // Analyse LoRa Message Payload
    TRACE0("    Checking plausibility of length of received data packet:\n");
    TRACE1("      Size expected (sizeof(tLoraDataPacket)): %u\n", sizeof(tLoraDataPacket));
    TRACE1("      Size received:                           %u\n", uiRxDataBuffLen);
    if (uiRxDataBuffLen == sizeof(tLoraDataPacket))
    {
        TRACE0("        -> Size Match\n");

        pLoraDataPacket = (tLoraDataPacket*)pabRxDataBuff_p;
        LoraPacketType = LoraPayloadDec.GetRxPacketType(pLoraDataPacket);
        switch (LoraPacketType)
        {
            case kLoraPacketUnused:
            {
                TRACE0("\n");
                TRACE1("Packet %u: Unexpected Packet of Type <kLoraPacketUnused>\n", uiMsgID_p);
                fIsKnownLoraMsgFormat = false;
                break;
            }

            case kLoraPacketBootup:
            {
                pLoraMsgData_p->m_LoraPacketType = LoraPacketType;
                pLoraMsgData_p->m_iLoraDevID = (int)LoraPayloadDec.GetRxPacketDevID(pLoraDataPacket);
                pLoraMsgData_p->m_LoraStationBootup = LoraPayloadDec.DecodeRxBootupPacket(pLoraDataPacket);
                pLoraMsgData_p->m_strLogLoraMsgData = LoraPayloadDec.LogStationBootup(&pLoraMsgData_p->m_LoraStationBootup);
                fIsKnownLoraMsgFormat = true;
                break;
            }

            case kLoraPacketDataHeader:
            {
                pLoraMsgData_p->m_LoraPacketType = LoraPacketType;
                pLoraMsgData_p->m_iLoraDevID = (int)LoraPayloadDec.GetRxPacketDevID(pLoraDataPacket);
                pLoraMsgData_p->m_LoraStationData = LoraPayloadDec.DecodeRxDataPacket(pLoraDataPacket);
                PprReconstructStationData(pLoraMsgData_p);
                pLoraMsgData_p->m_strLogLoraMsgData  = LoraPayloadDec.LogStationData(&pLoraMsgData_p->m_LoraStationData);
                pLoraMsgData_p->m_strLogLoraMsgData += PprLogReconstructStationData(pLoraMsgData_p);
                fIsKnownLoraMsgFormat = true;
                break;
            }

            case kLoraPacketDataGen0:
            case kLoraPacketDataGen1:
            case kLoraPacketDataGen2:
            {
                TRACE0("\n");
                TRACE1("Packet %u: Unexpected Packet of Type <kLoraPacketDataGen0/1/2>\n", uiMsgID_p);
                fIsKnownLoraMsgFormat = false;
                break;
            }

            default:
            {
                TRACE0("\n");
                TRACE1("Packet %u: Unexpected Packet of unknown Type\n", uiMsgID_p);
                fIsKnownLoraMsgFormat = false;
                break;
            }
        }
    }
    else
    {
        TRACE0("        -> Size Mismatch\n");
        fIsKnownLoraMsgFormat = false;
    }


    *pfIsKnownLoraMsgFormat_p = fIsKnownLoraMsgFormat;

    return (0);

}



//---------------------------------------------------------------------------
//  PprBuildJsonMessages
//---------------------------------------------------------------------------

int  PprBuildJsonMessages (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p)      // [IN/OUT] Ptr to Vector with Json Messages
{

int  iRes;


    // check parameter
    if ( (pLoraMsgData_p     == NULL) ||
         (pvecJsonMessages_p == NULL)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    // clear Message Vector
    pvecJsonMessages_p->clear();

    // build Message depending on LoraPacketType
    switch (pLoraMsgData_p->m_LoraPacketType)
    {
        case kLoraPacketBootup:
        {
            TRACE0("  LoraPacketType: kLoraPacketBootup\n");
            iRes = PprBuildJsonMessagesStationBootup(pLoraMsgData_p, pvecJsonMessages_p);
            break;
        }

        case kLoraPacketDataHeader:
        {
            TRACE0("  LoraPacketType: kLoraPacketDataHeader\n");
            iRes = PprBuildJsonMessagesStationData(pLoraMsgData_p, pvecJsonMessages_p);
            break;
        }

        default:
        {
            TRACE1("  Unexpected LoraPacketType (%d)\n", pLoraMsgData_p->m_LoraPacketType);
            break;
        }
    }

    return (0);

}



//---------------------------------------------------------------------------
//  PprBuildTelemetryMessage
//---------------------------------------------------------------------------

int  PprBuildTelemetryMessage (
    const tJsonMessage* pJsonMessage_p,                 // [IN]     Json Message with Telemetry Data
    uint8_t* pabMsgBuffer_p,                            // [IN]     Ptr to Message Buffer
    uint uiMsgBufferLen_p)                              // [IN]     Length of Message Buffer
{

std::string  strTimeStamp;


    if ( (pJsonMessage_p == NULL) ||
         (pabMsgBuffer_p == NULL)  )
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }


    // format TimeStamp and delete all spaces to get a compact string version
    strTimeStamp = PprFormatTimeStamp(pJsonMessage_p->m_tmTimeStamp);
    strTimeStamp.erase(remove(strTimeStamp.begin(), strTimeStamp.end(), ' '), strTimeStamp.end());

    // build string with Telemetry information
    snprintf((char*)pabMsgBuffer_p, uiMsgBufferLen_p, "Time=%s, MsgID=%u, Dev=%u, Seq=%u, RSSI=%d",
                                                      strTimeStamp.c_str(),
                                                      pJsonMessage_p->m_uiMsgID,
                                                      (uint)pJsonMessage_p->m_ui8DevID,
                                                      (uint)pJsonMessage_p->m_ui32SequNum,
                                                      (int)pJsonMessage_p->m_i8Rssi);

    return (0);

}



//---------------------------------------------------------------------------
//  PprPrintLoRaDataRecord
//---------------------------------------------------------------------------

int  PprPrintLoraDataRecord (
    const tLoraMsgData* pLoraMsgData_p)                 // [IN]     Ptr to LoRa Data Record to fill out
{

    if (pLoraMsgData_p == NULL)
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }


    // LoRa Message Basic Data
    printf("\n");
    printf("LoRa Data Record:\n");
    printf("  MsgID:   %u\n",        pLoraMsgData_p->m_uiMsgID);
    printf("  Time:    %s\n",        PprFormatTimeStamp(pLoraMsgData_p->m_tmTimeStamp).c_str());
    printf("  RSSI:    %d [dB]\n",   (int)pLoraMsgData_p->m_i8Rssi);
    printf("  Length:  %u [Byte]\n", pLoraMsgData_p->m_uiRawLoraMsgLen);


    // LoRa Message Payload Data
    switch (pLoraMsgData_p->m_LoraPacketType)
    {
        case kLoraPacketBootup:
        {
            printf("\n");
            printf("-----------------------------\n");
            printf("Packet %u: BOOTUP PACKET\n", pLoraMsgData_p->m_uiMsgID);
            printf("-----------------------------\n");
            printf(pLoraMsgData_p->m_strLogLoraMsgData.c_str());
            break;
        }

        case kLoraPacketDataHeader:
        {
            printf("\n");
            printf("-----------------------------\n");
            printf("Packet %u: DATA PACKET\n", pLoraMsgData_p->m_uiMsgID);
            printf("-----------------------------\n");
            printf(pLoraMsgData_p->m_strLogLoraMsgData.c_str());
            break;
        }

        case kLoraPacketUnused:
        case kLoraPacketDataGen0:
        case kLoraPacketDataGen1:
        case kLoraPacketDataGen2:
        {
            printf("\n");
            printf("Packet %u: Unexpected Packet of Type <kLoraPacketUnused> or <kLoraPacketDataGen0/1/2>\n", pLoraMsgData_p->m_uiMsgID);
            break;
        }

        default:
        {
            printf("\n");
            printf("Packet %u: Unexpected Packet of unknown Type\n", pLoraMsgData_p->m_uiMsgID);
            break;
        }
    }


    return (0);

}



//---------------------------------------------------------------------------
//  PprPrintJsonMessages
//---------------------------------------------------------------------------

int  PprPrintJsonMessages (
    std::vector<tJsonMessage>* pvecJsonMessages_p)      // [IN]     Ptr to Vector with Json Messages
{

tJsonMessage  JsonMessage;
int           iIdx;


    if (pvecJsonMessages_p == NULL)
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    for (iIdx=0;iIdx<pvecJsonMessages_p->size();iIdx++)
    {
        JsonMessage = pvecJsonMessages_p->at(iIdx);
        printf(" JsonMessage[%d]:\n", iIdx);
        PprPrintJsonMessage(&JsonMessage);
    }

    return (0);

}



//---------------------------------------------------------------------------
//  PprPrintJsonMessage
//---------------------------------------------------------------------------

int  PprPrintJsonMessage (
    tJsonMessage* pJsonMessage_p)                       // [IN]     Json Message to print
{

    if (pJsonMessage_p == NULL)
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    printf("  MsgID: %d\n", pJsonMessage_p->m_uiMsgID);
    printf("  PacketType: ");
    switch (pJsonMessage_p->m_PacketType)
    {
        case kLoraPacketUnused:         printf("kLoraPacketUnused");          break;
        case kLoraPacketBootup:         printf("kLoraPacketBootup");          break;
        case kLoraPacketDataHeader:     printf("kLoraPacketDataHeader");      break;
        case kLoraPacketDataGen0:       printf("kLoraPacketDataGen0");        break;
        case kLoraPacketDataGen1:       printf("kLoraPacketDataGen1");        break;
        case kLoraPacketDataGen2:       printf("kLoraPacketDataGen2");        break;
        default:                        printf("???");                        break;
    }
    printf("\n");
    printf("  DevID: %d\n", pJsonMessage_p->m_ui8DevID);
    printf("  SequNum: %d\n", pJsonMessage_p->m_ui32SequNum);
    printf("  JsonRecord:\n%s\n", pJsonMessage_p->m_strJsonRecord.c_str());

    return (0);

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  PprReconstructStationData
//---------------------------------------------------------------------------

static  int  PprReconstructStationData (
    tLoraMsgData* pLoraMsgData_p)                       // [IN]     Ptr to LoRa Data Record
{

uint   nDataGen;
uint   uiUptimeHeader;
uint   ui12UptimeHeaderSnippet;
uint   uiUptimeSnippetDiff;


    memset(pLoraMsgData_p->m_aLoraStationDataReconstruct, 0x00, sizeof(pLoraMsgData_p->m_aLoraStationDataReconstruct));

    // check validity of Header
    if (pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_DataStatus != LoraPayloadDecoder::kStatusValid)
    {
        // reconstruction of StationData requires an intact Header
        TRACE0("ERROR: PacketHeader is corrupt!\n");
        return (-1);
    }

    for (nDataGen=0; nDataGen<(sizeof(pLoraMsgData_p->m_LoraStationData.m_aDataRec)/sizeof(LoraPayloadDecoder::tDataRec)); nDataGen++)
    {
        // check validity
        if (pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_DataStatus != LoraPayloadDecoder::kStatusValid)
        {
            // skip invalid Data records (unused or invalid CRC)
            TRACE0("INFO: Skip invalid Data records (unused or invalid CRC)\n");
            continue;
        }

        // ---- Reconstruct SequenceNumber ----
        // The DataRecords included in a LoRa Package are 3 consecutive generations (Gen0/Gen1/Gen2).
        // Therefore the SequenceNumber for Gen1 and Gen2 is obtained by decreasing the SequenceNumber
        // of the DataHeader accordingly
        pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32SequNum = pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui32SequNum - nDataGen;

        // ---- Reconstruct Uptime und TimeStamp----
        // The DataHeader of a LoRa packet contains the complete uptime as 32Bit value (2^32 seconds => 136 years).
        // In contrast, the UptimeSnippets in the individual generation-based DataRecords are reduced to 12 bits
        // in 10 second steps (2^12 = 4096 * 10 seconds => 11 hours)
        if (pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_PacketType == kLoraPacketDataGen0)
        {
            // for Gen0 DataRecord the uptime is taken directly from the DataHeader
            pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime = pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui32Uptime;

            // for Gen0 DataRecord the TimeStamp is set to the receiving TimeStamp
            pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_tmTimeStamp = pLoraMsgData_p->m_tmTimeStamp;
        }
        else
        {
            // for Gen1 and Gen2 DataRecords the uptime is reconstructed from uptime of the DataHeader reduced by
            // the uptime difference between Gen1 or Gen2 and DataHeader
            //     tDataHeader.m_ui32Uptime      -> Uptime        32Bit  -> 0..4294967296 [sec] resp. 0..136 [year]
            //     tDataRec.m_ui12UptimeSnippet  -> UptimeSnippet 12Bit  -> 0..4095 [10 sec] resp. 0..11 [h]
            uiUptimeHeader = pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui32Uptime;
            ui12UptimeHeaderSnippet = ((uiUptimeHeader / 10) & 0x0FFF) * 10;    // same as UpdateSnippet processing (Encode -> LoRa Transmit -> Decode)
            uiUptimeSnippetDiff = ui12UptimeHeaderSnippet - pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_ui12UptimeSnippet;
            uiUptimeSnippetDiff &= 0x0FFF;                                      // reduce difference to 12Bit
            pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime = pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui32Uptime - uiUptimeSnippetDiff;

            // for Gen1 and Gen2 DataRecords the TimeStamp is reconstructed from receiving TimeStamp
            // the uptime difference between Gen1 or Gen2 and DataHeader
            pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_tmTimeStamp = pLoraMsgData_p->m_tmTimeStamp - uiUptimeSnippetDiff;
        }
    }

    return (0);

}



//---------------------------------------------------------------------------
//  PprLogReconstructStationData
//---------------------------------------------------------------------------

static  std::string  PprLogReconstructStationData (
    tLoraMsgData* pLoraMsgData_p)                       // [IN]     Ptr to LoRa Data Record
{

std::string  strLogData;
char         szLogBuffer[256];
uint         nDataGen;


    strLogData.clear();
    strLogData += " === ReconstructStationData ===\n";
    strLogData += " *ReconstructDataRecords*\n";
    for (nDataGen=0; nDataGen<(sizeof(pLoraMsgData_p->m_aLoraStationDataReconstruct)/sizeof(tLoraStationDataReconstruct)); nDataGen++)
    {
        snprintf(szLogBuffer, sizeof(szLogBuffer), "  ReconstructDataRecord[%d]:\n", nDataGen);
        strLogData += szLogBuffer;
        snprintf(szLogBuffer, sizeof(szLogBuffer), "   SequNum:               %u\n", (unsigned)pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32SequNum);
        strLogData += szLogBuffer;
        snprintf(szLogBuffer, sizeof(szLogBuffer), "   Uptime:                %u -> %s\n", (unsigned)pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime,
                                                                                           PprFormatUptime(pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime).c_str());
        strLogData += szLogBuffer;
        snprintf(szLogBuffer, sizeof(szLogBuffer), "   Timestamp:             %s\n", PprFormatTimeStamp(pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_tmTimeStamp).c_str());
        strLogData += szLogBuffer;
    }

    return (strLogData);

}



//---------------------------------------------------------------------------
//  PprBuildJsonMessagesStationBootup
//---------------------------------------------------------------------------

static  int  PprBuildJsonMessagesStationBootup (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p)      // [IN/OUT] Ptr to Vector with Json Messages
{

tJsonMessage  JsonMessage;
std::string   strJsonRecord;
char          szJsonItem[256];


    // clear Message Vector
    pvecJsonMessages_p->clear();

    // check validity
    if (pLoraMsgData_p->m_LoraStationBootup.m_DataStatus != LoraPayloadDecoder::kStatusValid)
    {
        return (-1);
    }

    // build Json Record
    strJsonRecord = "{\n";
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"MsgID\": %u,\n", pLoraMsgData_p->m_uiMsgID);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"MsgType\": \"StationBootup\",\n");
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"TimeStamp\": %u,\n", (unsigned)pLoraMsgData_p->m_tmTimeStamp);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"TimeStampFmt\": \"%s\",\n", PprFormatTimeStamp(pLoraMsgData_p->m_tmTimeStamp).c_str());
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"RSSI\": %d,\n", (int)pLoraMsgData_p->m_i8Rssi);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"DevID\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui8DevID);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"FirmwareVer\": \"%u.%02u\",\n", (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui8FirmwareVersion, (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui8FirmwareRevision);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"DataPackCycleTm\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui16DataPackCycleTm);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgOledDisplay\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgOledDisplay & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgDhtSensor\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgDhtSensor & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgSr501Sensor\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgSr501Sensor & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgAdcLightSensor\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgAdcLightSensor & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgAdcCarBatAin\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgAdcCarBatAin & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CfgAsyncLoraEvent\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCfgAsyncLoraEvent & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"Sr501PauseOnLoraTx\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fSr501PauseOnLoraTx & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"CommissioningMode\": %u,\n", (unsigned)(pLoraMsgData_p->m_LoraStationBootup.m_fCommissioningMode & 0x01));
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"LoraTxPower\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui8LoraTxPower);
    strJsonRecord += szJsonItem;
    snprintf(szJsonItem, sizeof(szJsonItem), "  \"LoraSpreadFactor\": %u\n", (unsigned)pLoraMsgData_p->m_LoraStationBootup.m_ui8LoraSpreadFactor);
    strJsonRecord += szJsonItem;
    strJsonRecord += "}";

    // build Json Message InfoBlock
    JsonMessage.m_uiMsgID       = pLoraMsgData_p->m_uiMsgID;
    JsonMessage.m_PacketType    = pLoraMsgData_p->m_LoraPacketType;
    JsonMessage.m_ui8DevID      = pLoraMsgData_p->m_LoraStationBootup.m_ui8DevID;
    JsonMessage.m_ui32SequNum   = 0;
    JsonMessage.m_i8Rssi        = pLoraMsgData_p->m_i8Rssi;
    JsonMessage.m_tmTimeStamp   = pLoraMsgData_p->m_tmTimeStamp;
    JsonMessage.m_strJsonRecord = strJsonRecord;
    pvecJsonMessages_p->push_back(JsonMessage);

    return (0);

}



//---------------------------------------------------------------------------
//  PprBuildJsonMessagesStationData
//---------------------------------------------------------------------------

static  int  PprBuildJsonMessagesStationData (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p)      // [IN/OUT] Ptr to Vector with Json Messages
{

tJsonMessage  JsonMessage;
std::string   strJsonRecord;
std::string   strDataHeader;
std::string   strDataRec;
char          szJsonItem[256];
uint          nDataGen;


    // clear Message Vector
    pvecJsonMessages_p->clear();

    // check validity
    if (pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_DataStatus != LoraPayloadDecoder::kStatusValid)
    {
        return (-1);
    }

    for (nDataGen=0; nDataGen<(sizeof(pLoraMsgData_p->m_LoraStationData.m_aDataRec)/sizeof(LoraPayloadDecoder::tDataRec)); nDataGen++)
    {
        // check validity
        if (pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_DataStatus != LoraPayloadDecoder::kStatusValid)
        {
            // skip invalid records (unused or invalid CRC)
            continue;
        }

        // LoRa Packet Header
        strJsonRecord = "{\n";
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"MsgID\": %u,\n", pLoraMsgData_p->m_uiMsgID);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"MsgType\": \"StationDataGen%u\",\n", nDataGen);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"TimeStamp\": %u,\n", (unsigned)pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_tmTimeStamp);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"TimeStampFmt\": \"%s\",\n", PprFormatTimeStamp(pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_tmTimeStamp).c_str());
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"RSSI\": %d,\n", (int)pLoraMsgData_p->m_i8Rssi);
        strJsonRecord += szJsonItem;

        // LoraStationData.DataHeader
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"DevID\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui8DevID);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"SequNum\": %u,\n", (unsigned)pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32SequNum);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"Uptime\": %u,\n", (unsigned)pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"UptimeFmt\": \"%s\",\n", PprFormatUptime(pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32Uptime).c_str());
        strJsonRecord += szJsonItem;

        // LoraStationData.DataRec[nIdx]
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"Temperature\": %.1f,\n", pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_flTemperature);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"Humidity\": %.1f,\n", pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_flHumidity);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"MotionActive\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_fMotionActive);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"MotionActiveTime\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_ui16MotionActiveTime);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"MotionActiveCount\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_ui16MotionActiveCount);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"LightLevel\": %u,\n", (unsigned)pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_ui8LightLevel);
        strJsonRecord += szJsonItem;
        snprintf(szJsonItem, sizeof(szJsonItem), "  \"CarBattLevel\": %.1f\n", pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_flCarBattLevel);
        strJsonRecord += szJsonItem;

        strJsonRecord += "}";

        // build Json Message InfoBlock
        JsonMessage.m_uiMsgID       = pLoraMsgData_p->m_uiMsgID;
        JsonMessage.m_PacketType    = pLoraMsgData_p->m_LoraStationData.m_aDataRec[nDataGen].m_PacketType;
        JsonMessage.m_ui8DevID      = pLoraMsgData_p->m_LoraStationData.m_DataHeader.m_ui8DevID;
        JsonMessage.m_ui32SequNum   = pLoraMsgData_p->m_aLoraStationDataReconstruct[nDataGen].m_ui32SequNum;
        JsonMessage.m_i8Rssi        = pLoraMsgData_p->m_i8Rssi;
        JsonMessage.m_tmTimeStamp   = pLoraMsgData_p->m_tmTimeStamp;
        JsonMessage.m_strJsonRecord = strJsonRecord;
        pvecJsonMessages_p->push_back(JsonMessage);
    }

    return (0);

}



//---------------------------------------------------------------------------
//  Format TimeStamp as Date/Time String
//---------------------------------------------------------------------------

static  std::string  PprFormatTimeStamp (
    time_t tmTimeStamp_p)
{

char         szTimeStamp[64];
std::string  strTimeStamp;


    PprFormatTimeStamp(tmTimeStamp_p, szTimeStamp, sizeof(szTimeStamp));
    strTimeStamp = szTimeStamp;

    return (strTimeStamp);

}

//---------------------------------------------------------------------------

static  int  PprFormatTimeStamp (
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

    iStrLen = (int)strlen(pszBuffer_p);

    return (iStrLen);

}



//---------------------------------------------------------------------------
//  Format Uptime as Date/Time String
//---------------------------------------------------------------------------

static  std::string  PprFormatUptime (
    uint32_t ui32Uptime_p,
    bool fForceDay_p /* = true */,
    bool fForceTwoDigitsHours_p /* = true */)
{

char         szUptime[64];
std::string  strUptime;


    PprFormatUptime(ui32Uptime_p, szUptime, sizeof(szUptime), fForceDay_p, fForceTwoDigitsHours_p);
    strUptime = szUptime;

    return (strUptime);

}

//---------------------------------------------------------------------------

static  int  PprFormatUptime (
    uint32_t ui32Uptime_p,
    char* pszBuffer_p,
    int iBuffSize_p,
    bool fForceDay_p /* = true */,
    bool fForceTwoDigitsHours_p /* = true */)
{

const uint32_t  SECONDS_PER_DAY    = 86400;
const uint32_t  SECONDS_PER_HOURS  = 3600;
const uint32_t  SECONDS_PER_MINUTE = 60;


uint32_t  ui32Uptime;
uint32_t  ui32Days;
uint32_t  ui32Hours;
uint32_t  ui32Minutes;
uint32_t  ui32Seconds;
int       iStrLen;


    ui32Uptime = ui32Uptime_p;

    ui32Days = ui32Uptime / SECONDS_PER_DAY;
    ui32Uptime = ui32Uptime - (ui32Days * SECONDS_PER_DAY);

    ui32Hours = ui32Uptime / SECONDS_PER_HOURS;
    ui32Uptime = ui32Uptime - (ui32Hours * SECONDS_PER_HOURS);

    ui32Minutes = ui32Uptime / SECONDS_PER_MINUTE;
    ui32Uptime = ui32Uptime - (ui32Minutes * SECONDS_PER_MINUTE);

    ui32Seconds = ui32Uptime;

    if ( (ui32Days > 0) || fForceDay_p )
    {
        snprintf(pszBuffer_p, iBuffSize_p, "%ud/%02u:%02u:%02u", (uint)ui32Days, (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
    }
    else
    {
        if ( fForceTwoDigitsHours_p )
        {
            snprintf(pszBuffer_p, iBuffSize_p, "%02u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
        else
        {
            snprintf(pszBuffer_p, iBuffSize_p, "%u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
    }

    iStrLen = (int)strlen(pszBuffer_p);

    return (iStrLen);

}




// EOF


