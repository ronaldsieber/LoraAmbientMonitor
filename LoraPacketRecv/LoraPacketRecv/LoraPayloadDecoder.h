/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Class <LoraPayloadDecoder> Declaration

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LORAPAYLOADDECODER_H_
#define _LORAPAYLOADDECODER_H_





//---------------------------------------------------------------------------
//  Type Definitions
//---------------------------------------------------------------------------







/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          CLASS  LoraPayloadDecoder                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

class  LoraPayloadDecoder
{

    //-----------------------------------------------------------------------
    //  Definitions
    //-----------------------------------------------------------------------

    public:

        // Status of PayloadSegment (Header / DataBlock)
        typedef enum
        {
            kStatusUnused,
            kStatusCrcError,
            kStatusValid

        } tDataStatus;



        // ---- LoRa Bootup Data ----
        typedef struct
        {                                                       // ------------------+-----------+---------------------+-----------------
                                                                // Value             | Size[Bit] | Data Range          | Value Range
            tDataStatus     m_DataStatus;                       // ------------------+-----------+---------------------+-----------------
            tLoraPacketType m_PacketType;                       // PacketType           4          <tLoraPacketType>     <kLoraPacketBootup>
            uint8_t         m_ui8DevID;                         // DevID                4          0..15                 0..15
            uint8_t         m_ui8FirmwareVersion;               // FirmwareVersion      8          0..255                0..255
            uint8_t         m_ui8FirmwareRevision;              // FirmwareRevision     8          0..255                0..255
            uint16_t        m_ui16DataPackCycleTm;              // DataPacketCycleTm   16          0..65535              0..1092 [min]
            bool            m_fCfgOledDisplay;                  // CfgOledDisplay       1          true | false          true | false
            bool            m_fCfgDhtSensor;                    // CfgDhtSensor         1          true | false          true | false
            bool            m_fCfgSr501Sensor;                  // CfgSr501Sensor       1          true | false          true | false
            bool            m_fCfgAdcLightSensor;               // CfgAdcLightSensor    1          true | false          true | false
            bool            m_fCfgAdcCarBatAin;                 // CfgAdcCarBatAin      1          true | false          true | false
            bool            m_fCfgAsyncLoraEvent;               // CfgAsyncLoraEvent    1          true | false          true | false
            bool            m_fSr501PauseOnLoraTx;              // Sr501PauseOnLoraTx   1          true | false          true | false
            bool            m_fCommissioningMode;               // CommissioningMode    1          true | false          true | false
            uint8_t         m_ui8LoraTxPower;                   // LoRaTxPower          8          0..255                2..20 [dB]
            uint8_t         m_ui8LoraSpreadFactor;              // LoRaSpreadingFactor  8          0..255                6..12

        } tLoraStationBootup;



        // [Substructure DataHeader]
        typedef struct
        {                                                       // ------------------+-----------+---------------------+-----------------
                                                                // Value             | Size[Bit] | Data Range          | Value Range
            tDataStatus     m_DataStatus;                       // ------------------+-----------+---------------------+-----------------
            tLoraPacketType m_PacketType;                       // PacketType           4          <tLoraPacketType>     <kLoraPacketDataHeader>
            uint8_t         m_ui8DevID;                         // DevID                4          0..15                 0..15
            uint32_t        m_ui32SequNum;                      // SequNum             24          0..16777216           0..16777216
            uint32_t        m_ui32Uptime;                       // Uptime              32          0..4294967296 [sec]   0..136 [year]

        } tDataHeader;


        // [Substructure DataRecord]
        typedef struct
        {                                                       // ------------------+-----------+---------------------+-----------------
                                                                // Value             | Size[Bit] | Data Range          | Value Range
            tDataStatus     m_DataStatus;                       // ------------------+-----------+---------------------+-----------------
            tLoraPacketType m_PacketType;                       // PacketType           4          <tLoraPacketType>     <kLoraPacketDataGen0/1/2>
            uint16_t        m_ui12UptimeSnippet;                // UptimeSnippet       12          0..4095 [10 sec]      0..11 [h]
            float           m_flTemperature;                    // Temperature          8          -128..127 [0.5 °C]    -64.0..63.5 [°C]
            float           m_flHumidity;                       // Humidity             7          0..127 [%]            0..100 [%]
            bool            m_fMotionActive;                    // MotionActive         1          true | false          true | false
            uint16_t        m_ui16MotionActiveTime;             // MotionActiveTime     8          0..255 [10 sec]       0..42 [min]
            uint16_t        m_ui16MotionActiveCount;            // MotionActiveCount    10         0..1023               0..1023
            uint8_t         m_ui8LightLevel;                    // LightLevel           6          0..63 [2 %]           0..100 [%]
            float           m_flCarBattLevel;                   // CarBattLevel         8          0..255 [0.1 V]        0..25.5 [V]

        } tDataRec;


        // ---- LoRa Station Data ----
        typedef struct
        {
            tDataHeader     m_DataHeader;
            tDataRec        m_aDataRec[3];

        } tLoraStationData;





    //-----------------------------------------------------------------------
    //  Private Attributes
    //-----------------------------------------------------------------------

    private:

        tLoraStationBootup  m_LoraStationBootup;
        tLoraStationData    m_LoraStationData;
        char                m_szLogStationBootup[1024];
        char                m_szLogStationData[4096];



    //-----------------------------------------------------------------------
    //  Public Methodes
    //-----------------------------------------------------------------------

    public:

        LoraPayloadDecoder();
        ~LoraPayloadDecoder();

        tLoraPacketType     GetRxPacketType(const tLoraDataPacket* pLoraPacket_p);
        int                 GetRxPacketDevID(const tLoraDataPacket* pLoraPacket_p);
        tLoraStationBootup  DecodeRxBootupPacket(const tLoraDataPacket* pLoraPacket_p);
        tLoraStationData    DecodeRxDataPacket(const tLoraDataPacket* pLoraPacket_p);

        const char*         LogStationBootup(const tLoraStationBootup* pLoraStationBootup_p);
        const char*         LogStationData(const tLoraStationData* pLoraStationData_p);



    //-----------------------------------------------------------------------
    //  Private Methodes
    //-----------------------------------------------------------------------

    private:

        float     I7ToFloat(int8_t i8DataValue_p);
        float     I8ToFloat(int8_t i8DataValue_p);
        float     UI7ToFloat(uint8_t ui8DataValue_p);
        float     UI8ToFloat(uint8_t ui8DataValue_p);
        bool      IsCleared(const void* pDataBlock_p, unsigned int uiDataBlockSize_p);
        uint16_t  CalcCrc16(const void* pDataBlock_p, unsigned int uiDataBlockSize_p);
        size_t    LogStr(char* pszLogBuff_p, size_t nLogBuffSize_p, const char* pszFmt_p, ...);
        size_t    FormatUptime (uint32_t ui32Uptime_p, char* pszLogBuff_p, size_t nLogBuffSize_p, bool fForceDay_p = true, bool fForceTwoDigitsHours_p = true);

};



#endif  // _LORAPAYLAODDECODER_H_



