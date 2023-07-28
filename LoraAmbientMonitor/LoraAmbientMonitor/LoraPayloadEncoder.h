/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraPayloadEncoder> Declaration

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LORAPAYLOADENCODER_H_
#define _LORAPAYLOADENCODER_H_





//---------------------------------------------------------------------------
//  Type Definitions
//---------------------------------------------------------------------------







/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraPayloadEncoder                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class  LoraPayloadEncoder
{

    //-----------------------------------------------------------------------
    //  Definitions
    //-----------------------------------------------------------------------

    public:

        // data used to build-up LoRa packet substructure <tLoraBootupHeader>
        typedef struct
        {
            uint8_t     m_ui8FirmwareVersion;
            uint8_t     m_ui8FirmwareRevision;
            uint16_t    m_ui16DataPackCycleTm;
            bool        m_fCfgOledDisplay;
            bool        m_fCfgDhtSensor;
            bool        m_fCfgSr501Sensor;
            bool        m_fCfgAdcLightSensor;
            bool        m_fCfgAdcCarBatAin;
            bool        m_fCfgAsyncLoraEvent;
            bool        m_fSr501PauseOnLoraTx;
            bool        m_fCommissioningMode;
            uint8_t     m_ui8LoraTxPower;
            uint8_t     m_ui8LoraSpreadFactor;

        } tDeviceConfig;


        // data used to build-up LoRa packet substructures <tLoraDataHeader>/<tLoraDataRec>
        typedef struct
        {
            uint32_t    m_ui32Uptime;
            float       m_flTemperature;
            float       m_flHumidity;
            bool        m_fMotionActive;
            uint16_t    m_ui16MotionActiveTime;
            uint16_t    m_ui16MotionActiveCount;
            uint8_t     m_ui8LightLevel;
            float       m_flCarBattLevel;

        } tSensorDataRec;



    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        uint8_t         m_ui8DevID;
        uint32_t        m_ui32SequNum;
        tLoraDataPacket m_TxLoraBootupPacket;
        tLoraDataPacket m_TxLoraDataPacket;
        char            m_szLogDeviceConfig[1024];
        char            m_szLogSensorDataRec[1024];



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        LoraPayloadEncoder();
        ~LoraPayloadEncoder();

        void              Setup(uint8_t ui8DevID_p);
        int               EncodeTxBootupPacket(const tDeviceConfig* pDeviceConfig_p);
        tLoraDataPacket*  GetTxBootupPacket(void);
        int               EncodeTxDataPacket(const tSensorDataRec* pSensorDataRec_p);
        tLoraDataPacket*  GetTxDataPacket(void);

        const char*       LogDeviceConfig(const tDeviceConfig* pDeviceConfig_p);
        const char*       LogSensorDataRec(const tSensorDataRec* pSensorDataRec_p);



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        int8_t    FloatToI7(float flDataValue_p);
        int8_t    FloatToI8(float flDataValue_p);
        int8_t    FloatToUI7(float flDataValue_p);
        int8_t    FloatToUI8(float flDataValue_p);
        uint16_t  CalcCrc16(const void* pDataBlock_p, unsigned int uiDataBlockSize_p);
        size_t    LogStr(char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...);

};



#endif  // _LORAPAYLOADENCODER_H_



