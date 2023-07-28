/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraTransmitter> Declaration

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LORATRANSMITTER_H_
#define _LORATRANSMITTER_H_





//---------------------------------------------------------------------------
//  Type Definitions
//---------------------------------------------------------------------------







/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraTransmitter                                         */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class  LoraTransmitter
{

    //-----------------------------------------------------------------------
    //  Definitions
    //-----------------------------------------------------------------------

    public:

        typedef struct
        {
        
            int     m_iPinLora_SCK;
            int     m_iPinLora_MISO;
            int     m_iPinLora_MOSI;
            int     m_iPinLora_CS;
            int     m_iPinLora_RST;
            int     m_iPinLora_DIO0;
            int     m_iLoraTxPower;
            int     m_iLoraSpreadingFactor;
            long    m_lLoraSignalBandwidth;
            int     m_iLoraCodingRateDenominator;

        } tLoraTransmitterSettings;



    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        uint32_t  m_ui32TransmitInhibitTime;
        uint32_t  m_ui32TransmitCycleTime;
        uint32_t  m_ui32SysTickLastTransmitPacket;



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        LoraTransmitter();
        ~LoraTransmitter();

        int       Setup(const tLoraTransmitterSettings* pLoraTransmitterSettings_p, unsigned long uiRandomSeed_p);
        uint32_t  CalcNextTransmitCycleTime(uint32_t ui32LoraPacketInhibitTime_p, uint32_t ui32LoraPacketCycleTime_p);
        int       GetReasonToTransmitPacket(bool* pfAsyncTransmitEvent_p);
        int32_t   GetRemainingTransmitCycleTime();
        int       TransmitPacket(const void* pTxPacket_p, uint8_t ui8TxPacketLen_p, bool fLogDataToConsole_p = false);
        bool      GetTransmitIndicatorState(uint32_t ui32SignalActiveTime_p);



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        void  DumpBuffer(const void* pabDataBuff_p, unsigned int uiDataBuffLen_p);


};



#endif  // _LORATRANSMITTER_H_
