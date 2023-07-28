/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraPayloadDecoder> Implementation

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#if defined(ARDUINO_ARCH_ESP32)
    #include "Arduino.h"
#else
    #define _CRT_SECURE_NO_WARNINGS
    #include <iostream>
    typedef  unsigned int  uint;
#endif

#include "LoraPacket.h"
#include "LoraPayloadDecoder.h"
#include <string.h>
#include <stdarg.h>





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraPayloadDecoder                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E   A T T R I B U T E S                            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          C O N S T R U C T O R   /   D E S T R U C T O R                //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------

LoraPayloadDecoder::LoraPayloadDecoder()
{

    // clear packet buffers
    memset(&m_LoraStationBootup, 0x00, sizeof(m_LoraStationBootup));
    memset(&m_LoraStationData, 0x00, sizeof(m_LoraStationData));

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

LoraPayloadDecoder::~LoraPayloadDecoder()
{

    return;

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P U B L I C    M E T H O D E N                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  GetRxPacketType
//---------------------------------------------------------------------------

tLoraPacketType  LoraPayloadDecoder::GetRxPacketType (const tLoraDataPacket* pLoraPacket_p)
{

tLoraPacketType  PacketType;


    if (pLoraPacket_p == NULL)
    {
        return (kLoraPacketUnused);
    }

    PacketType = (tLoraPacketType)(pLoraPacket_p->m_LoraHeader.m_ui4PacketType & 0x0F);

    return (PacketType);

}



//---------------------------------------------------------------------------
//  GetRxPacketDevID
//---------------------------------------------------------------------------

int  LoraPayloadDecoder::GetRxPacketDevID (const tLoraDataPacket* pLoraPacket_p)
{

int  iLoraDevID;


    if (pLoraPacket_p == NULL)
    {
        return (-1);
    }

    iLoraDevID = (int)(pLoraPacket_p->m_LoraHeader.m_ui4DevID & 0x0F);

    return (iLoraDevID);

}



//---------------------------------------------------------------------------
//  DecodeRxBootupPacket
//---------------------------------------------------------------------------

LoraPayloadDecoder::tLoraStationBootup  LoraPayloadDecoder::DecodeRxBootupPacket (const tLoraDataPacket* pLoraPacket_p)
{

tLoraBootupHeader*  pLoraBootupHeader;
uint16_t            ui16CrcSum;


    // clear data packet
    memset(&m_LoraStationBootup, 0x00, sizeof(m_LoraStationBootup));

    if (pLoraPacket_p == NULL)
    {
        return (m_LoraStationBootup);
    }

    // interpret LoRa packet as Bootup Data
    pLoraBootupHeader = (tLoraBootupHeader*)pLoraPacket_p;

    // decode Bootup
    ui16CrcSum = CalcCrc16(pLoraBootupHeader, (64/8));
    if (ui16CrcSum == pLoraBootupHeader->m_ui16CRC16)
    {
        m_LoraStationBootup.m_DataStatus = kStatusValid;
    }
    else
    {
        m_LoraStationBootup.m_DataStatus = kStatusCrcError;
    }
    m_LoraStationBootup.m_PacketType          = (tLoraPacketType)(pLoraBootupHeader->m_ui4PacketType & 0x0F);
    m_LoraStationBootup.m_ui8DevID            = (pLoraBootupHeader->m_ui4DevID & 0x0F);
    m_LoraStationBootup.m_ui8FirmwareVersion  = pLoraBootupHeader->m_ui8FirmwareVersion;
    m_LoraStationBootup.m_ui8FirmwareRevision = pLoraBootupHeader->m_ui8FirmwareRevision;
    m_LoraStationBootup.m_ui16DataPackCycleTm = pLoraBootupHeader->m_ui16DataPackCycleTm;
    m_LoraStationBootup.m_fCfgOledDisplay     = (bool)pLoraBootupHeader->m_ui1CfgOledDisplay;
    m_LoraStationBootup.m_fCfgDhtSensor       = (bool)pLoraBootupHeader->m_ui1CfgDhtSensor;
    m_LoraStationBootup.m_fCfgSr501Sensor     = (bool)pLoraBootupHeader->m_ui1CfgSr501Sensor;
    m_LoraStationBootup.m_fCfgAdcLightSensor  = (bool)pLoraBootupHeader->m_ui1CfgAdcLightSensor;
    m_LoraStationBootup.m_fCfgAdcCarBatAin    = (bool)pLoraBootupHeader->m_ui1CfgAdcCarBatAin;
    m_LoraStationBootup.m_fCfgAsyncLoraEvent  = (bool)pLoraBootupHeader->m_ui1CfgAsyncLoraEvent;
    m_LoraStationBootup.m_fSr501PauseOnLoraTx = (bool)pLoraBootupHeader->m_ui1Sr501PauseOnLoraTx;
    m_LoraStationBootup.m_fCommissioningMode  = (bool)pLoraBootupHeader->m_ui1CommissioningMode;
    m_LoraStationBootup.m_ui8LoraTxPower      = pLoraBootupHeader->m_ui8LoraTxPower;
    m_LoraStationBootup.m_ui8LoraSpreadFactor = pLoraBootupHeader->m_ui8LoraSpreadFactor;

    return (m_LoraStationBootup);

}



//---------------------------------------------------------------------------
//  DecodeRxDataPacket
//---------------------------------------------------------------------------

LoraPayloadDecoder::tLoraStationData  LoraPayloadDecoder::DecodeRxDataPacket (const tLoraDataPacket* pLoraPacket_p)
{

uint16_t  ui16CrcSum;
int       nIdx;


    // clear data packet
    memset(&m_LoraStationData, 0x00, sizeof(m_LoraStationData));

    if (pLoraPacket_p == NULL)
    {
        return (m_LoraStationData);
    }

    // decode Header
    ui16CrcSum = CalcCrc16(&(pLoraPacket_p->m_LoraHeader), (64/8));
    if (ui16CrcSum == pLoraPacket_p->m_LoraHeader.m_ui16CRC16)
    {
        m_LoraStationData.m_DataHeader.m_DataStatus = kStatusValid;
    }
    else
    {
        m_LoraStationData.m_DataHeader.m_DataStatus = kStatusCrcError;
    }
    m_LoraStationData.m_DataHeader.m_PacketType  = (tLoraPacketType)(pLoraPacket_p->m_LoraHeader.m_ui4PacketType & 0x0F);
    m_LoraStationData.m_DataHeader.m_ui8DevID    = (pLoraPacket_p->m_LoraHeader.m_ui4DevID & 0x0F);
    m_LoraStationData.m_DataHeader.m_ui32SequNum = (pLoraPacket_p->m_LoraHeader.m_ui24SequNum & 0x00FFFFFF);
    m_LoraStationData.m_DataHeader.m_ui32Uptime  = (pLoraPacket_p->m_LoraHeader.m_ui32Uptime & 0xFFFFFFFF);

    // decode DataRecords
    for (nIdx=0; nIdx<(sizeof(m_LoraStationData.m_aDataRec)/sizeof(tDataRec)); nIdx++)
    {
        if ( IsCleared(&(pLoraPacket_p->m_aLoraDataRec[nIdx]), (64/8)) )
        {
            m_LoraStationData.m_aDataRec[nIdx].m_DataStatus = kStatusUnused;
        }
        else
        {
            ui16CrcSum = CalcCrc16(&(pLoraPacket_p->m_aLoraDataRec[nIdx]), (64/8));
            if (ui16CrcSum == pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui16CRC16)
            {
                m_LoraStationData.m_aDataRec[nIdx].m_DataStatus = kStatusValid;
            }
            else
            {
                m_LoraStationData.m_aDataRec[nIdx].m_DataStatus = kStatusCrcError;
            }
            m_LoraStationData.m_aDataRec[nIdx].m_PacketType            = (tLoraPacketType)(pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui4PacketType & 0x0F);
            m_LoraStationData.m_aDataRec[nIdx].m_ui12UptimeSnippet     = (pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui12UptimeSnippet & 0x0FFF) * 10;
            m_LoraStationData.m_aDataRec[nIdx].m_flTemperature         = I8ToFloat(pLoraPacket_p->m_aLoraDataRec[nIdx].m_i8Temperature & 0xFF) / 2;
            m_LoraStationData.m_aDataRec[nIdx].m_flHumidity            = UI7ToFloat(pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui7Humidity & 0x7F);
            m_LoraStationData.m_aDataRec[nIdx].m_fMotionActive         = (pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui1MotionActive ? true : false);
            m_LoraStationData.m_aDataRec[nIdx].m_ui16MotionActiveTime  = (pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui8MotionActiveTime & 0xFF) * 10;
            m_LoraStationData.m_aDataRec[nIdx].m_ui16MotionActiveCount = (pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui10MotionActiveCount & 0x03FF);
            m_LoraStationData.m_aDataRec[nIdx].m_ui8LightLevel         = (pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui6LightLevel & 0x3F) * 2;
            m_LoraStationData.m_aDataRec[nIdx].m_flCarBattLevel        = UI8ToFloat(pLoraPacket_p->m_aLoraDataRec[nIdx].m_ui8CarBattLevel & 0xFF) / 10.0f;
        }
    }

    return (m_LoraStationData);

}



//---------------------------------------------------------------------------
//  LogStationBootup
//---------------------------------------------------------------------------

const char*  LoraPayloadDecoder::LogStationBootup (const tLoraStationBootup* pLoraStationBootup_p)
{

char  szLogBuff[sizeof(m_szLogStationBootup)];


    memset(m_szLogStationBootup, '\0', sizeof(m_szLogStationBootup));
    memset(szLogBuff, '\0', sizeof(szLogBuff));

    if (pLoraStationBootup_p == NULL)
    {
        return (m_szLogStationBootup);
    }

    LogStr(szLogBuff, sizeof(szLogBuff), " === StationBootup ===\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  DataStatus:             ");
    switch (pLoraStationBootup_p->m_DataStatus)
    {
        case kStatusUnused:             LogStr(szLogBuff, sizeof(szLogBuff), "kStatusUnused");              break;
        case kStatusCrcError:           LogStr(szLogBuff, sizeof(szLogBuff), "kStatusCrcError");            break;
        case kStatusValid:              LogStr(szLogBuff, sizeof(szLogBuff), "kStatusValid");               break;
        default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                        break;
    }
    LogStr(szLogBuff, sizeof(szLogBuff), "\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  PacketType:             ");
    switch (pLoraStationBootup_p->m_PacketType)
    {
        case kLoraPacketUnused:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketUnused");          break;
        case kLoraPacketBootup:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketBootup");          break;
        case kLoraPacketDataHeader:     LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataHeader");      break;
        case kLoraPacketDataGen0:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen0");        break;
        case kLoraPacketDataGen1:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen1");        break;
        case kLoraPacketDataGen2:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen2");        break;
        default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                        break;
    }
    LogStr(szLogBuff, sizeof(szLogBuff), "\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  DevID:                  %u\n",       (unsigned int)pLoraStationBootup_p->m_ui8DevID);
    LogStr(szLogBuff, sizeof(szLogBuff), "  Firmware Version:       %u.%02u\n",  (unsigned int)pLoraStationBootup_p->m_ui8FirmwareVersion, (uint)pLoraStationBootup_p->m_ui8FirmwareRevision);
    LogStr(szLogBuff, sizeof(szLogBuff), "  DataPacketCycleTime:    %u [sec]\n", (unsigned int)pLoraStationBootup_p->m_ui16DataPackCycleTm);
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG OLED Display:       %s\n",       (pLoraStationBootup_p->m_fCfgOledDisplay     ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG DHT Sensor:         %s\n",       (pLoraStationBootup_p->m_fCfgDhtSensor       ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG SR501 Sensor:       %s\n",       (pLoraStationBootup_p->m_fCfgSr501Sensor     ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG ADC Light Sensor:   %s\n",       (pLoraStationBootup_p->m_fCfgAdcLightSensor  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG ADC Car Battery:    %s\n",       (pLoraStationBootup_p->m_fCfgAdcCarBatAin    ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG Async LoRa Event:   %s\n",       (pLoraStationBootup_p->m_fCfgAsyncLoraEvent  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  SR501 Pause on LoRa Tx: %s\n",       (pLoraStationBootup_p->m_fSr501PauseOnLoraTx ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  Commissioning Mode:     %s\n",       (pLoraStationBootup_p->m_fCommissioningMode  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  LoRa Tx Power:          %u [dB]\n",  (unsigned int)pLoraStationBootup_p->m_ui8LoraTxPower);
    LogStr(szLogBuff, sizeof(szLogBuff), "  LoRa Spreading Factor:  %u\n",       (unsigned int)pLoraStationBootup_p->m_ui8LoraSpreadFactor);

    strncpy(m_szLogStationBootup, szLogBuff, sizeof(m_szLogStationBootup));

    return (m_szLogStationBootup);

}



//---------------------------------------------------------------------------
//  LogStationData
//---------------------------------------------------------------------------

const char*  LoraPayloadDecoder::LogStationData (const tLoraStationData* pLoraStationData_p)
{

char    szLogBuff[sizeof(m_szLogStationData)];
size_t  nUsedBuffLen;
int     nIdx;


    memset(m_szLogStationData, '\0', sizeof(m_szLogStationData));
    memset(szLogBuff, '\0', sizeof(szLogBuff));

    if (pLoraStationData_p == NULL)
    {
        return (m_szLogStationData);
    }

    LogStr(szLogBuff, sizeof(szLogBuff), " === StationData ===\n");

    // decode Header
    LogStr(szLogBuff, sizeof(szLogBuff), " *Header*\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  DataStatus:             ");
    switch (pLoraStationData_p->m_DataHeader.m_DataStatus)
    {
        case kStatusUnused:             LogStr(szLogBuff, sizeof(szLogBuff), "kStatusUnused");              break;
        case kStatusCrcError:           LogStr(szLogBuff, sizeof(szLogBuff), "kStatusCrcError");            break;
        case kStatusValid:              LogStr(szLogBuff, sizeof(szLogBuff), "kStatusValid");               break;
        default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                        break;
    }
    LogStr(szLogBuff, sizeof(szLogBuff), "\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  PacketType:             ");
    switch (pLoraStationData_p->m_DataHeader.m_PacketType)
    {
        case kLoraPacketUnused:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketUnused");          break;
        case kLoraPacketBootup:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketBootup");          break;
        case kLoraPacketDataHeader:     LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataHeader");      break;
        case kLoraPacketDataGen0:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen0");        break;
        case kLoraPacketDataGen1:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen1");        break;
        case kLoraPacketDataGen2:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen2");        break;
        default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                        break;
    }
    LogStr(szLogBuff, sizeof(szLogBuff), "\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  DevID:                  %u\n",          (unsigned int)pLoraStationData_p->m_DataHeader.m_ui8DevID);
    LogStr(szLogBuff, sizeof(szLogBuff), "  Sequence Number:        %lu\n",         (unsigned long)pLoraStationData_p->m_DataHeader.m_ui32SequNum);
    LogStr(szLogBuff, sizeof(szLogBuff), "  Uptime:                 %lu [sec] -> ", (unsigned long)pLoraStationData_p->m_DataHeader.m_ui32Uptime);
    nUsedBuffLen = strlen(szLogBuff);
    FormatUptime(pLoraStationData_p->m_DataHeader.m_ui32Uptime, (szLogBuff + nUsedBuffLen), (sizeof(szLogBuff) - nUsedBuffLen), true, true);
    LogStr(szLogBuff, sizeof(szLogBuff), "\n");

    // decode DataRecords
    LogStr(szLogBuff, sizeof(szLogBuff), " *DataRecords*\n");
    for (nIdx=0; nIdx<(sizeof(pLoraStationData_p->m_aDataRec)/sizeof(tDataRec)); nIdx++)
    {
        LogStr(szLogBuff, sizeof(szLogBuff), "  DataRecord[%d]:\n",                   nIdx);
        LogStr(szLogBuff, sizeof(szLogBuff), "   DataStatus:            ");
        switch (pLoraStationData_p->m_aDataRec[nIdx].m_DataStatus)
        {
            case kStatusUnused:             LogStr(szLogBuff, sizeof(szLogBuff), "kStatusUnused");          break;
            case kStatusCrcError:           LogStr(szLogBuff, sizeof(szLogBuff), "kStatusCrcError");        break;
            case kStatusValid:              LogStr(szLogBuff, sizeof(szLogBuff), "kStatusValid");           break;
            default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                    break;
        }
        LogStr(szLogBuff, sizeof(szLogBuff), "\n");
        LogStr(szLogBuff, sizeof(szLogBuff), "   PacketType:            ");
        switch (pLoraStationData_p->m_aDataRec[nIdx].m_PacketType)
        {
            case kLoraPacketUnused:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketUnused");      break;
            case kLoraPacketBootup:         LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketBootup");      break;
            case kLoraPacketDataHeader:     LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataHeader");  break;
            case kLoraPacketDataGen0:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen0");    break;
            case kLoraPacketDataGen1:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen1");    break;
            case kLoraPacketDataGen2:       LogStr(szLogBuff, sizeof(szLogBuff), "kLoraPacketDataGen2");    break;
            default:                        LogStr(szLogBuff, sizeof(szLogBuff), "???");                    break;
        }
        LogStr(szLogBuff, sizeof(szLogBuff), "\n");
        LogStr(szLogBuff, sizeof(szLogBuff), "   Uptime Snippet:        %u\n",        (unsigned int)pLoraStationData_p->m_aDataRec[nIdx].m_ui12UptimeSnippet);
        LogStr(szLogBuff, sizeof(szLogBuff), "   Temperature:           %.1f [C]\n",  pLoraStationData_p->m_aDataRec[nIdx].m_flTemperature);
        LogStr(szLogBuff, sizeof(szLogBuff), "   Humidity:              %.1f [%%]\n", pLoraStationData_p->m_aDataRec[nIdx].m_flHumidity);
        LogStr(szLogBuff, sizeof(szLogBuff), "   MotionActive:          %s\n",        (pLoraStationData_p->m_aDataRec[nIdx].m_fMotionActive ? "True" : "False"));
        LogStr(szLogBuff, sizeof(szLogBuff), "   MotionActiveTime:      %u [sec]\n",  (unsigned int)pLoraStationData_p->m_aDataRec[nIdx].m_ui16MotionActiveTime);
        LogStr(szLogBuff, sizeof(szLogBuff), "   MotionActiveCount:     %u\n",        (unsigned int)pLoraStationData_p->m_aDataRec[nIdx].m_ui16MotionActiveCount);
        LogStr(szLogBuff, sizeof(szLogBuff), "   LightLevel:            %u [%%]\n",   (unsigned int)pLoraStationData_p->m_aDataRec[nIdx].m_ui8LightLevel);
        LogStr(szLogBuff, sizeof(szLogBuff), "   Car Battery Level:     %.1f [V]\n",  pLoraStationData_p->m_aDataRec[nIdx].m_flCarBattLevel);
    }

    strncpy(m_szLogStationData, szLogBuff, sizeof(m_szLogStationData));

    return (m_szLogStationData);

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E    M E T H O D E N                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: I7ToFloat
//---------------------------------------------------------------------------

float  LoraPayloadDecoder::I7ToFloat (int8_t i8DataValue_p)
{

int    iDataValue;
float  flDataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // | - | S |     -63 ... +63       |
    // +---+---+---+---+---+---+---+---+
    //
    //       0   1   1   1   1   1   1      +63
    //                ...
    //       0   0   0   0   0   0   1       +1
    //       0   0   0   0   0   0   0        0
    //       1   1   1   1   1   1   1       -1
    //                ...
    //       1   0   0   0   0   0   1      -63

    iDataValue = (int)(i8DataValue_p & 0x7F);       // mask B5..B0
    if ( (iDataValue & 0x40) )                      // extend negative values
    {
        iDataValue |= 0xFFFFFF80;
    }
    flDataValue = (float)iDataValue;                // int -> float

    return (flDataValue);

}



//---------------------------------------------------------------------------
//  Private: I8ToFloat
//---------------------------------------------------------------------------

float  LoraPayloadDecoder::I8ToFloat (int8_t i8DataValue_p)
{

int    iDataValue;
float  flDataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // | S |       -127 ... +127       |
    // +---+---+---+---+---+---+---+---+
    //
    //   0   1   1   1   1   1   1   1      +127
    //                ...
    //   0   0   0   0   0   0   0   1        +1
    //   0   0   0   0   0   0   0   0         0
    //   1   1   1   1   1   1   1   1        -1
    //                ...
    //   1   0   0   0   0   0   0   1      -127

    iDataValue = (int)(i8DataValue_p & 0xFF);       // mask B6..B0
    if ( (iDataValue & 0x80) )                      // extend negative values
    {
        iDataValue |= 0xFFFFFF00;
    }
    flDataValue = (float)iDataValue;                // int -> float

    return (flDataValue);

}



//---------------------------------------------------------------------------
//  Private: UI7ToFloat
//---------------------------------------------------------------------------

float  LoraPayloadDecoder::UI7ToFloat (uint8_t ui8DataValue_p)
{

uint   uiDataValue;
float  flDataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // | - |        0 ... +127         |
    // +---+---+---+---+---+---+---+---+
    //
    //       1   1   1   1   1   1   1      +127
    //                ...
    //       0   0   0   0   0   0   1        +1
    //       0   0   0   0   0   0   0         0

    uiDataValue = (uint)(ui8DataValue_p & 0x7F);    // mask B6..B0
    flDataValue = (float)(uiDataValue);             // int -> float

    return (flDataValue);

}



//---------------------------------------------------------------------------
//  Private: UI8ToFloat
//---------------------------------------------------------------------------

float  LoraPayloadDecoder::UI8ToFloat (uint8_t ui8DataValue_p)
{

uint   uiDataValue;
float  flDataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // |          0 ... +255           |
    // +---+---+---+---+---+---+---+---+
    //
    //   1   1   1   1   1   1   1   1      +255
    //                ...
    //   0   0   0   0   0   0   0   1        +1
    //   0   0   0   0   0   0   0   0         0

    uiDataValue = (uint)(ui8DataValue_p & 0xFF);    // mask B7..B0
    flDataValue = (float)(uiDataValue);             // int -> float

    return (flDataValue);

}



//---------------------------------------------------------------------------
//  Private: IsCleared
//---------------------------------------------------------------------------

bool  LoraPayloadDecoder::IsCleared (const void* pDataBlock_p, unsigned int uiDataBlockSize_p)
{

uint8_t*  pabDataBlock;


    pabDataBlock = (uint8_t*)pDataBlock_p;

    while ( uiDataBlockSize_p-- )
    {
        if (*pabDataBlock != 0x00)
        {
            return (false);
        }
        pabDataBlock++;
    }

    return (true);

}



//---------------------------------------------------------------------------
//  Private: CalcCrc16
//---------------------------------------------------------------------------

uint16_t  LoraPayloadDecoder::CalcCrc16 (const void* pDataBlock_p, unsigned int uiDataBlockSize_p)
{

// CCITT-Polynom (DIN 66 219): X^16 + X^12 + X^5 + 1 (=0x1021)
#define CCITT_CRC_POLYNOM   0x1021

uint8_t*  pabDataBlock;
uint      uiBitCnt;
bool      fOverflow;
uint16_t  ui16CrcSum;


    pabDataBlock = (uint8_t*)pDataBlock_p;
    ui16CrcSum = 0;

    while ( uiDataBlockSize_p-- )
    {
        uiBitCnt = 8;
        while ( uiBitCnt-- )
        {
            fOverflow = (bool)((ui16CrcSum & 0x8000) != 0);
            ui16CrcSum <<= 1;
            if ( fOverflow )
            {
                ui16CrcSum ^= CCITT_CRC_POLYNOM;
            }
        }
        ui16CrcSum ^= *pabDataBlock++;
    }


    return (ui16CrcSum);

}



//---------------------------------------------------------------------------
//  Private: LogStr
//---------------------------------------------------------------------------

size_t  LoraPayloadDecoder::LogStr (char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...)
{

va_list  pArgList;
size_t   nUsedBuffLen;
size_t   nFreeBuffLen;


    if ((pszLogBuff_p == NULL) || (nLogBuffSize_p == 0) || (pszFmt_p == NULL))
    {
        return (0);
    }

    nUsedBuffLen = strlen(pszLogBuff_p);
    nFreeBuffLen = nLogBuffSize_p - nUsedBuffLen;
    if (nFreeBuffLen <= 0)
    {
        return (0);
    }

    va_start(pArgList, pszFmt_p);
    vsnprintf(&pszLogBuff_p[nUsedBuffLen], nFreeBuffLen, pszFmt_p, pArgList);
    va_end(pArgList);

    return (strlen(pszLogBuff_p) - nUsedBuffLen);

}



//---------------------------------------------------------------------------
//  Private: FormatUptime
//---------------------------------------------------------------------------

size_t  LoraPayloadDecoder::FormatUptime (uint32_t ui32Uptime_p, char* pszLogBuff_p, size_t nLogBuffSize_p, bool fForceDay_p /* = true */, bool fForceTwoDigitsHours_p /* = true */)
{

const uint32_t  SECONDS_PER_DAY    = 86400;
const uint32_t  SECONDS_PER_HOURS  = 3600;
const uint32_t  SECONDS_PER_MINUTE = 60;


uint32_t  ui32Uptime;
uint32_t  ui32Days;
uint32_t  ui32Hours;
uint32_t  ui32Minutes;
uint32_t  ui32Seconds;
size_t    nStrLen;


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
        snprintf(pszLogBuff_p, nLogBuffSize_p, "%ud/%02u:%02u:%02u", (uint)ui32Days, (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
    }
    else
    {
        if ( fForceTwoDigitsHours_p )
        {
            snprintf(pszLogBuff_p, nLogBuffSize_p, "%02u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
        else
        {
            snprintf(pszLogBuff_p, nLogBuffSize_p, "%u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
    }

    nStrLen = strlen(pszLogBuff_p);

    return (nStrLen);

}



//  EOF
