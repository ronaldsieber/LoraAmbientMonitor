/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Definition of Over-the-Air LoRa Data Packet Structures

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/



//---------------------------------------------------------------------------
//  Definitions for LoRa Packet Types
//---------------------------------------------------------------------------

typedef enum
{
    kLoraPacketInvalid                      = -1,
    kLoraPacketUnused                       =  0,
    kLoraPacketBootup                       =  1,
    kLoraPacketDataHeader                   =  2,
    kLoraPacketDataGen0                     =  3,
    kLoraPacketDataGen1                     =  4,
    kLoraPacketDataGen2                     =  5

} tLoraPacketType;



//---------------------------------------------------------------------------
//  Definitions for LoRa Packets
//---------------------------------------------------------------------------

// [Substructure Header of LoRa Bootup Packet]
#pragma pack(push, 1)
typedef struct
{                                                       // ------------------+-----------+---------------------+-----------------
                                                        // Value             | Size[Bit] | Data Range          | Value Range
                                                        // ------------------+-----------+---------------------+-----------------
    uint64_t        m_ui4PacketType         :  4;       // PacketType           4          <tLoraPacketType>     <kLoraPacketBootup>
    uint64_t        m_ui4DevID              :  4;       // DevID                4          0..15                 0..15
    uint64_t        m_ui8FirmwareVersion    :  8;       // FirmwareVersion      8          0..255                0..255
    uint64_t        m_ui8FirmwareRevision   :  8;       // FirmwareRevision     8          0..255                0..255
    uint64_t        m_ui16DataPackCycleTm   : 16;       // DataPacketCycleTm   16          0..65535              0..1092 [min]
    uint64_t        m_ui1CfgOledDisplay     :  1;       // CfgOledDisplay       1          true | false          true | false
    uint64_t        m_ui1CfgDhtSensor       :  1;       // CfgDhtSensor         1          true | false          true | false
    uint64_t        m_ui1CfgSr501Sensor     :  1;       // CfgSr501Sensor       1          true | false          true | false
    uint64_t        m_ui1CfgAdcLightSensor  :  1;       // CfgAdcLightSensor    1          true | false          true | false
    uint64_t        m_ui1CfgAdcCarBatAin    :  1;       // CfgAdcCarBatAin      1          true | false          true | false
    uint64_t        m_ui1CfgAsyncLoraEvent  :  1;       // CfgAsyncLoraEvent    1          true | false          true | false
    uint64_t        m_ui1Sr501PauseOnLoraTx :  1;       // Sr501PauseOnLoraTx   1          true | false          true | false
    uint64_t        m_ui1CommissioningMode  :  1;       // CommissioningMode    1          true | false          true | false
    uint64_t        m_ui8LoraTxPower        :  8;       // LoRaTxPower          8          0..255                2..20 [dB]
    uint64_t        m_ui8LoraSpreadFactor   :  8;       // LoRaSpreadingFactor  8          0..255                6..12
    uint16_t        m_ui16CRC16;                        // CRC16               16

} tLoraBootupHeader;
#pragma pack(pop)


// [Substructure Header of LoRa Data Packet]
#pragma pack(push, 1)
typedef struct
{                                                       // ------------------+-----------+---------------------+-----------------
                                                        // Value             | Size[Bit] | Data Range          | Value Range
                                                        // ------------------+-----------+---------------------+-----------------
    uint64_t        m_ui4PacketType         :  4;       // PacketType           4          <tLoraPacketType>     <kLoraPacketDataHeader>
    uint64_t        m_ui4DevID              :  4;       // DevID                4          0..15                 0..15
    uint64_t        m_ui24SequNum           : 24;       // SequNum             24          0..16777216           0..16777216
    uint64_t        m_ui32Uptime            : 32;       // Uptime              32          0..4294967296 [sec]   0..136 [year]
    uint16_t        m_ui16CRC16;                        // CRC16               16

} tLoraDataHeader;
#pragma pack(pop)


// [Substructure Measurement Data of LoRa Data Packet]
#pragma pack(push, 1)
typedef struct
{                                                       // ------------------+-----------+---------------------+-----------------
                                                        // Value             | Size[Bit] | Data Range          | Value Range
                                                        // ------------------+-----------+---------------------+-----------------
    uint64_t        m_ui4PacketType         :  4;       // PacketType           4          <tLoraPacketType>     <kLoraPacketDataGen0/1/2>
    uint64_t        m_ui12UptimeSnippet     : 12;       // UptimeSnippet       12          0..4095 [10 sec]      0..11 [h]
    uint64_t        m_i8Temperature         :  8;       // Temperature          8          -128..127 [0.5 °C]    -64.0..63.5 [°C]
    uint64_t        m_ui7Humidity           :  7;       // Humidity             7          0..127 [%]            0..100 [%]
    uint64_t        m_ui1MotionActive       :  1;       // MotionActive         1          true | false          true | false
    uint64_t        m_ui8MotionActiveTime   :  8;       // MotionActiveTime     8          0..255 [10 sec]       0..42 [min]
    uint64_t        m_ui10MotionActiveCount : 10;       // MotionActiveCount   10          0..1023               0..1023
    uint64_t        m_ui6LightLevel         :  6;       // LightLevel           6          0..63 [2 %]           0..100 [%]
    uint64_t        m_ui8CarBattLevel       :  8;       // CarBattLevel         8          0..255 [0.1 V]        0..25.5 [V]
    uint16_t        m_ui16CRC16;                        // CRC16               16

} tLoraDataRec;
#pragma pack(pop)





//---------------------------------------------------------------------------
// LoRa Data Packet (Over-the-Air Data Packet)
//---------------------------------------------------------------------------
// Notice:  This structure <tLoraDataPacket> is implicitly used as a union for the both
//          packet types <BootupPacket> and <DataPacket>.
//          The beginnings as well as the size of both structures <tLoraBootupHeader> and
//          <tLoraDataHeader> are identical. The packet type is recognized by the element
//          <m_ui4PacketType>.
//
//          BootupPacket:   Header  -> tLoraBootupHeader
//                          DataRec -> tLoraDataRec[3] (completely filled with 0x00)
//
//          DataPacket:     Header  -> tLoraDataHeader
//                          DataRec -> tLoraDataRec[3] (3 generations of DataRecord)
//---------------------------------------------------------------------------
#pragma pack(push, 1)
typedef struct
{                                                       // -------------------------+-----------+--------------------------------
                                                        // Bootup Packet            | Size[Bit] | Data Packet
                                                        // -------------------------+-----------+--------------------------------
    tLoraDataHeader m_LoraHeader;                       // tLoraBootupHeader              80      tLoraDataHeader
    tLoraDataRec    m_aLoraDataRec[3];                  // tLoraDataRec[3] = { 0 }      3*80      tLoraDataRec[3]

} tLoraDataPacket;
#pragma pack(pop)




// EOF


