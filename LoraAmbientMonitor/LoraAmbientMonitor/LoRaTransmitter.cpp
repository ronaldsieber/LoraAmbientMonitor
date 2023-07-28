/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraTransmitter> Implementation

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#include "Arduino.h"
#include <SPI.h>
#include <LoRa.h>
#include "LoraTransmitter.h"





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraTransmitter                                         */
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

LoraTransmitter::LoraTransmitter()
{

    m_ui32TransmitInhibitTime       = 0;
    m_ui32TransmitCycleTime         = 0;
    m_ui32SysTickLastTransmitPacket = 0;

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

LoraTransmitter::~LoraTransmitter()
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

int  LoraTransmitter::Setup (const tLoraTransmitterSettings* pSettings_p, unsigned long uiRandomSeed_p)
{

int  iRes;


    // setup SPI channel
    SPI.begin(pSettings_p->m_iPinLora_SCK, pSettings_p->m_iPinLora_MISO, pSettings_p->m_iPinLora_MOSI, pSettings_p->m_iPinLora_CS);

    // setup LoRa Radio Transmitter
    LoRa.setPins(pSettings_p->m_iPinLora_CS, pSettings_p->m_iPinLora_RST, pSettings_p->m_iPinLora_DIO0);
    LoRa.setTxPower(pSettings_p->m_iLoraTxPower, PA_OUTPUT_PA_BOOST_PIN);   // 20dB output must via PABOOST
    LoRa.setSpreadingFactor(pSettings_p->m_iLoraSpreadingFactor);
    LoRa.setSignalBandwidth(pSettings_p->m_lLoraSignalBandwidth);
    LoRa.setCodingRate4(pSettings_p->m_iLoraCodingRateDenominator);
    iRes = LoRa.begin(868E6);
    if (iRes != 1)
    {
        return (-1);
    }

    // init random generator, used by <CalcNextTransmitCycleTime>
    randomSeed(uiRandomSeed_p);
    
    return (0);

};



//---------------------------------------------------------------------------
//  CalcNextTransmitCycleTime
//---------------------------------------------------------------------------

uint32_t  LoraTransmitter::CalcNextTransmitCycleTime (uint32_t ui32LoraPacketInhibitTime_p, uint32_t ui32LoraPacketCycleTime_p)
{

float  flCycleTimeLowerBound;
float  flCycleTimeUpperBound;
float  flCycleTimeRange;
float  flCycleTimeShift;
float  flRandomValue;


    m_ui32TransmitInhibitTime = ui32LoraPacketInhibitTime_p;

    flCycleTimeLowerBound = round((float)ui32LoraPacketCycleTime_p * 0.95F);            // LowerBound =  95%
    flCycleTimeUpperBound = round((float)ui32LoraPacketCycleTime_p * 1.05F);            // UpperBound = 105%

    flCycleTimeRange = flCycleTimeUpperBound - flCycleTimeLowerBound;                   // Range in [sec]

    flRandomValue = (float)random(32768);                                               // use random generator from Arduino runtime
    flRandomValue /= 32768;                                                             // convert to range 0...1

    flCycleTimeShift = flCycleTimeRange * flRandomValue;                                // ShiftTime = [0...Range[sec])
    m_ui32TransmitCycleTime = (uint32_t)(flCycleTimeLowerBound + flCycleTimeShift);

    if (m_ui32TransmitCycleTime < m_ui32TransmitInhibitTime)
    {
        m_ui32TransmitCycleTime = m_ui32TransmitInhibitTime;
    }

    return (m_ui32TransmitCycleTime);

}



//---------------------------------------------------------------------------
//  GetReasonToTransmitPacket
//---------------------------------------------------------------------------

int  LoraTransmitter::GetReasonToTransmitPacket (bool* pfAsyncTransmitEvent_p)
{

uint32_t  ui32CurrTick;


    ui32CurrTick = millis();

    if ((ui32CurrTick - m_ui32SysTickLastTransmitPacket) < m_ui32TransmitInhibitTime)
    {
        // suppress transmission within inhibit time period
        return (-1);
    }

    if ( *pfAsyncTransmitEvent_p )
    {
        // trigger asynchronous transmission before expiration of regular cycle time
        *pfAsyncTransmitEvent_p = false;     // reset asynchronous event
        return (1);
    }
    
    if ((ui32CurrTick - m_ui32SysTickLastTransmitPacket) >= m_ui32TransmitCycleTime)
    {
        // trigger transmission after expiration of regular cycle time
        return (2);
    }

    // continue waiting
    return (0);

}



//---------------------------------------------------------------------------
//  GetRemainingTransmitCycleTime
//---------------------------------------------------------------------------

int32_t  LoraTransmitter::GetRemainingTransmitCycleTime ()
{

uint32_t  ui32CurrTick;
int32_t   i32RemainingTime;


    ui32CurrTick = millis();
    i32RemainingTime = (int)m_ui32TransmitCycleTime - (int)(ui32CurrTick - m_ui32SysTickLastTransmitPacket);

    return (i32RemainingTime);

}



//---------------------------------------------------------------------------
//  TransmitPacket
//---------------------------------------------------------------------------

int  LoraTransmitter::TransmitPacket (const void* pTxPacket_p, uint8_t ui8TxPacketLen_p, bool fLogDataToConsole_p /* = false */)
{

int  iRes;


    iRes = LoRa.beginPacket();
    if (iRes != 1)
    {
        return (-1);
    }
    
    if ( fLogDataToConsole_p )
    {
        DumpBuffer(pTxPacket_p, (unsigned int)ui8TxPacketLen_p);
    }

    iRes = LoRa.write((const uint8_t*)pTxPacket_p, (size_t)ui8TxPacketLen_p);
    if (iRes != (int)ui8TxPacketLen_p)
    {
        return (-2);
    }

    iRes = LoRa.endPacket();
    if (iRes != 1)
    {
        return (-3);
    }

    m_ui32SysTickLastTransmitPacket = millis();

    return (0);

}



//---------------------------------------------------------------------------
//  GetTransmitIndicatorState
//---------------------------------------------------------------------------

bool  LoraTransmitter::GetTransmitIndicatorState (uint32_t ui32SignalActiveTime_p)
{

uint32_t  ui32CurrTick;
bool      fIndicatorState;


    ui32CurrTick = millis();
    if ((ui32CurrTick - m_ui32SysTickLastTransmitPacket) < ui32SignalActiveTime_p)
    {
        fIndicatorState = true;
    }
    else
    {
        fIndicatorState = false;
    }

    return (fIndicatorState);

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//          P R I V A T E    M E T H O D E N                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Private: DumpBuffer
//---------------------------------------------------------------------------

void  LoraTransmitter::DumpBuffer (const void* pabDataBuff_p, unsigned int uiDataBuffLen_p)
{

#define COLUMNS_PER_LINE    16

const unsigned char*  pabBuffData;
unsigned int          uiBuffSize;
char                  szLineBuff[128];
unsigned char         bData;
int                   nRow;
int                   nCol;


    // get pointer to buffer and length of buffer
    pabBuffData = (const unsigned char*)pabDataBuff_p;
    uiBuffSize  = (unsigned int)uiDataBuffLen_p;


    // dump buffer contents
    for (nRow=0; ; nRow++)
    {
        sprintf(szLineBuff, "\n%04lX:   ", (unsigned long)(nRow*COLUMNS_PER_LINE));
        Serial.print(szLineBuff);

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            if ((unsigned int)nCol < uiBuffSize)
            {
                sprintf(szLineBuff, "%02X ", (unsigned int)*(pabBuffData+nCol));
                Serial.print(szLineBuff);
            }
            else
            {
                Serial.print("   ");
            }
        }

        Serial.print(" ");

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            bData = *pabBuffData++;
            if ((unsigned int)nCol < uiBuffSize)
            {
                if ((bData >= 0x20) && (bData < 0x7F))
                {
                    sprintf(szLineBuff, "%c", bData);
                    Serial.print(szLineBuff);
                }
                else
                {
                    Serial.print(".");
                }
            }
            else
            {
                Serial.print(" ");
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

        delay(50);          // give serial interface time to flush data
    }

    Serial.print("\n");

    return;

}



//  EOF
