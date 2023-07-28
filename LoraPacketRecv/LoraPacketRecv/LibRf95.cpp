/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of RF95 Function Library

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <math.h>
#include <RH_RF95.h>
#include "LibRf95.h"
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

static  RH_RF95*    m_pRF95         = NULL;             // runtime instance of RF95 driver
static  uint8_t     m_ui8GpioPinCS  = (uint8_t)-1;
static  uint8_t     m_ui8GpioPinIRQ = (uint8_t)-1;
static  uint8_t     m_ui8GpioPinRST = (uint8_t)-1;



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  RF95Setup
//---------------------------------------------------------------------------

void  RF95Setup (uint8_t ui8GpioPinCS_p, uint8_t ui8GpioPinIRQ_p, uint8_t ui8GpioPinRST_p)
{

    // save GPIO pin configuration
    m_ui8GpioPinCS  = ui8GpioPinCS_p;
    m_ui8GpioPinIRQ = ui8GpioPinIRQ_p;
    m_ui8GpioPinRST = ui8GpioPinRST_p;

    // create runtime instance of RF95 driver
    m_pRF95 = new RH_RF95(m_ui8GpioPinCS, m_ui8GpioPinIRQ);

    return;

}



//---------------------------------------------------------------------------
//  RF95ResetModule
//---------------------------------------------------------------------------

int  RF95ResetModule ()
{

    if (m_ui8GpioPinRST == (uint8_t)-1)
    {
        return (-1);
    }

    // Pulse a reset on module
    pinMode(m_ui8GpioPinRST, OUTPUT);
    digitalWrite(m_ui8GpioPinRST, LOW);
    bcm2835_delay(150);
    digitalWrite(m_ui8GpioPinRST, HIGH);
    bcm2835_delay(100);

    return (0);

}



//---------------------------------------------------------------------------
//  RF95InitModule
//---------------------------------------------------------------------------

int  RF95InitModule (int8_t i8TxPower_p, float flCentreFrequ_p)
{

bool  fRes;


    TRACE0("RF95InitModule:\n");

    if (m_pRF95 == NULL)
    {
        TRACE0("FAILED (m_pRF95 == NULL)!\n");
        return (-1);
    }

    // initialize RF95 board driver
    TRACE0("m_pRF95->init()...\n");
    fRes = m_pRF95->init();
    if ( !fRes )
    {
        TRACE0("FAILED!\n");
        return (-2);
    }

    // set transmitter power output level
    TRACE0("m_pRF95->setTxPower()...\n");
    m_pRF95->setTxPower(i8TxPower_p, false);

    // set transmitter and receiver centre frequency
    TRACE0("m_pRF95->setFrequency()...\n");
    m_pRF95->setFrequency(flCentreFrequ_p);

    // grab all packets received by this node
    TRACE0("m_pRF95->setPromiscuous()...\n");
    m_pRF95->setPromiscuous(true);

    // enabele listening for for all incoming messages
    TRACE0("m_pRF95->setModeRx()...\n");
    m_pRF95->setModeRx();

    return (0);

}



//---------------------------------------------------------------------------
//  RF95GetRecvDataPacket
//---------------------------------------------------------------------------

bool  RF95GetRecvDataPacket (uint8_t* pabRxDataBuff_p, uint* puiRxDataBuffLen_p, int8_t* pi8LastRssi_p)
{

uint8_t  abRxDataBuff[RH_RF95_MAX_PAYLOAD_LEN];
uint8_t  ui8RxDataPackLen;
uint8_t  ui8IrqFlags;
int8_t   i8LastRssi;
uint     uiRxDataBuffLen;
bool     fRxValid;


    TRACE0("RF95GetRecvDataPacket:\n");

    // read interrupt register
    ui8IrqFlags = m_pRF95->spiRead(RH_RF95_REG_12_IRQ_FLAGS);
    fRxValid = (ui8IrqFlags & RH_RF95_RX_DONE) ? true : false;
    TRACE1("fRxValid = %d\n", (int)fRxValid);

    // get received data from RF95
    if ( fRxValid )
    {
        // packet successfully received
        ui8RxDataPackLen = m_pRF95->spiRead(RH_RF95_REG_13_RX_NB_BYTES);
        if (ui8RxDataPackLen > sizeof(abRxDataBuff))
        {
            // limt copy size to max. buffer size
            ui8RxDataPackLen = sizeof(abRxDataBuff);
        }

        // reset FIFO read ptr to beginning of packet
        m_pRF95->spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, m_pRF95->spiRead(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR));
        m_pRF95->spiBurstRead(RH_RF95_REG_00_FIFO, abRxDataBuff, ui8RxDataPackLen);

        // clear all IRQ flags
        m_pRF95->spiWrite(RH_RF95_REG_12_IRQ_FLAGS, 0xFF);

        // remember the RSSI of this packet
        // this is according to the doc, but is it really correct?
        // weakest receiveable signals are reported RSSI at about -66
        i8LastRssi = m_pRF95->spiRead(RH_RF95_REG_1A_PKT_RSSI_VALUE) - 137;
    }

    // copy received data to application
    if ( fRxValid )
    {
        // reserve space for termination char at the end of buffer
        uiRxDataBuffLen = (*puiRxDataBuffLen_p) - sizeof('\0');

        // MIN(ui8RxDataPackLen, uiRxDataBuffLen)
        TRACE2("MIN(ui8RxDataPackLen=%u, uiRxDataBuffLen=%u) => ", (uint)ui8RxDataPackLen, uiRxDataBuffLen);
        if ((uint)ui8RxDataPackLen > uiRxDataBuffLen)
        {
            ui8RxDataPackLen = (uint8_t)uiRxDataBuffLen;
        }
        TRACE1("ui8RxDataPackLen=%u\n", (uint)ui8RxDataPackLen);

        // copy received data and terminate data block
        memcpy(pabRxDataBuff_p, abRxDataBuff, ui8RxDataPackLen);
        pabRxDataBuff_p[ui8RxDataPackLen] = '\0';

        // return used data buffer length
        *puiRxDataBuffLen_p = (uint)ui8RxDataPackLen;

        // return RSSI level
        *pi8LastRssi_p = i8LastRssi;
    }

    return (fRxValid);

}



//---------------------------------------------------------------------------
//  RF95DiagDumpRegs
//---------------------------------------------------------------------------

int  RF95DiagDumpRegs ()
{

uint8_t  aui8RegData[0x65];
int      iIdx;


    if (m_pRF95 == NULL)
    {
        return (-1);
    }


    printf("-------------\r\n");
    printf("Reg.  :  Data\r\n");
    printf("-------------\r\n");

    m_pRF95->spiBurstRead(RH_RF95_REG_00_FIFO, aui8RegData, sizeof(aui8RegData));
    for (iIdx=0; iIdx<sizeof(aui8RegData); iIdx++)
    {
        printf("0x%02X  :  0x%02X\r\n", iIdx, (uint)aui8RegData[iIdx]);
    }
    printf("\r\n");

    return (0);

}



//---------------------------------------------------------------------------
//  RF95DiagPrintConfig
//---------------------------------------------------------------------------

int  RF95DiagPrintConfig ()
{

uint8_t   ui8RegAddr;
uint8_t   ui8RegData;
uint      uiCfgData;
uint32_t  ui32Frequency;
float     flFrequency;
uint32_t  ui32Bitrate;
float     flBitrate;
uint32_t  ui32Fdev;
float     flFdev;
uint      uiPaSelect;
float     flPmax;
float     flPout;


    if (m_pRF95 == NULL)
    {
        return (-1);
    }


    printf("-------------\r\n");
    printf("Reg.  :  Data\r\n");
    printf("-------------\r\n");


    // ---- RH_RF95_REG_01_OP_MODE ----
    ui8RegAddr = RH_RF95_REG_01_OP_MODE;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_01_OP_MODE");
    printf("                       [.7]   LONG_RANGE_MODE    = %d\r\n", ((ui8RegData & RH_RF95_LONG_RANGE_MODE)    ? 1 : 0));
    printf("                       [.6]   ACCESS_SHARED_REG  = %d\r\n", ((ui8RegData & RH_RF95_ACCESS_SHARED_REG)  ? 1 : 0));
    printf("                       [.3]   LOW_FREQUENCY_MODE = %d\r\n", ((ui8RegData & RH_RF95_LOW_FREQUENCY_MODE) ? 1 : 0));
    printf("                       [.2-0] MODE               = ");
    switch (ui8RegData & RH_RF95_MODE)
    {
        case RH_RF95_MODE_SLEEP:            printf("MODE_SLEEP (%d)\r\n",        (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_STDBY:            printf("MODE_STDBY (%d)\r\n",        (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_FSTX:             printf("MODE_FSTX (%d)\r\n",         (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_TX:               printf("MODE_TX (%d)\r\n",           (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_FSRX:             printf("MODE_FSRX (%d)\r\n",         (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_RXCONTINUOUS:     printf("MODE_RXCONTINUOUS (%d)\r\n", (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_RXSINGLE:         printf("MODE_RXSINGLE (%d)\r\n",     (ui8RegData & RH_RF95_MODE));      break;
        case RH_RF95_MODE_CAD:              printf("MODE_CAD (%d)\r\n",          (ui8RegData & RH_RF95_MODE));      break;
        default:                            printf("??? (%d)\r\n",               (ui8RegData & RH_RF95_MODE));      break;
    }


    // ---- RH_RF95_REG_02_RESERVED / RH_RF95_REG_03_RESERVED (RH_RF95_REG_02_BITRATE_MSB / RH_RF95_REG_03_BITRATE_LSB) ----
    ui8RegAddr = RH_RF95_REG_02_RESERVED;       // = RH_RF95_REG_02_BITRATE_MSB
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_02_BITRATE_MSB");
    ui32Bitrate = (uint32_t)ui8RegData << 8;
    ui8RegAddr = RH_RF95_REG_03_RESERVED;       // = RH_RF95_REG_03_BITRATE_LSB
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_03_BITRATE_LSB");
    ui32Bitrate |= (uint32_t)ui8RegData;
    flBitrate = RH_RF95_FXOSC / (float)ui32Bitrate;
    flBitrate = round(flBitrate);
    printf("                       [.15-0] BITRATE = 0x%04lX -> %.1fkb/s\r\n", (ulong)ui32Bitrate, flBitrate/1000);


    // ---- RH_RF95_REG_04_RESERVED / RH_RF95_REG_05_RESERVED (RH_RF95_REG_04_FDEV_MSB / RH_RF95_REG_05_FDEV_LSB) ----
    ui8RegAddr = RH_RF95_REG_04_RESERVED;       // = RH_RF95_REG_04_FDEV_MSB
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_04_FDEV_MSB");
    ui32Fdev = (uint32_t)ui8RegData << 8;
    ui8RegAddr = RH_RF95_REG_05_RESERVED;       // = RH_RF95_REG_05_FDEV_LSB
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_05_FDEV_LSB");
    ui32Fdev |= (uint32_t)ui8RegData;
    flFdev = RH_RF95_FSTEP * (float)ui32Fdev;
    printf("                       [.15-0] FDEV = 0x%04lX -> %.1fkHz\r\n", (ulong)ui32Fdev, flFdev/1000);


    // ---- RH_RF95_REG_06_FRF_MSB / RH_RF95_REG_07_FRF_MID / RH_RF95_REG_08_FRF_LSB ----
    ui8RegAddr = RH_RF95_REG_06_FRF_MSB;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_06_FRF_MSB");
    ui32Frequency = (uint32_t)ui8RegData << 16;
    ui8RegAddr = RH_RF95_REG_07_FRF_MID;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_07_FRF_MID");
    ui32Frequency |= (uint32_t)ui8RegData << 8;
    ui8RegAddr = RH_RF95_REG_08_FRF_LSB;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_08_FRF_LSB");
    ui32Frequency |= (uint32_t)ui8RegData;
    flFrequency = ((float)ui32Frequency * RH_RF95_FSTEP) / 1000000.0;
    printf("                       [.23-0] FRF = 0x%06lX -> %3.2fMHz\r\n", (ulong)ui32Frequency, flFrequency);


    // ---- RH_RF95_REG_09_PA_CONFIG ----
    ui8RegAddr = RH_RF95_REG_09_PA_CONFIG;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_09_PA_CONFIG");
    uiPaSelect = (ui8RegData & RH_RF95_PA_SELECT) ? 1 : 0;
    printf("                       [.7]   RH_RF95_PA_SELECT    = %d -> %s\r\n", uiPaSelect, (uiPaSelect == 0) ? "RFO pin. Maximum power of +14 dBm" : "PA_BOOST pin. Maximum power of +20 dBm");
    uiCfgData = (ui8RegData & RH_RF95_MAX_POWER) >> 4;
    flPmax = 10.8 + (0.6 * uiCfgData);
    printf("                       [.6-4] RH_RF95_MAX_POWER    = %d -> %.1fdBm\r\n", uiCfgData, flPmax);
    uiCfgData = (ui8RegData & RH_RF95_OUTPUT_POWER);
    flPout = (uiPaSelect == 0) ? flPmax - (15 - uiCfgData) : 17 - (15 - uiCfgData);
    printf("                       [.3-0] RH_RF95_OUTPUT_POWER = %d -> %.1fdBm\r\n", uiCfgData, flPout);


    // ---- RH_RF95_REG_1D_MODEM_CONFIG1 ----
    ui8RegAddr = RH_RF95_REG_1D_MODEM_CONFIG1;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_1D_MODEM_CONFIG1");
    printf("                       [.7-4] SIGNAL_BANDWIDTH        = ");
    switch (ui8RegData & RH_RF95_BW)
    {
        case RH_RF95_BW_7_8KHZ:             printf("7.8kHz (%d)\r\n",   (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_10_4KHZ:            printf("10.4kHz (%d)\r\n",  (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_15_6KHZ:            printf("15.6kHz (%d)\r\n",  (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_20_8KHZ:            printf("20.8kHz (%d)\r\n",  (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_31_25KHZ:           printf("31.25kHz (%d)\r\n", (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_41_7KHZ:            printf("41.7kHz (%d)\r\n",  (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_62_5KHZ:            printf("62.5kHz (%d)\r\n",  (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_125KHZ:             printf("125kHz (%d)\r\n",   (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_250KHZ:             printf("250kHz (%d)\r\n",   (ui8RegData & RH_RF95_BW));                 break;
        case RH_RF95_BW_500KHZ:             printf("500kHz (%d)\r\n",   (ui8RegData & RH_RF95_BW));                 break;
        default:                            printf("??? (%d)\r\n",      (ui8RegData & RH_RF95_BW));                 break;
    }
    printf("                       [.3-1] CODING_RATE             = ");
    switch (ui8RegData & RH_RF95_CODING_RATE)
    {
        case RH_RF95_CODING_RATE_4_5:       printf("4/5 (%d)\r\n",      (ui8RegData & RH_RF95_CODING_RATE));        break;
        case RH_RF95_CODING_RATE_4_6:       printf("4/6 (%d)\r\n",      (ui8RegData & RH_RF95_CODING_RATE));        break;
        case RH_RF95_CODING_RATE_4_7:       printf("4/7 (%d)\r\n",      (ui8RegData & RH_RF95_CODING_RATE));        break;
        case RH_RF95_CODING_RATE_4_8:       printf("4/8 (%d)\r\n",      (ui8RegData & RH_RF95_CODING_RATE));        break;
        default:                            printf("??? (%d)\r\n",      (ui8RegData & RH_RF95_CODING_RATE));        break;
    }
    printf("                       [.0]   IMPLICIT_HEADER_MODE_ON = %d\r\n", ((ui8RegData & RH_RF95_IMPLICIT_HEADER_MODE_ON) ? 1 : 0));


    // ---- RH_RF95_REG_1E_MODEM_CONFIG2 ----
    ui8RegAddr = RH_RF95_REG_1E_MODEM_CONFIG2;
    ui8RegData = m_pRF95->spiRead(ui8RegAddr);
    printf("0x%02X  :  0x%02X  ->  %s\r\n", (uint)ui8RegAddr, (uint)ui8RegData, "RH_RF95_REG_1E_MODEM_CONFIG2");
    printf("                       [.7-4] SPREADING_FACTOR   = ");
    switch (ui8RegData & RH_RF95_SPREADING_FACTOR)
    {
        case RH_RF95_SPREADING_FACTOR_64CPS:    printf("SF6: 64 chips/symbol (%d)\r\n",     (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_128CPS:   printf("SF7: 128 chips/symbol (%d)\r\n",    (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_256CPS:   printf("SF8: 256 chips/symbol (%d)\r\n",    (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_512CPS:   printf("SF9: 512 chips/symbol (%d)\r\n",    (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_1024CPS:  printf("SF10: 1024 chips/symbol (%d)\r\n",  (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_2048CPS:  printf("SF11: 2048 chips/symbol (%d)\r\n",  (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        case RH_RF95_SPREADING_FACTOR_4096CPS:  printf("SF12: 4096 chips/symbol (%d)\r\n",  (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
        default:                                printf("??? (%d)\r\n",                      (ui8RegData & RH_RF95_SPREADING_FACTOR));       break;
    }
    printf("                       [.3]   TX_CONTINUOUS_MODE = %d\r\n", ((ui8RegData & RH_RF95_TX_CONTINUOUS_MOE) ? 1 : 0));
    printf("                       [.2]   PAYLOAD_CRC_ON     = %d\r\n", ((ui8RegData & RH_RF95_PAYLOAD_CRC_ON) ? 1 : 0));


    printf("\r\n");

    return (0);

}




// EOF


