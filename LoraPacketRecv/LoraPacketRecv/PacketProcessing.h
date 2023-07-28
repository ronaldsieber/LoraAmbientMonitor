/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Declarations for LoRa Packet Processor

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _PACKETPROCESSING_H_
#define _PACKETPROCESSING_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------

typedef struct
{
    uint32_t            m_ui32SequNum;              // SequNum: 24Bit used -> 0..16777216
    uint32_t            m_ui32Uptime;               // Uptime:  32Bit used -> 0..4294967296 [sec] = 0..136 [year]
    time_t              m_tmTimeStamp;              // Linux Standard Time in Seconds since 01.01.1970

} tLoraStationDataReconstruct;


typedef struct
{
    // LoRa Message Basic Data
    uint                m_uiMsgID;                  // MsgID is generated on Receiver side
    time_t              m_tmTimeStamp;              // TimeStamp is generated on Receiver side
    int8_t              m_i8Rssi;

    // LoRa Message Raw Data
    uint8_t             m_abRawLoraMsg[RH_RF95_MAX_PAYLOAD_LEN+1];
    uint                m_uiRawLoraMsgLen;

    // Decoded Bootup or Station Data
    tLoraDataPacket                         m_LoraDataPacket;       // if KnownFormat then <m_LoraDataPacket> is a copy of <m_abRawLoraMsg>, otherwise is cleared with 0
    tLoraPacketType                         m_LoraPacketType;
    int                                     m_iLoraDevID;
    LoraPayloadDecoder::tLoraStationBootup  m_LoraStationBootup;    // -\ depending on <m_LoraPacketType> either <m_LoraStationBootup>
    LoraPayloadDecoder::tLoraStationData    m_LoraStationData;      // -/ or <m_LoraStationData> is filled with decoded data
    tLoraStationDataReconstruct             m_aLoraStationDataReconstruct[sizeof(m_LoraStationData.m_aDataRec)/sizeof(LoraPayloadDecoder::tDataRec)];
    std::string                             m_strLogLoraMsgData;

} tLoraMsgData;


typedef struct
{
    uint                m_uiMsgID;
    tLoraPacketType     m_PacketType;
    uint8_t             m_ui8DevID;
    uint32_t            m_ui32SequNum;
    int8_t              m_i8Rssi;
    time_t              m_tmTimeStamp;
    std::string         m_strJsonRecord;

} tJsonMessage;



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

int  PprGainLoraDataRecord (
    uint uiMsgID_p,                                     // [IN]     MessageID (e.g. RxPacketCntr)
    time_t tmTimeStamp_p,                               // [IN]     Message Receive TimeStamp
    int8_t i8Rssi_p,                                    // [IN]     Message Receive RSSI Level
    const uint8_t* pabRxDataBuff_p,                     // [IN]     Ptr to Message to decode
    uint uiRxDataBuffLen_p,                             // [IN]     Length of Message to decode
    tLoraMsgData* pLoraMsgData_p,                       // [IN/OUT] Ptr to LoRa Data Record to fill out
    bool* pfIsKnownLoraMsgFormat_p);                    // [IN/OUT] Ptr to Flag to signal Known LoRa Data Format or not


int  PprBuildJsonMessages (
    const tLoraMsgData* pLoraMsgData_p,                 // [IN]     Ptr to LoRa Data Record
    std::vector<tJsonMessage>* pvecJsonMessages_p);     // [IN/OUT] Ptr to Vector with Json Messages


int  PprBuildTelemetryMessage (
    const tJsonMessage* pJsonMessage_p,                 // [IN]     Json Message with Telemetry Data
    uint8_t* pabMsgBuffer_p,                            // [IN]     Ptr to Message Buffer
    uint uiMsgBufferLen_p);                             // [IN]     Length of Message Buffer


int  PprPrintLoraDataRecord (
    const tLoraMsgData* pLoraMsgData_p);                // [IN]     Ptr to LoRa Data Record to print out


int  PprPrintJsonMessages (
    std::vector<tJsonMessage>* pvecJsonMessages_p);     // [IN]     Ptr to Vector with Json Messages

int  PprPrintJsonMessage (
    tJsonMessage* pJsonMessage_p);                      // [IN]     Json Message to print



#endif  // #ifndef _PACKETPROCESSING_H_


// EOF

