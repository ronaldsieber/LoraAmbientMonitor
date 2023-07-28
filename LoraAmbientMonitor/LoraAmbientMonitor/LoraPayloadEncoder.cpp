/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraPayloadEncoder> Implementation

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
#include "LoraPayloadEncoder.h"
#include <stdarg.h>





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraPayloadEncoder                                      */
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

LoraPayloadEncoder::LoraPayloadEncoder()
{

    m_ui8DevID    = 0;
    m_ui32SequNum = 0;

    memset(&m_TxLoraBootupPacket, 0x00, sizeof(m_TxLoraBootupPacket));
    memset(&m_TxLoraDataPacket, 0x00, sizeof(m_TxLoraDataPacket));

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

LoraPayloadEncoder::~LoraPayloadEncoder()
{

    return;

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P U B L I C    M E T H O D E N                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Setup
//---------------------------------------------------------------------------

void  LoraPayloadEncoder::Setup (uint8_t ui8DevID_p)
{

    m_ui8DevID = ui8DevID_p;

    return;

};



//---------------------------------------------------------------------------
//  EncodeTxBootupPacket
//---------------------------------------------------------------------------

int  LoraPayloadEncoder::EncodeTxBootupPacket (const tDeviceConfig* pDeviceConfig_p)
{

tLoraBootupHeader*  pLoraBootupHeader;


    pLoraBootupHeader = (tLoraBootupHeader*)&m_TxLoraBootupPacket.m_LoraHeader;

    // setup Header of LoRa Bootup Packet
    pLoraBootupHeader->m_ui4PacketType         = kLoraPacketBootup;
    pLoraBootupHeader->m_ui4DevID              = (m_ui8DevID & 0x0F);
    pLoraBootupHeader->m_ui8FirmwareVersion    = pDeviceConfig_p->m_ui8FirmwareVersion;
    pLoraBootupHeader->m_ui8FirmwareRevision   = pDeviceConfig_p->m_ui8FirmwareRevision;
    pLoraBootupHeader->m_ui16DataPackCycleTm   = pDeviceConfig_p->m_ui16DataPackCycleTm;
    pLoraBootupHeader->m_ui1CfgOledDisplay     = pDeviceConfig_p->m_fCfgOledDisplay;
    pLoraBootupHeader->m_ui1CfgDhtSensor       = pDeviceConfig_p->m_fCfgDhtSensor;
    pLoraBootupHeader->m_ui1CfgSr501Sensor     = pDeviceConfig_p->m_fCfgSr501Sensor;
    pLoraBootupHeader->m_ui1CfgAdcLightSensor  = pDeviceConfig_p->m_fCfgAdcLightSensor;
    pLoraBootupHeader->m_ui1CfgAdcCarBatAin    = pDeviceConfig_p->m_fCfgAdcCarBatAin;
    pLoraBootupHeader->m_ui1CfgAsyncLoraEvent  = pDeviceConfig_p->m_fCfgAsyncLoraEvent;
    pLoraBootupHeader->m_ui1Sr501PauseOnLoraTx = pDeviceConfig_p->m_fSr501PauseOnLoraTx;
    pLoraBootupHeader->m_ui1CommissioningMode  = pDeviceConfig_p->m_fCommissioningMode;
    pLoraBootupHeader->m_ui8LoraTxPower        = pDeviceConfig_p->m_ui8LoraTxPower;
    pLoraBootupHeader->m_ui8LoraSpreadFactor   = pDeviceConfig_p->m_ui8LoraSpreadFactor;
    pLoraBootupHeader->m_ui16CRC16             = CalcCrc16(&m_TxLoraBootupPacket.m_LoraHeader, (64/8));

    return (0);

}



//---------------------------------------------------------------------------
//  GetTxBootupPacket
//---------------------------------------------------------------------------

tLoraDataPacket*  LoraPayloadEncoder::GetTxBootupPacket (void)
{

    return (&m_TxLoraBootupPacket);

}



//---------------------------------------------------------------------------
//  EncodeTxDataPacket
//---------------------------------------------------------------------------

int  LoraPayloadEncoder::EncodeTxDataPacket (const tSensorDataRec* pSensorDataRec_p)
{

    // update SequenceNumber
    m_ui32SequNum++;

    // setup Header of LoRa Data Packet
    m_TxLoraDataPacket.m_LoraHeader.m_ui4PacketType = kLoraPacketDataHeader;
    m_TxLoraDataPacket.m_LoraHeader.m_ui4DevID      = (m_ui8DevID & 0x0F);
    m_TxLoraDataPacket.m_LoraHeader.m_ui24SequNum   = (m_ui32SequNum & 0x00FFFFFF);
    m_TxLoraDataPacket.m_LoraHeader.m_ui32Uptime    = pSensorDataRec_p->m_ui32Uptime;
    m_TxLoraDataPacket.m_LoraHeader.m_ui16CRC16     = CalcCrc16(&m_TxLoraDataPacket.m_LoraHeader, (64/8));

    // process generation list ([2]->[1] | [1]->[0])
    m_TxLoraDataPacket.m_aLoraDataRec[2] = m_TxLoraDataPacket.m_aLoraDataRec[1];
    if ((tLoraPacketType)(m_TxLoraDataPacket.m_aLoraDataRec[2].m_ui4PacketType) == kLoraPacketDataGen1)
    {
        m_TxLoraDataPacket.m_aLoraDataRec[2].m_ui4PacketType = kLoraPacketDataGen2;
        m_TxLoraDataPacket.m_aLoraDataRec[2].m_ui16CRC16 = CalcCrc16(&m_TxLoraDataPacket.m_aLoraDataRec[2], (64/8));
    }
    m_TxLoraDataPacket.m_aLoraDataRec[1] = m_TxLoraDataPacket.m_aLoraDataRec[0];
    if ((tLoraPacketType)(m_TxLoraDataPacket.m_aLoraDataRec[1].m_ui4PacketType) == kLoraPacketDataGen0)
    {
        m_TxLoraDataPacket.m_aLoraDataRec[1].m_ui4PacketType = kLoraPacketDataGen1;
        m_TxLoraDataPacket.m_aLoraDataRec[1].m_ui16CRC16 = CalcCrc16(&m_TxLoraDataPacket.m_aLoraDataRec[1], (64/8));
    }

    // setup newest element with current process data
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui4PacketType         = kLoraPacketDataGen0;
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui12UptimeSnippet     = ((pSensorDataRec_p->m_ui32Uptime / 10) & 0x0FFF);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_i8Temperature         = (FloatToI8(pSensorDataRec_p->m_flTemperature * 2) & 0xFF);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui7Humidity           = (FloatToUI7(pSensorDataRec_p->m_flHumidity) & 0x7F);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui1MotionActive       = (pSensorDataRec_p->m_fMotionActive ? 1 : 0);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui8MotionActiveTime   = (((pSensorDataRec_p->m_ui16MotionActiveTime + 5) / 10) & 0xFF);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui10MotionActiveCount = (pSensorDataRec_p->m_ui16MotionActiveCount & 0x03FF);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui6LightLevel         = ((pSensorDataRec_p->m_ui8LightLevel / 2) & 0x3F);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui8CarBattLevel       = (FloatToUI8(pSensorDataRec_p->m_flCarBattLevel * 10.0f) & 0xFF);
    m_TxLoraDataPacket.m_aLoraDataRec[0].m_ui16CRC16             = CalcCrc16(&m_TxLoraDataPacket.m_aLoraDataRec[0], (64/8));

    return (0);

}



//---------------------------------------------------------------------------
//  GetTxDataPacket
//---------------------------------------------------------------------------

tLoraDataPacket*  LoraPayloadEncoder::GetTxDataPacket (void)
{

    return (&m_TxLoraDataPacket);

}



//---------------------------------------------------------------------------
//  LogDeviceConfig
//---------------------------------------------------------------------------

const char*  LoraPayloadEncoder::LogDeviceConfig (const tDeviceConfig* pDeviceConfig_p)
{

char  szLogBuff[sizeof(m_szLogDeviceConfig)];


    memset(m_szLogDeviceConfig, '\0', sizeof(m_szLogDeviceConfig));
    memset(szLogBuff, '\0', sizeof(szLogBuff));

    if (pDeviceConfig_p == NULL)
    {
        return (m_szLogDeviceConfig);
    }

    LogStr(szLogBuff, sizeof(szLogBuff), " === DeviceConfig ===\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  Firmware Version:       %u.%02u\n",   (unsigned int)pDeviceConfig_p->m_ui8FirmwareVersion, (unsigned int)pDeviceConfig_p->m_ui8FirmwareRevision);
    LogStr(szLogBuff, sizeof(szLogBuff), "  DataPacketCycleTime:    %u [sec]\n",  (unsigned int)pDeviceConfig_p->m_ui16DataPackCycleTm);
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG OLED Display:       %s\n",        (pDeviceConfig_p->m_fCfgOledDisplay     ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG DHT Sensor:         %s\n",        (pDeviceConfig_p->m_fCfgDhtSensor       ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG SR501 Sensor:       %s\n",        (pDeviceConfig_p->m_fCfgSr501Sensor     ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG ADC Light Sensor:   %s\n",        (pDeviceConfig_p->m_fCfgAdcLightSensor  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG ADC Car Battery:    %s\n",        (pDeviceConfig_p->m_fCfgAdcCarBatAin    ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  CFG Async LoRa Event:   %s\n",        (pDeviceConfig_p->m_fCfgAsyncLoraEvent  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  SR501 Pause on LoRa Tx: %s\n",        (pDeviceConfig_p->m_fSr501PauseOnLoraTx ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  Commissioning Mode:     %s\n",        (pDeviceConfig_p->m_fCommissioningMode  ? "Enabled" : "Disabled"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  LoRa Tx Power:          %u [dB]\n",   (unsigned int)pDeviceConfig_p->m_ui8LoraTxPower);
    LogStr(szLogBuff, sizeof(szLogBuff), "  LoRa Spreading Factor:  %u\n",        (unsigned int)pDeviceConfig_p->m_ui8LoraSpreadFactor);

    strncpy(m_szLogDeviceConfig, szLogBuff, sizeof(m_szLogDeviceConfig));

    return (m_szLogDeviceConfig);

}



//---------------------------------------------------------------------------
//  LogSensorDataRec
//---------------------------------------------------------------------------

const char*  LoraPayloadEncoder::LogSensorDataRec (const tSensorDataRec* pSensorDataRec_p)
{

char  szLogBuff[sizeof(m_szLogSensorDataRec)];


    memset(m_szLogSensorDataRec, '\0', sizeof(m_szLogSensorDataRec));
    memset(szLogBuff, '\0', sizeof(szLogBuff));

    if (pSensorDataRec_p == NULL)
    {
        return (m_szLogSensorDataRec);
    }

    LogStr(szLogBuff, sizeof(szLogBuff), " === SensorDataRec ===\n");
    LogStr(szLogBuff, sizeof(szLogBuff), "  Uptime:                 %lu [sec]\n", (unsigned long)pSensorDataRec_p->m_ui32Uptime);
    LogStr(szLogBuff, sizeof(szLogBuff), "  Temperature:            %.1f [C]\n",  pSensorDataRec_p->m_flTemperature);
    LogStr(szLogBuff, sizeof(szLogBuff), "  Humidity:               %.1f [%s]\n", pSensorDataRec_p->m_flHumidity, "%%");
    LogStr(szLogBuff, sizeof(szLogBuff), "  MotionActive:           %s\n",        (pSensorDataRec_p->m_fMotionActive ? "True" : "False"));
    LogStr(szLogBuff, sizeof(szLogBuff), "  MotionActiveTime:       %u [sec]\n",  (unsigned int)pSensorDataRec_p->m_ui16MotionActiveTime);
    LogStr(szLogBuff, sizeof(szLogBuff), "  MotionActiveCount:      %u\n",        (unsigned int)pSensorDataRec_p->m_ui16MotionActiveCount);
    LogStr(szLogBuff, sizeof(szLogBuff), "  LightLevel:             %u [%s]\n",   (unsigned int)pSensorDataRec_p->m_ui8LightLevel, "%%");
    LogStr(szLogBuff, sizeof(szLogBuff), "  Car Battery Level:      %.1f [V]\n",  pSensorDataRec_p->m_flCarBattLevel);

    strncpy(m_szLogSensorDataRec, szLogBuff, sizeof(m_szLogSensorDataRec));

    return (m_szLogSensorDataRec);

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E    M E T H O D E N                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: FloatToI7
//---------------------------------------------------------------------------

int8_t  LoraPayloadEncoder::FloatToI7 (float flDataValue_p)
{

int     iDataValue;
int8_t  i8DataValue;


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

    // limit values to mappable range
    if (flDataValue_p > 63.0f)
    {
        flDataValue_p = 63.0f;
    }
    if (flDataValue_p < -63.0f)
    {
        flDataValue_p = -63.0f;
    }

    iDataValue = (int)round(flDataValue_p);         // float -> int
    i8DataValue = (int8_t)(iDataValue & 0x7F);      // mask Signum + B5..B0

    return (i8DataValue);

}



//---------------------------------------------------------------------------
//  Private: FloatToI8
//---------------------------------------------------------------------------

int8_t  LoraPayloadEncoder::FloatToI8 (float flDataValue_p)
{

int     iDataValue;
int8_t  i8DataValue;


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

    // limit values to mappable range
    if (flDataValue_p > 127.0f)
    {
        flDataValue_p = 127.0f;
    }
    if (flDataValue_p < -127.0f)
    {
        flDataValue_p = -127.0f;
    }

    iDataValue = (int)round(flDataValue_p);         // float -> int
    i8DataValue = (int8_t)(iDataValue & 0xFF);      // mask Signum + B6..B0

    return (i8DataValue);

}



//---------------------------------------------------------------------------
//  Private: FloatToUI7
//---------------------------------------------------------------------------

int8_t  LoraPayloadEncoder::FloatToUI7 (float flDataValue_p)
{

uint     uiDataValue;
uint8_t  ui8DataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // | - |        0 ... +127         |
    // +---+---+---+---+---+---+---+---+
    //
    //       1   1   1   1   1   1   1      +127
    //                ...
    //       0   0   0   0   0   0   1        +1
    //       0   0   0   0   0   0   0         0

    // limit values to mappable range
    if (flDataValue_p > 127.0f)
    {
        flDataValue_p = 127.0f;
    }
    if (flDataValue_p < 0.0f)
    {
        flDataValue_p = 0.0f;
    }

    uiDataValue  = (uint)round(flDataValue_p);      // float -> uint
    ui8DataValue = (uint8_t)(uiDataValue & 0x7F);   // mask B6..B0

    return (ui8DataValue);

}



//---------------------------------------------------------------------------
//  Private: FloatToUI8
//---------------------------------------------------------------------------

int8_t  LoraPayloadEncoder::FloatToUI8 (float flDataValue_p)
{

uint    uiDataValue;
int8_t  ui8DataValue;


    //   B7  B6  B5  B4  B3  B2  B1  B0
    // +---+---+---+---+---+---+---+---+
    // |          0 ... +255           |
    // +---+---+---+---+---+---+---+---+
    //
    //   1   1   1   1   1   1   1   1      +255
    //                ...
    //   0   0   0   0   0   0   0   1        +1
    //   0   0   0   0   0   0   0   0         0

    // limit values to mappable range
    if (flDataValue_p > 255.0f)
    {
        flDataValue_p = 255.0f;
    }
    if (flDataValue_p < 0.0f)
    {
        flDataValue_p = 0.0f;
    }

    uiDataValue  = (uint)round(flDataValue_p);      // float -> uint
    ui8DataValue = (uint8_t)(uiDataValue & 0xFF);   // mask B7..B0

    return (ui8DataValue);

}



//---------------------------------------------------------------------------
//  Private: CalcCrc16
//---------------------------------------------------------------------------

uint16_t  LoraPayloadEncoder::CalcCrc16 (const void* pDataBlock_p, unsigned int uiDataBlockSize_p)
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

size_t  LoraPayloadEncoder::LogStr (char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...)
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



//  EOF


