/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Ambient Monitor
  Description:  Main Module of LoRa Ambient Monitor

  -------------------------------------------------------------------------

    Arduino IDE Settings:

    Board:              "ESP32 Dev Module"
    Upload Speed:       "115200"
    CPU Frequency:      "240MHz (WiFi/BT)"
    Flash Frequency:    "80Mhz"
    Flash Mode:         "QIO"
    Flash Size:         "4MB (32Mb)"
    Partition Scheme:   "No OTA (2MB APP/2MB SPIFFS)"
    PSRAM:              "Disabled"

  -------------------------------------------------------------------------

        DIP Switch

        +-----------------+
        | ON          DIP |
        | +-+ +-+ +-+ +-+ |
        | | | | | | | | | |
        | |*| |*| |*| |*| |
        | +-+ +-+ +-+ +-+ |
        |  1   2   3   4  |
        +-----------------+
           |   |   |   |
           |   |   +-+-+
           |   |     |
           |   |     +--------->    DevID:   DIP3 | DIP4 | DevID
           |   |                             -----+------+------
           |   |                               0  |   0  |   0
           |   |                               0  |   1  |   1
           |   |                               1  |   0  |   2
           |   |                               1  |   1  |   3
           |   |
           |   |
           |   +--------------->    Pause SEN-HC-SR501 Sensor (IR Motion Sensor)
           |                        during LoRa Packet Transmission
           |                        (avoid faulty signaling of SR501 Sensor due to
           |                        crosstalk from LoRa transmitting module)
           |
           |
           +------------------->    Generate asynchronous LoRa Transmission Events:
                                    - SEN-HC-SR501 Sensor (IR Motion Sensor):
                                      triggered when the Sensor has detected an event
                                    - CarBatt AnalogIn (AI1 @ ADS1115):
                                      triggered when connecting a CarBatt was detected


        CFG Button

        +-+
        |*| ------------------->    Pressed during Bootup (Power-on or Reset):
        +-+                         Device is running in Commisioning Mode,
                                    LoRa Packets are transmitted at shorter intervals
                                    to facilitate commissioning by increasing the number
                                    of LoRa Packets

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

//#define DEBUG                                                           // Enable/Disable TRACE
#define DEBUG_DUMP_BUFFER


#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <U8x8lib.h>
#include <DHT.h>
#include "SimpleMovingAverage.hpp"
#include "LoraPacket.h"
#include "LoraPayloadEncoder.h"
#include "LoraTransmitter.h"
#include "Trace.h"





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Application Configuration
//---------------------------------------------------------------------------

const int       APP_VERSION                         = 1;                // 1.xx
const int       APP_REVISION                        = 0;                // x.00
const char      APP_BUILD_TIMESTAMP[]               = __DATE__ " " __TIME__;

const int       CFG_ENABLE_OLED_DISPLAY             = 1;
const int       CFG_ENABLE_DHT_SENSOR               = 1;
const int       CFG_ENABLE_SEN_HC_SR501_SENSOR      = 1;
const int       CFG_ENABLE_ADS1115_LIGHT_SENSOR     = 1;
const int       CFG_ENABLE_ADS1115_CAR_BATT_AIN     = 1;
const int       CFG_ENABLE_LOG_LORA_PACKET_DATA     = 1;
const int       CFG_ENABLE_LOG_LORA_PACKET_DUMP     = 1;

const uint8_t   OLED_LINE_DEV_INFO                  = 0;
const uint8_t   OLED_LINE_TEMPERATURE               = 1;
const uint8_t   OLED_LINE_HUMIDITY                  = 2;
const uint8_t   OLED_LINE_PIR_CNT                   = 3;
const uint8_t   OLED_LINE_LIGHT                     = 4;
const uint8_t   OLED_LINE_CAR_BATT                  = 5;
const uint8_t   OLED_LINE_LORA_PACK_CTN             = 6;
const uint8_t   OLED_LINE_LORA_TM_NXT               = 7;
const uint8_t   OLED_OFFS_DATA_VALUE                = 8;

const float     ADS1115_DIGIT                       = ((float)6144/(float)32768);   // max. ADC Input Voltage: 6.144V / Resulution: 15Bit with Signum => 0.1875mV
const float     ADC_CAR_BATT_VOLTAGE_DIVIDER        = ((float)4.78);                // R6=68k/R7=18k -> 12V/4.78 = 2.51V@ADC1
const float     ADC_CAR_BATT_RPD_FORWARD_VOLTAGE    = ((float)0.5);                 // Forward Voltage of Reverse Polarity Protection Diode D2 at CarBatt Input Connector
const float     ADC_CAR_BATT_VALID_LEVEL_MIN        = ((float)3.0);                 // min. ADC Input Voltage above which the CarBatt Voltage is recognized as valid

const int       SMA_DHT_SAMPLE_WINDOW_SIZE          = 200;              // Window Size of Simple Moving Average Filter for DHT-Sensor (Temperature/Humidity)
const int       SMA_CARBATT_SAMPLE_WINDOW_SIZE      = 20;               // Window Size of Simple Moving Average Filter for CarBattLevel (AI1 @ ADS1115)



//---------------------------------------------------------------------------
//  Hardware/Pin Configuration
//---------------------------------------------------------------------------

const int       PIN_SW_DIP1                         = 36;               // SW_DIP1              (GPIO36 -> Pin33)
const int       PIN_SW_DIP2                         = 37;               // SW_DIP2              (GPIO37 -> Pin32)
const int       PIN_SW_DIP3                         = 38;               // SW_DIP3              (GPIO38 -> Pin31)
const int       PIN_SW_DIP4                         = 39;               // SW_DIP4              (GPIO39 -> Pin30)

const int       PIN_KEY_CFG                         = 13;               // KEY_CFG              (GPIO13 -> Pin20)

const int       PIN_LED_MOTION_ACTIVE               = 25;               // LED_MOTION_ACTIVE    (GPIO25 -> Pin25)   (Red)    (= on-board LED (white))
const int       PIN_LED_LORA_TRANSMIT               = 12;               // LED_LORA_TRANSMIT    (GPIO12 -> Pin21)   (Green)
const int       PIN_LED_CAR_BATT_PLUGGED            = 23;               // LED_CAR_BATT_PLUGGED (GPIO23 -> Pin11)   (Yellow)

const int       PIN_OLED_SCL                        = 15;               // GPIO Pin connected to OLED_SCL  (GPIO15 -> Pin14)
const int       PIN_OLED_SDA                        =  4;               // GPIO Pin connected to OLED_SDA  (GPIO04 -> Pin16)
const int       PIN_OLED_RST                        = 16;               // GPIO Pin connected to OLED_RST  (GPIO16 -> Pin18)

const int       PIN_LORA_SCK                        =  5;               // GPIO Pin connected to LORA_SCK  (GPIO05 -> Pin13)
const int       PIN_LORA_MISO                       = 19;               // GPIO Pin connected to LORA_MISO (GPIO19 -> Pin10)
const int       PIN_LORA_MOSI                       = 27;               // GPIO Pin connected to LORA_MOSI (GPIO27 -> Pin23)
const int       PIN_LORA_CS                         = 18;               // GPIO Pin connected to LORA_CS   (GPIO18 -> Pin12)
const int       PIN_LORA_RST                        = 14;               // GPIO Pin connected to LORA_RST  (GPIO14 -> Pin22)
const int       PIN_LORA_DIO0                       = 26;               // GPIO Pin connected to LORA_DIO0 (GPIO26 -> Pin24)

const int       TYPE_DHT                            = DHT22;            // DHT11, DHT21 (AM2301), DHT22 (AM2302,AM2321)
const int       PIN_DHT22_SENSOR                    = 17;               // PIN used for DHT22 (AM2302/AM2321)   (GPIO17 -> Pin17)
const uint32_t  DHT_SENSOR_SAMPLE_PERIOD            = (5 * 1000);       // Sample Period for DHT-Sensor in [ms]

const int       PIN_SEN_HC_SR501                    =  2;               // GPIO Pin connected to HC-SR501 Sensor (IR Motion Sensor) (GPIO02 -> Pin15)
const uint32_t  SEN_HC_SR501_PAUSE_PERIOD           = (3 * 1000);       // Pause Period for HC-SR501 Sensor (IR Motion Sensor) in [ms]

const uint8_t   ADC_CH_LIGHT_SENSOR                 =  0;               // ADC[0] = Light Sensor
const uint8_t   ADC_CH_CAR_BATT_AIN                 =  1;               // ADC[1] = CarBattery Level



//---------------------------------------------------------------------------
//  LoRa Radio Configuration
//---------------------------------------------------------------------------

const int       LORA_TX_POWER                       = 20;               // LoRa Tx Power in dB  (Values: 2..20 for PA_OUTPUT_PA_BOOST_PIN, 0..14 for PA_OUTPUT_RFO_PIN, Default = 17)
const int       LORA_SPREADING_FACTOR               = 12;               // LoRa SpreadingFactor (Values: 7..12, Default = 7)
const long      LORA_SIGNAL_BANDWIDTH               = 125E3;            // LoRa SignalBandwidth (Values: 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3, Default = 125E3)
const int       LORA_CODING_RATE_DENOMINATOR        = 5;                // LoRa CodingRateDenominator (Values: 5..8, Default = 5 -> means 5/4)

const uint32_t  LORA_PACKET_INHIBIT_TIME            = (     30 * 1000); // LoRa Packet Inhibit Time [ms]

const uint32_t  LORA_PACKET_FIRST_TIME              = ( 5 * 60 * 1000); // Normal LoRa Mode:   Time between BootupPacket and first DataPacket [ms]
const uint32_t  LORA_PACKET_CYCLE_TIME              = (30 * 60 * 1000); // Normal LoRa Mode:   Time between DataPackets [ms]

const uint32_t  LORA_PACKET_FIRST_TIME_COMM_MODE    = ( 1 * 60 * 1000); // Commissioning Mode: Time between BootupPacket and first DataPacket [ms]
const uint32_t  LORA_PACKET_CYCLE_TIME_COMM_MODE    = ( 3 * 60 * 1000); // Commissioning Mode: Time between DataPackets [ms]

const uint32_t  LORA_TX_LED_SIGNAL_ACTIVE_TIME      = (     10 * 1000); // Active Time for LoRa Transmit Indicator LED [ms]



//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------

// Bits of DIP Switch
typedef struct
{
    uint8_t     m_fDip1 : 1;                                            // DIP '1' (left most)
    uint8_t     m_fDip2 : 1;                                            // DIP '2'
    uint8_t     m_fDip3 : 1;                                            // DIP '3'
    uint8_t     m_fDip4 : 1;                                            // DIP '4' (right most)

} tDipSwitch;


// Device Configuration -> LoRa Bootup Message
typedef struct
{

    int         m_iFirmwareVersion;
    int         m_iFirmwareRevision;
    uint32_t    m_ui32DataPackCycleTm;
    int         m_fCfgOledDisplay;
    int         m_fCfgDhtSensor;
    int         m_fCfgSr501Sensor;
    int         m_fCfgAdcLightSensor;
    int         m_fCfgAdcCarBatAin;
    bool        m_fCfgAsyncLoraEvent;
    bool        m_fSr501PauseOnLoraTx;
    bool        m_fCommissioningMode;
    int         m_iLoraTxPower;
    int         m_iLoraSpreadFactor;

} tDeviceConfig;


// SensorData Record -> LoRa Data Message
typedef struct
{
    ulong       m_ulMainLoopCycle;
    uint32_t    m_ui32Uptime;
    tDipSwitch  m_DipBits;
    uint8_t     m_ui8DipValue;
    float       m_flTemperature;
    float       m_flHumidity;
    bool        m_fMotionActive;
    uint16_t    m_ui16MotionActiveTime;
    uint16_t    m_ui16MotionActiveCount;
    uint16_t    m_ui16AdcValLightLevel;
    uint8_t     m_ui8LightLevel;
    uint16_t    m_ui16AdcValCarBattLevel;
    float       m_flCarBattLevel;
    uint32_t    m_ui32LoraPacketCount;
    int32_t     m_i32LoraRemainingCycleTime;

} tSensorData;



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  DHT                                 DhtSensor_g(PIN_DHT22_SENSOR, TYPE_DHT);
static  Adafruit_ADS1115                    Ads1115_g;                  // I2C Address = 0x48 is hardcoded in Adafruit ADS1115 Library
static  U8X8_SSD1306_128X64_NONAME_SW_I2C   Oled_U8x8_g(PIN_OLED_SCL, PIN_OLED_SDA, PIN_OLED_RST);

static  SimpleMovingAverage<float>          AverageTemperature_g(SMA_DHT_SAMPLE_WINDOW_SIZE);
static  SimpleMovingAverage<float>          AverageHumidity_g(SMA_DHT_SAMPLE_WINDOW_SIZE);
static  SimpleMovingAverage<float>          AverageCarBattLevel_g(SMA_CARBATT_SAMPLE_WINDOW_SIZE);

static  LoraPayloadEncoder                  LoraPayloadEnc_g;
static  LoraTransmitter                     LoraTransmitter_g;

static  ulong           ulMainLoopCycle_g           = 0;
static  uint            uiMainLoopProcStep_g        = 0;
static  uint32_t        ui32LastTickDhtRead_g       = 0;
static  uint32_t        ui32StartTickMotionSegm_g   = 0;
static  uint16_t        ui16MotionActiveTimeBase_g  = 0;
static  uint32_t        ui32StartTickPauseSr501_g   = 0;
static  bool            fPauseActiveSr501_g         = false;
static  bool            fCarBattPlugged_g           = false;
static  bool            fAsyncLoraTransmitEvent_g   = false;

static  bool            fGenAsyncLoraEvent_g        = false;            // derived from DIP1
static  bool            fSr501PauseOnLoraTx_g       = false;            // derived from DIP2
static  bool            fCfgBtnState_g              = false;
static  bool            fCommissioningMode_g        = false;            // derived from CFG Button
static  bool            fLedLoRaTransmit_g          = LOW;
static  bool            fLedMotionActive_g          = LOW;
static  bool            fLedCarBattPlugged_g        = LOW;

static  tDeviceConfig   DeviceConfig_g              = { 0 };
static  tSensorData     SensorDataRec_g             = { 0 };
static  tSensorData     PrevSensorDataRec_g         = { 0 };



//---------------------------------------------------------------------------
//  Local functions
//---------------------------------------------------------------------------





//=========================================================================//
//                                                                         //
//          S K E T C H   P U B L I C   F U N C T I O N S                  //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Application Setup
//---------------------------------------------------------------------------

void setup()
{

LoraTransmitter::tLoraTransmitterSettings  LoraTransmitterSettings;
tLoraDataPacket*  pLoraDataPacket;
char              szTextBuff[256];
uint8_t           ui8DevID;
unsigned long     uiRandomSeed;
uint32_t          ui32LoraNextTransmitCycleTime;
bool              fLogDataToConsole;
int               iRes;


    // Serial console
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println("======== APPLICATION START ========");
    Serial.println();
    Serial.flush();


    // Application Version Information
    snprintf(szTextBuff, sizeof(szTextBuff), "App Version:              %u.%02u", APP_VERSION, APP_REVISION);
    Serial.println(szTextBuff);
    snprintf(szTextBuff, sizeof(szTextBuff), "Build Timestamp:          %s", APP_BUILD_TIMESTAMP);
    Serial.println(szTextBuff);
    Serial.println();
    Serial.flush();


    // Initialize Workspace
    ulMainLoopCycle_g          = 0;
    uiMainLoopProcStep_g       = 0;
    ui32LastTickDhtRead_g      = 0;
    ui32StartTickMotionSegm_g  = 0;
    ui16MotionActiveTimeBase_g = 0;
    ui32StartTickPauseSr501_g  = 0;
    fPauseActiveSr501_g        = false;
    fCarBattPlugged_g          = false;
    fAsyncLoraTransmitEvent_g  = false;
    fGenAsyncLoraEvent_g       = false;
    fSr501PauseOnLoraTx_g      = false;
    fCfgBtnState_g             = false;
    fCommissioningMode_g       = false;
    fLedLoRaTransmit_g         = LOW;
    fLedMotionActive_g         = LOW;
    fLedCarBattPlugged_g       = LOW;
    memset(&DeviceConfig_g, 0x00, sizeof(DeviceConfig_g));
    memset(&SensorDataRec_g, 0x00, sizeof(SensorDataRec_g));
    memset(&PrevSensorDataRec_g, 0x00, sizeof(PrevSensorDataRec_g));


    // Setup Status LEDs
    Serial.println("Setup Status LEDs...");
    StatusLedsSetup();


    // Setup Config Button
    Serial.println("Setup Config Button...");
    CfgBtnSetup();


    // Setup DIP Switch
    Serial.println("Setup DIP Switch...");
    DipSetup();
    PrevSensorDataRec_g.m_ui8DipValue = 0xFF;


    // Get Config Button state
    fCfgBtnState_g = CfgBtnGetState();
    snprintf(szTextBuff, sizeof(szTextBuff), "CFG Button:               %u", (unsigned int)(fCfgBtnState_g & 0x01));
    Serial.println(szTextBuff);
    fCommissioningMode_g = fCfgBtnState_g ? true : false;


    // Get DIP Switch Setting
    SensorDataRec_g.m_ui8DipValue = DipGetSetting(&SensorDataRec_g.m_DipBits);
    snprintf(szTextBuff, sizeof(szTextBuff), "DIP Switch:               %u%u%u%ub (%02XH)",
                                            (unsigned int)(SensorDataRec_g.m_DipBits.m_fDip1 & 0x01),
                                            (unsigned int)(SensorDataRec_g.m_DipBits.m_fDip2 & 0x01),
                                            (unsigned int)(SensorDataRec_g.m_DipBits.m_fDip3 & 0x01),
                                            (unsigned int)(SensorDataRec_g.m_DipBits.m_fDip4 & 0x01),
                                            (unsigned int)SensorDataRec_g.m_ui8DipValue);
    Serial.println(szTextBuff);
    ui8DevID = SensorDataRec_g.m_DipBits.m_fDip3 << 1 | SensorDataRec_g.m_DipBits.m_fDip4;
    snprintf(szTextBuff, sizeof(szTextBuff), "  DevID:                  %u", (unsigned int)ui8DevID);
    Serial.println(szTextBuff);


    // Setup OLED Display
    if ( CFG_ENABLE_OLED_DISPLAY )
    {
        Serial.println("Setup OLED Display...");
        Oled_U8x8_g.begin();
        Oled_U8x8_g.setFont(u8x8_font_chroma48medium8_r);
        Oled_U8x8_g.clear();
        OledShowStartScreen(APP_VERSION, APP_REVISION, ui8DevID);
    }


    // Test all LED'a
    Serial.println("Run LED System Test...");
    RunLedSystemTest();


    // Setup DHT22 Sensor (Temerature/Humidity)
    if ( CFG_ENABLE_DHT_SENSOR )
    {
        Serial.println("Setup DHT22 Sensor...");
        DhtSensor_g.begin();
        snprintf(szTextBuff, sizeof(szTextBuff), "  Sample Period:          %u [sec]", (DHT_SENSOR_SAMPLE_PERIOD / 1000));
        Serial.println(szTextBuff);
        snprintf(szTextBuff, sizeof(szTextBuff), "  DHT SMA WndSize:        %u", SMA_DHT_SAMPLE_WINDOW_SIZE);
        Serial.println(szTextBuff);
    }


    // Setup ADS1115 ADC @ I2C
    if ( CFG_ENABLE_ADS1115_LIGHT_SENSOR || CFG_ENABLE_ADS1115_CAR_BATT_AIN )
    {
        Serial.println("Setup ADS1115 ADC...");
        Ads1115_g.begin();

        snprintf(szTextBuff, sizeof(szTextBuff), "  CarBatt SMA WndSize:    %u", SMA_CARBATT_SAMPLE_WINDOW_SIZE);
        Serial.println(szTextBuff);

    }


    // Setup SEN-HC-SR501 Sensor (IR Motion Sensor)
    if ( CFG_ENABLE_SEN_HC_SR501_SENSOR )
    {
        Serial.println("Setup SEN_HC_SR501 Sensor...");
        pinMode(PIN_SEN_HC_SR501, INPUT);
        ui32StartTickMotionSegm_g  = 0;
        ui16MotionActiveTimeBase_g = 0;
        fGenAsyncLoraEvent_g = (bool)SensorDataRec_g.m_DipBits.m_fDip1;
        snprintf(szTextBuff, sizeof(szTextBuff), "  Gen Async LoRa Events:  %u", (unsigned)fGenAsyncLoraEvent_g);
        Serial.println(szTextBuff);
        fSr501PauseOnLoraTx_g = (bool)SensorDataRec_g.m_DipBits.m_fDip2;
        snprintf(szTextBuff, sizeof(szTextBuff), "  Pause on LoRa Tx:       %u", (unsigned)fSr501PauseOnLoraTx_g);
        Serial.println(szTextBuff);
        PrevSensorDataRec_g.m_fMotionActive = true;         // just force a timely update in the OLED display
    }


    // Setup LoRa Payload Encoder
    Serial.println("Setup LoRa Payload Encoder...");
    LoraPayloadEnc_g.Setup(ui8DevID);


    // Setup LoRa Transmitter
    Serial.println("Setup LoRa Transmitter...");
    LoraTransmitterSettings.m_iPinLora_SCK               = PIN_LORA_SCK;
    LoraTransmitterSettings.m_iPinLora_MISO              = PIN_LORA_MISO;
    LoraTransmitterSettings.m_iPinLora_MOSI              = PIN_LORA_MOSI;
    LoraTransmitterSettings.m_iPinLora_CS                = PIN_LORA_CS;
    LoraTransmitterSettings.m_iPinLora_RST               = PIN_LORA_RST;
    LoraTransmitterSettings.m_iPinLora_DIO0              = PIN_LORA_DIO0;
    LoraTransmitterSettings.m_iLoraTxPower               = LORA_TX_POWER;
    LoraTransmitterSettings.m_iLoraSpreadingFactor       = LORA_SPREADING_FACTOR;
    LoraTransmitterSettings.m_lLoraSignalBandwidth       = LORA_SIGNAL_BANDWIDTH;
    LoraTransmitterSettings.m_iLoraCodingRateDenominator = LORA_CODING_RATE_DENOMINATOR;
    snprintf(szTextBuff, sizeof(szTextBuff), "  TxPower:                %u [dB]\n  SpreadingFactor:        %u\n  SignalBandwidth:        %.1f [kHz]\n  CodingRateDenominator:  %u",
                                            (unsigned int)LoraTransmitterSettings.m_iLoraTxPower,
                                            (unsigned int)LoraTransmitterSettings.m_iLoraSpreadingFactor,
                                            (float)LoraTransmitterSettings.m_lLoraSignalBandwidth / 1000.0,     // [Hz] -> [kHz]
                                            (unsigned int)LoraTransmitterSettings.m_iLoraCodingRateDenominator);
    Serial.println(szTextBuff);
    uiRandomSeed = ui8DevID * 1000;
    snprintf(szTextBuff, sizeof(szTextBuff), "  RandomSeed:             %u", uiRandomSeed);
    Serial.println(szTextBuff);
    LoraTransmitter_g.Setup(&LoraTransmitterSettings, uiRandomSeed);


    // Encode and transmit LoRa Bootup Packet
    Serial.println("Encode LoRa Bootup Packet...");
    DeviceConfig_g.m_iFirmwareVersion    = APP_VERSION;
    DeviceConfig_g.m_iFirmwareRevision   = APP_REVISION;
    DeviceConfig_g.m_ui32DataPackCycleTm = LoraGetCyclicPacketIntervalTime();
    DeviceConfig_g.m_fCfgOledDisplay     = CFG_ENABLE_OLED_DISPLAY;
    DeviceConfig_g.m_fCfgDhtSensor       = CFG_ENABLE_DHT_SENSOR;
    DeviceConfig_g.m_fCfgSr501Sensor     = CFG_ENABLE_SEN_HC_SR501_SENSOR;
    DeviceConfig_g.m_fCfgAdcLightSensor  = CFG_ENABLE_ADS1115_LIGHT_SENSOR;
    DeviceConfig_g.m_fCfgAdcCarBatAin    = CFG_ENABLE_ADS1115_CAR_BATT_AIN;
    DeviceConfig_g.m_fCfgAsyncLoraEvent  = fGenAsyncLoraEvent_g;
    DeviceConfig_g.m_fSr501PauseOnLoraTx = fSr501PauseOnLoraTx_g;
    DeviceConfig_g.m_fCommissioningMode  = fCommissioningMode_g;
    DeviceConfig_g.m_iLoraTxPower        = LORA_TX_POWER;
    DeviceConfig_g.m_iLoraSpreadFactor   = LORA_SPREADING_FACTOR;
    fLogDataToConsole = CFG_ENABLE_LOG_LORA_PACKET_DATA;
    pLoraDataPacket = LoraEncodeBootupPacket(&DeviceConfig_g, fLogDataToConsole);
    if (pLoraDataPacket == NULL)
    {
        Serial.println("  LoraEncodeBootupPacket() FAILED!");
    }
    else
    {
        Serial.println("Transmit LoRa Bootup Packet...");
        if ( fSr501PauseOnLoraTx_g )
        {
            Serial.println("  Pause SEN_HC_SR501 Sensor during LoRa transmission");
            HcSr501SensorStartPause();
        }
        fLogDataToConsole = CFG_ENABLE_LOG_LORA_PACKET_DUMP;
        iRes = LoraTransmitter_g.TransmitPacket(pLoraDataPacket, sizeof(tLoraDataPacket), fLogDataToConsole);
        if (iRes == 0)
        {
            SensorDataRec_g.m_ui32LoraPacketCount++;
            snprintf(szTextBuff, sizeof(szTextBuff), "  LoraPacketCounter: %lu", (unsigned long)SensorDataRec_g.m_ui32LoraPacketCount);
            Serial.println(szTextBuff);
        }
        else
        {
            snprintf(szTextBuff, sizeof(szTextBuff), "  LoraTransmitter_g.TransmitPacket() FAILED! (iRes=%d)", iRes);
            Serial.println(szTextBuff);
        }
    }

    // Calculate time interval for transmitting 1st LoRa Data Packet
    ui32LoraNextTransmitCycleTime = LoraTransmitter_g.CalcNextTransmitCycleTime(LORA_PACKET_INHIBIT_TIME, LoraGetFirstPacketIntervalTime());
    snprintf(szTextBuff, sizeof(szTextBuff), "TimeSpan until transmitting first cyclic LoRa DataPacket: %s", FormatDateTime(ui32LoraNextTransmitCycleTime, false, true).c_str());
    Serial.println(szTextBuff);


    return;

}



//---------------------------------------------------------------------------
//  Application Main Loop
//---------------------------------------------------------------------------

void loop()
{

tLoraDataPacket*  pLoraDataPacket;
char      szTextBuff[128];
uint32_t  ui32CurrTick;
uint      uiProcStep;
uint32_t  ui32Uptime;
String    strUptime;
float     flTemperature;
float     flAverageTemperature;
float     flHumidity;
float     flAverageHumidity;
bool      fMotionActive;
uint16_t  ui16MotionActiveTime;
uint16_t  ui16AdcVal;
uint8_t   ui8LightLevel;
float     flCarBattLevel;
float     flAverageCarBattLevel;
uint32_t  ui32LoraNextTransmitCycleTime;
bool      fLogDataToConsole;
int       iRes;


    uiProcStep = uiMainLoopProcStep_g++ % 10;
    switch (uiProcStep)
    {
        // provide main information
        case 0:
        {
            ulMainLoopCycle_g++;
            Serial.println();
            strUptime = GetSysUptime(&ui32Uptime);
            snprintf(szTextBuff, sizeof(szTextBuff), "Main Loop Cycle:       %lu (Uptime: %s)", ulMainLoopCycle_g, strUptime.c_str());
            Serial.println(szTextBuff);
            SensorDataRec_g.m_ulMainLoopCycle = ulMainLoopCycle_g;
            SensorDataRec_g.m_ui32Uptime = ui32Uptime;
            break;
        }

        // process DHT Sensor (Temperature/Humidity)
        case 1:
        {
            if ( CFG_ENABLE_DHT_SENSOR )
            {
                Serial.print("DHT22 Sensor:          ");
                ui32CurrTick = millis();
                if ((ui32CurrTick - ui32LastTickDhtRead_g) < DHT_SENSOR_SAMPLE_PERIOD)
                {
                    Serial.println("[skipped]");
                    break;
                }
                iRes = DhtSensorGetData(&flTemperature, &flHumidity);
                if (iRes == 0)
                {
                    flAverageTemperature = AverageTemperature_g.CalcMovingAverage(flTemperature);
                    flAverageHumidity = AverageHumidity_g.CalcMovingAverage(flHumidity);
                    snprintf(szTextBuff, sizeof(szTextBuff), "Temperature = %.1f °C (Ø %.1f °C), Humidity = %.1f %% (Ø %.1f %%)", flTemperature, flAverageTemperature, flHumidity, flAverageHumidity);
                }
                else
                {
                    snprintf(szTextBuff, sizeof(szTextBuff), "FAILED! (iRes=%d)", iRes);
                }
                Serial.println(szTextBuff);
                SensorDataRec_g.m_flTemperature = flAverageTemperature;
                SensorDataRec_g.m_flHumidity = flAverageHumidity;
                ui32LastTickDhtRead_g = ui32CurrTick;
            }
            break;
        }

        // process SEN-HC-SR501 Sensor (IR Motion Sensor)
        case 2:
        {
            if ( CFG_ENABLE_SEN_HC_SR501_SENSOR )
            {
                Serial.print("SEN_HC_SR501 Sensor:   ");
                if ( IsPausedHcSr501Sensor() )
                {
                    Serial.println("PAUSED during LoRa transmission");
                    break;
                }
                iRes = HcSr501SensorGetData(&fMotionActive, &ui16MotionActiveTime);
                if (iRes == 0)
                {
                    if ( fMotionActive )
                    {
                        snprintf(szTextBuff, sizeof(szTextBuff), "Active (for %u sec)", (uint)ui16MotionActiveTime);
                    }
                    else
                    {
                        snprintf(szTextBuff, sizeof(szTextBuff), "Idle");
                    }
                }
                else
                {
                    snprintf(szTextBuff, sizeof(szTextBuff), "FAILED! (iRes=%d)", iRes);
                }
                Serial.println(szTextBuff);
                if ( !(SensorDataRec_g.m_fMotionActive) && fMotionActive )
                {
                    // rising edge on <fMotionActive> detected
                    SensorDataRec_g.m_ui16MotionActiveCount++;
                    if ( fGenAsyncLoraEvent_g )
                    {
                        // generate asynchronous LoRa transmission Event due to rising edge on <fMotionActive>
                        fAsyncLoraTransmitEvent_g = true;
                    }
                }
                SensorDataRec_g.m_fMotionActive = fMotionActive;
                SensorDataRec_g.m_ui16MotionActiveTime = ui16MotionActiveTime;
                fLedMotionActive_g = fMotionActive;
            }
            break;
        }

        // process Light Sensor (AI0 @ ADS1115)
        case 3:
        {
            if ( CFG_ENABLE_ADS1115_LIGHT_SENSOR )
            {
                Serial.print("ADS1115/Light Sensor:  ");
                iRes = GetAds1115LightSensorData(&ui16AdcVal, &ui8LightLevel);
                if (iRes == 0)
                {
                    snprintf(szTextBuff, sizeof(szTextBuff), "LightLevel = %u %% (ADC=%05u)", (uint)ui8LightLevel, ui16AdcVal);
                }
                else
                {
                    snprintf(szTextBuff, sizeof(szTextBuff), "FAILED! (iRes=%d)", iRes);
                }
                Serial.println(szTextBuff);
                SensorDataRec_g.m_ui16AdcValLightLevel = ui16AdcVal;
                SensorDataRec_g.m_ui8LightLevel = ui8LightLevel;
            }
            break;
        }

        // process CarBatt AnalogIn (AI1 @ ADS1115)
        case 4:
        {
            if ( CFG_ENABLE_ADS1115_CAR_BATT_AIN )
            {
                Serial.print("ADS1115/CarBatt AIN:   ");
                flAverageCarBattLevel = 0.0;
                iRes = GetAds1115CarBattAinData(&ui16AdcVal, &flCarBattLevel);
                if (iRes == 0)      // iRes == 0 -> Vadc >> 0V -> CarBatt connected
                {
                    if ( !fCarBattPlugged_g )
                    {
                        // rising edge on <flCarBattLevel> detected
                        if ( fGenAsyncLoraEvent_g )
                        {
                            // generate asynchronous LoRa transmission Event due to rising edge on <flCarBattLevel>
                            fAsyncLoraTransmitEvent_g = true;
                        }
                    }
                    fCarBattPlugged_g = true;
                    flAverageCarBattLevel = AverageCarBattLevel_g.CalcMovingAverage(flCarBattLevel);
                    snprintf(szTextBuff, sizeof(szTextBuff), "CarBattLevel = %.1f V (Ø %.1f V)", flCarBattLevel, flAverageCarBattLevel);
                }
                else                // iRes == -1 -> Vadc ~ 0V (noise) -> CarBatt disconnected
                {
                    if ( fCarBattPlugged_g )
                    {
                        // falling edge on <flCarBattLevel> detected
                        if ( fGenAsyncLoraEvent_g )
                        {
                            // generate asynchronous LoRa transmission Event due to falling edge on <flCarBattLevel>
                            fAsyncLoraTransmitEvent_g = true;
                        }
                        AverageCarBattLevel_g.Clean();      // clean history of Simple Moving Average Filter
                    }
                    fCarBattPlugged_g = false;
                    snprintf(szTextBuff, sizeof(szTextBuff), "NOT_CONNECTED (iRes=%d)", iRes);
                }
                Serial.println(szTextBuff);
                SensorDataRec_g.m_ui16AdcValCarBattLevel = ui16AdcVal;
                SensorDataRec_g.m_flCarBattLevel = flAverageCarBattLevel;
                fLedCarBattPlugged_g = fCarBattPlugged_g ? HIGH : LOW;
            }
            break;
        }

        case 5:
        case 6:
        case 7:
        {
            break;
        }

        // encode and transmit LoRa data packet
        case 8:
        {
            iRes = LoraTransmitter_g.GetReasonToTransmitPacket(&fAsyncLoraTransmitEvent_g);
            snprintf(szTextBuff, sizeof(szTextBuff), "Check Reason to Transmit Packet: -> %d", iRes);
            Serial.println(szTextBuff);
            if (iRes > 0)
            {
                Serial.print("LoraEncodeDataPacket... ");
                fLogDataToConsole = CFG_ENABLE_LOG_LORA_PACKET_DATA;
                pLoraDataPacket = LoraEncodeDataPacket(&SensorDataRec_g, fLogDataToConsole);
                if (pLoraDataPacket == NULL)
                {
                    Serial.println("LoraEncodeDataPacket() FAILED!");
                    break;
                }
                Serial.println("Transmit LoRa Data Packet...");
                if ( fSr501PauseOnLoraTx_g )
                {
                    Serial.println("  Pause SEN_HC_SR501 Sensor during LoRa transmission");
                    HcSr501SensorStartPause();
                }
                fLogDataToConsole = CFG_ENABLE_LOG_LORA_PACKET_DUMP;
                iRes = LoraTransmitter_g.TransmitPacket(pLoraDataPacket, sizeof(tLoraDataPacket), fLogDataToConsole);
                if (iRes == 0)
                {
                    SensorDataRec_g.m_ui32LoraPacketCount++;
                    snprintf(szTextBuff, sizeof(szTextBuff), "  LoraPacketCounter: %lu", (unsigned long)SensorDataRec_g.m_ui32LoraPacketCount);
                    Serial.println(szTextBuff);
                }
                else
                {
                    snprintf(szTextBuff, sizeof(szTextBuff), "  LoraTransmitter_g.TransmitPacket() FAILED! (iRes=%d)", iRes);
                    Serial.println(szTextBuff);
                }
                HcSr501SensorRestMotionActiveTime();

                // Calculate time interval for transmitting next LoRa Data Packet
                ui32LoraNextTransmitCycleTime = LoraTransmitter_g.CalcNextTransmitCycleTime(LORA_PACKET_INHIBIT_TIME, DeviceConfig_g.m_ui32DataPackCycleTm);
                snprintf(szTextBuff, sizeof(szTextBuff), "TimeSpan until transmitting next cyclic LoRa DataPacket: %s", FormatDateTime(ui32LoraNextTransmitCycleTime, false, true).c_str());
                Serial.println(szTextBuff);
                SensorDataRec_g.m_i32LoraRemainingCycleTime = LoraTransmitter_g.GetRemainingTransmitCycleTime();
            }
            else
            {
                SensorDataRec_g.m_i32LoraRemainingCycleTime = LoraTransmitter_g.GetRemainingTransmitCycleTime();
                snprintf(szTextBuff, sizeof(szTextBuff), "Remaining TimeSpan until transmitting next cyclic LoRa DataPacket: %s", FormatDateTime(SensorDataRec_g.m_i32LoraRemainingCycleTime, false, true).c_str());
                Serial.println(szTextBuff);
            }
            fLedLoRaTransmit_g = LoraTransmitter_g.GetTransmitIndicatorState(LORA_TX_LED_SIGNAL_ACTIVE_TIME);
            break;
        }

        // update process information on OLED
        case 9:
        {
            OledUpdateProcessData(&SensorDataRec_g, &PrevSensorDataRec_g);
            break;
        }

        default:
        {
            break;
        }
    }


    // set Status LEDs
    SetLedLoRaTransmit(GetStateLedLoRaTransmit(fLedLoRaTransmit_g, fCommissioningMode_g, uiMainLoopProcStep_g));
    SetLedMotionActive(fLedMotionActive_g);
    SetLedCarBattPlugged(fLedCarBattPlugged_g);

    delay(100);

    return;

}





//=========================================================================//
//                                                                         //
//          S K E T C H   P R I V A T E   F U N C T I O N S                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  LoRa: Encode Bootup Packet
//---------------------------------------------------------------------------

tLoraDataPacket*  LoraEncodeBootupPacket (const tDeviceConfig* pDeviceConfig_p, bool fLogDataToConsole_p)
{

LoraPayloadEncoder::tDeviceConfig  DeviceConfig;
tLoraDataPacket*  pLoraDataPacket;                  // <tLoraDataPacket> is also used for Bootup Packets
const char*       pszLogBuffer;
int               iRes;


    if (pDeviceConfig_p == NULL)
    {
        return (NULL);
    }

    DeviceConfig.m_ui8FirmwareVersion  = (uint8_t)(pDeviceConfig_p->m_iFirmwareVersion);
    DeviceConfig.m_ui8FirmwareRevision = (uint8_t)(pDeviceConfig_p->m_iFirmwareRevision);
    DeviceConfig.m_ui16DataPackCycleTm = pDeviceConfig_p->m_ui32DataPackCycleTm / 1000;     // [ms] -> [sec]
    DeviceConfig.m_fCfgOledDisplay     = (bool)(pDeviceConfig_p->m_fCfgOledDisplay);
    DeviceConfig.m_fCfgDhtSensor       = (bool)(pDeviceConfig_p->m_fCfgDhtSensor);
    DeviceConfig.m_fCfgSr501Sensor     = (bool)(pDeviceConfig_p->m_fCfgSr501Sensor);
    DeviceConfig.m_fCfgAdcLightSensor  = (bool)(pDeviceConfig_p->m_fCfgAdcLightSensor);
    DeviceConfig.m_fCfgAdcCarBatAin    = (bool)(pDeviceConfig_p->m_fCfgAdcCarBatAin);
    DeviceConfig.m_fCfgAsyncLoraEvent  = pDeviceConfig_p->m_fCfgAsyncLoraEvent;
    DeviceConfig.m_fSr501PauseOnLoraTx = pDeviceConfig_p->m_fSr501PauseOnLoraTx;
    DeviceConfig.m_fCommissioningMode  = pDeviceConfig_p->m_fCommissioningMode;
    DeviceConfig.m_ui8LoraTxPower      = (uint8_t)(pDeviceConfig_p->m_iLoraTxPower);
    DeviceConfig.m_ui8LoraSpreadFactor = (uint8_t)(pDeviceConfig_p->m_iLoraSpreadFactor);
    iRes = LoraPayloadEnc_g.EncodeTxBootupPacket(&DeviceConfig);
    if (iRes != 0)
    {
        return (NULL);
    }
    pLoraDataPacket = LoraPayloadEnc_g.GetTxBootupPacket();

    if ( fLogDataToConsole_p )
    {
        pszLogBuffer = LoraPayloadEnc_g.LogDeviceConfig(&DeviceConfig);
        Serial.println(pszLogBuffer);
    }

    return (pLoraDataPacket);

}



//---------------------------------------------------------------------------
//  LoRa: Encode Data Packet
//---------------------------------------------------------------------------

tLoraDataPacket*  LoraEncodeDataPacket (const tSensorData* pSensorDataRec_p, bool fLogDataToConsole_p)
{

LoraPayloadEncoder::tSensorDataRec  SensorDataRec;
tLoraDataPacket*  pLoraDataPacket;
const char*       pszLogBuffer;
int               iRes;


    if (pSensorDataRec_p == NULL)
    {
        return (NULL);
    }

    SensorDataRec.m_ui32Uptime            = pSensorDataRec_p->m_ui32Uptime / 1000;      // [ms] -> [sec]
    SensorDataRec.m_flTemperature         = pSensorDataRec_p->m_flTemperature;
    SensorDataRec.m_flHumidity            = pSensorDataRec_p->m_flHumidity;
    SensorDataRec.m_fMotionActive         = pSensorDataRec_p->m_fMotionActive;
    SensorDataRec.m_ui16MotionActiveTime  = pSensorDataRec_p->m_ui16MotionActiveTime;
    SensorDataRec.m_ui16MotionActiveCount = pSensorDataRec_p->m_ui16MotionActiveCount;
    SensorDataRec.m_ui8LightLevel         = pSensorDataRec_p->m_ui8LightLevel;
    SensorDataRec.m_flCarBattLevel        = pSensorDataRec_p->m_flCarBattLevel;
    iRes = LoraPayloadEnc_g.EncodeTxDataPacket(&SensorDataRec);
    if (iRes != 0)
    {
        return (NULL);
    }
    pLoraDataPacket = LoraPayloadEnc_g.GetTxDataPacket();

    if ( fLogDataToConsole_p )
    {
        pszLogBuffer = LoraPayloadEnc_g.LogSensorDataRec(&SensorDataRec);
        Serial.println(pszLogBuffer);
    }

    return (pLoraDataPacket);

}



//---------------------------------------------------------------------------
//  LoRa: Get First Packet Interval Time
//---------------------------------------------------------------------------

uint32_t  LoraGetFirstPacketIntervalTime ()
{

uint32_t  ui32FirstPacketIntervalTime;


    if (fCommissioningMode_g != true)
    {
        ui32FirstPacketIntervalTime = LORA_PACKET_FIRST_TIME;
    }
    else
    {
        ui32FirstPacketIntervalTime = LORA_PACKET_FIRST_TIME_COMM_MODE;
    }

    return (ui32FirstPacketIntervalTime);

}



//---------------------------------------------------------------------------
//  LoRa: Get Cyclic Packet Interval Time
//---------------------------------------------------------------------------

uint32_t  LoraGetCyclicPacketIntervalTime ()
{

uint32_t  ui32CyclicPacketIntervalTime;


    if (fCommissioningMode_g != true)
    {
        ui32CyclicPacketIntervalTime = LORA_PACKET_CYCLE_TIME;
    }
    else
    {
        ui32CyclicPacketIntervalTime = LORA_PACKET_CYCLE_TIME_COMM_MODE;
    }

    return (ui32CyclicPacketIntervalTime);

}



//---------------------------------------------------------------------------
//  LEDs: Setup GPIO Pins for Status LEDs
//---------------------------------------------------------------------------

void  StatusLedsSetup (void)
{

    // Status LEDs Setup
    pinMode(PIN_LED_MOTION_ACTIVE, OUTPUT);
    digitalWrite(PIN_LED_MOTION_ACTIVE, LOW);
    pinMode(PIN_LED_LORA_TRANSMIT, OUTPUT);
    digitalWrite(PIN_LED_LORA_TRANSMIT, LOW);
    pinMode(PIN_LED_CAR_BATT_PLUGGED, OUTPUT);
    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, LOW);

    return;

}



//---------------------------------------------------------------------------
//  LEDs: Set State for 'MotionActive'
//---------------------------------------------------------------------------

void  SetLedMotionActive (bool fLedState_p)
{

    digitalWrite(PIN_LED_MOTION_ACTIVE, (fLedState_p ? HIGH : LOW));
    return;

}



//---------------------------------------------------------------------------
//  LEDs: Set State for 'LoRaTransmit'
//---------------------------------------------------------------------------

void  SetLedLoRaTransmit (bool fLedState_p)
{

    digitalWrite(PIN_LED_LORA_TRANSMIT, (fLedState_p ? HIGH : LOW));
    return;

}



//---------------------------------------------------------------------------
//  LEDs: Set State for 'CarBattPlugged'
//---------------------------------------------------------------------------

void  SetLedCarBattPlugged (bool fLedState_p)
{

    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, (fLedState_p ? HIGH : LOW));
    return;

}



//---------------------------------------------------------------------------
//  LEDs: Get State for 'LoRaTransmit'
//---------------------------------------------------------------------------

bool  GetStateLedLoRaTransmit (bool fLedLoRaTransmit_p, bool fCommissioningMode_p, uint uiMainLoopProcStep_p)
{

bool  fStateLedLoRaTransmit;


    fStateLedLoRaTransmit = LOW;

    if ( fLedLoRaTransmit_p )
    {
        fStateLedLoRaTransmit = HIGH;
    }
    else
    {
        if ( fCommissioningMode_p )
        {
            fStateLedLoRaTransmit = ((uiMainLoopProcStep_p % 20) / 10) ? HIGH : LOW;
        }
    }

    return (fStateLedLoRaTransmit);

}



//---------------------------------------------------------------------------
//  LEDs: Run LED System Test
//---------------------------------------------------------------------------

void  RunLedSystemTest (void)
{

    digitalWrite(PIN_LED_MOTION_ACTIVE, LOW);
    digitalWrite(PIN_LED_LORA_TRANSMIT, LOW);
    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, LOW);
    delay(50);

    digitalWrite(PIN_LED_LORA_TRANSMIT, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_LORA_TRANSMIT, LOW);
    delay(50);

    digitalWrite(PIN_LED_MOTION_ACTIVE, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_MOTION_ACTIVE, LOW);
    delay(50);

    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, LOW);
    delay(50);

    digitalWrite(PIN_LED_MOTION_ACTIVE, LOW);
    digitalWrite(PIN_LED_LORA_TRANSMIT, LOW);
    digitalWrite(PIN_LED_CAR_BATT_PLUGGED, LOW);

    return;

}



//---------------------------------------------------------------------------
//  DIP: Setup GPIO Pins for DIP-Switch
//---------------------------------------------------------------------------

void  DipSetup (void)
{

    pinMode(PIN_SW_DIP1, INPUT);
    pinMode(PIN_SW_DIP2, INPUT);
    pinMode(PIN_SW_DIP3, INPUT);
    pinMode(PIN_SW_DIP4, INPUT);

    return;

}



//---------------------------------------------------------------------------
//  DIP: Get DIP-Switch Setting
//---------------------------------------------------------------------------

uint8_t  DipGetSetting (tDipSwitch* pDipBits_o)
{

tDipSwitch  DipBits;
bool        fDipPin;
uint8_t     ui8DipValue;


    ui8DipValue = 0x00;

    fDipPin = !digitalRead(PIN_SW_DIP1);                // DIPs are inverted (1=off, 0=on)
    DipBits.m_fDip1 = fDipPin;
    ui8DipValue |= (fDipPin & 0x01) << 3;

    fDipPin = !digitalRead(PIN_SW_DIP2);                // DIPs are inverted (1=off, 0=on)
    DipBits.m_fDip2 = fDipPin;
    ui8DipValue |= (fDipPin & 0x01) << 2;

    fDipPin = !digitalRead(PIN_SW_DIP3);                // DIPs are inverted (1=off, 0=on)
    DipBits.m_fDip3 = fDipPin;
    ui8DipValue |= (fDipPin & 0x01) << 1;

    fDipPin = !digitalRead(PIN_SW_DIP4);                // DIPs are inverted (1=off, 0=on)
    DipBits.m_fDip4 = fDipPin;
    ui8DipValue |= (fDipPin & 0x01) << 0;

    if (pDipBits_o != NULL)
    {
        *pDipBits_o = DipBits;
    }

    return (ui8DipValue);

}



//---------------------------------------------------------------------------
//  CfgBtn: Setup GPIO Pin for Config Button
//---------------------------------------------------------------------------

void  CfgBtnSetup (void)
{

    pinMode(PIN_KEY_CFG, INPUT);
    return;

}



//---------------------------------------------------------------------------
//  CfgBtn: Get Config Button State
//---------------------------------------------------------------------------

bool  CfgBtnGetState (void)
{

bool  fKeyPin;


    fKeyPin = !digitalRead(PIN_KEY_CFG);                // DIPs are inverted (1=off, 0=on)
    return (fKeyPin & 0x01);

}



//---------------------------------------------------------------------------
//  OLED: Show Start Screen
//---------------------------------------------------------------------------

void  OledShowStartScreen (int iFirmwareVersion_p, int iFirmwareRevision_p, uint8_t ui8DevID_p)
{

char  szOledData[16 - OLED_OFFS_DATA_VALUE + sizeof('\0')];

    //                                                 |0123456789ABCDEF|
    Oled_U8x8_g.clear();
    Oled_U8x8_g.drawString(0, OLED_LINE_DEV_INFO,      "Info:   ?.?? ??H");
    Oled_U8x8_g.drawString(0, OLED_LINE_TEMPERATURE,   "Temp:   --.- C  ");
    Oled_U8x8_g.drawString(0, OLED_LINE_HUMIDITY,      "rH:     --- %   ");
    Oled_U8x8_g.drawString(0, OLED_LINE_PIR_CNT,       "PIR #:  ------  ");
    Oled_U8x8_g.drawString(0, OLED_LINE_LIGHT,         "Light:  --- %   ");
    Oled_U8x8_g.drawString(0, OLED_LINE_CAR_BATT,      "Batt:   --.-- V ");
    Oled_U8x8_g.drawString(0, OLED_LINE_LORA_PACK_CTN, "LoRa #: ------  ");
    Oled_U8x8_g.drawString(0, OLED_LINE_LORA_TM_NXT,   "LoRa T: -:--:-- ");

    memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
    snprintf(szOledData, sizeof(szOledData), "%u.%02u %02uH", iFirmwareVersion_p, iFirmwareRevision_p, (unsigned int)ui8DevID_p);
    OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_DEV_INFO);

    return;

}



//---------------------------------------------------------------------------
//  OLED: Update Process Data
//---------------------------------------------------------------------------

void  OledUpdateProcessData (const tSensorData* pSensorDataRec_p, tSensorData* pPrevSensorDataRec_p)
{

char  szOledData[16 - OLED_OFFS_DATA_VALUE + sizeof('\0')];


    TRACE0("+ 'OledUpdateProcessData()':\n");
    TRACE1("sizeof(szOledData) = %d\n", sizeof(szOledData));


    // update DHT Sensor (Temperature/Humidity)
    if ( CFG_ENABLE_DHT_SENSOR )
    {
        if (pSensorDataRec_p->m_flTemperature != pPrevSensorDataRec_p->m_flTemperature)
        {
            memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
            snprintf(szOledData, sizeof(szOledData), "%.1f C", pSensorDataRec_p->m_flTemperature);
            OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_TEMPERATURE);
        }
        pPrevSensorDataRec_p->m_flTemperature = pSensorDataRec_p->m_flTemperature;

        if (pSensorDataRec_p->m_flHumidity != pPrevSensorDataRec_p->m_flHumidity)
        {
            memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
            snprintf(szOledData, sizeof(szOledData), "%.1f %%", pSensorDataRec_p->m_flHumidity);
            OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_HUMIDITY);
        }
        pPrevSensorDataRec_p->m_flHumidity = pSensorDataRec_p->m_flHumidity;
    }

    // update SEN-HC-SR501 Sensor (IR Motion Sensor)
    if ( CFG_ENABLE_SEN_HC_SR501_SENSOR )
    {
        if (pSensorDataRec_p->m_ui16MotionActiveCount != pPrevSensorDataRec_p->m_ui16MotionActiveCount)
        {
            memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
            snprintf(szOledData, sizeof(szOledData), "%lu", (unsigned long)pSensorDataRec_p->m_ui16MotionActiveCount);
            OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_PIR_CNT);
        }
        pPrevSensorDataRec_p->m_fMotionActive = pSensorDataRec_p->m_fMotionActive;
        pPrevSensorDataRec_p->m_ui16MotionActiveCount = pSensorDataRec_p->m_ui16MotionActiveCount;
    }

    // update Light Sensor (AI0 @ ADS1115)
    if ( CFG_ENABLE_ADS1115_LIGHT_SENSOR )
    {
        if (pSensorDataRec_p->m_ui16AdcValLightLevel != pPrevSensorDataRec_p->m_ui16AdcValLightLevel)
        {
            memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
            snprintf(szOledData, sizeof(szOledData), "%u %%", (uint)(pSensorDataRec_p->m_ui8LightLevel));
            OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_LIGHT);
        }
        pPrevSensorDataRec_p->m_ui16AdcValLightLevel = pSensorDataRec_p->m_ui16AdcValLightLevel;
        pPrevSensorDataRec_p->m_ui8LightLevel        = pSensorDataRec_p->m_ui8LightLevel;
    }

    // update CarBatt AnalogIn (AI1 @ ADS1115)
    if ( CFG_ENABLE_ADS1115_CAR_BATT_AIN )
    {
        if (pSensorDataRec_p->m_ui16AdcValCarBattLevel != pPrevSensorDataRec_p->m_ui16AdcValCarBattLevel)
        {
            memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
            snprintf(szOledData, sizeof(szOledData), "%.1f V", pSensorDataRec_p->m_flCarBattLevel);
            OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_CAR_BATT);
        }
        pPrevSensorDataRec_p->m_ui16AdcValCarBattLevel = pPrevSensorDataRec_p->m_ui16AdcValCarBattLevel;
        pPrevSensorDataRec_p->m_flCarBattLevel         = pSensorDataRec_p->m_flCarBattLevel;
    }

    // update LoRa Packet Count
    if (pSensorDataRec_p->m_ui32LoraPacketCount != pPrevSensorDataRec_p->m_ui32LoraPacketCount)
    {
        memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
        snprintf(szOledData, sizeof(szOledData), "%lu", (unsigned long)pSensorDataRec_p->m_ui32LoraPacketCount);
        OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_LORA_PACK_CTN);
    }
    pPrevSensorDataRec_p->m_ui32LoraPacketCount = pSensorDataRec_p->m_ui32LoraPacketCount;

    // update LoRa Remaining Tx Packet Cycle Time
    if (pSensorDataRec_p->m_i32LoraRemainingCycleTime != pPrevSensorDataRec_p->m_i32LoraRemainingCycleTime)
    {
        memset(szOledData, '\0', 16 - OLED_OFFS_DATA_VALUE + sizeof('\0'));
        snprintf(szOledData, sizeof(szOledData), "%s", FormatDateTime(SensorDataRec_g.m_i32LoraRemainingCycleTime, false, false).c_str());
        OledPrintDataValue(szOledData, sizeof(szOledData), OLED_OFFS_DATA_VALUE, OLED_LINE_LORA_TM_NXT);
    }
    pPrevSensorDataRec_p->m_i32LoraRemainingCycleTime = pSensorDataRec_p->m_i32LoraRemainingCycleTime;


    TRACE0("- 'OledUpdateProcessData()'\n");

    return;

}



//---------------------------------------------------------------------------
//  OLED: Print Process Data
//---------------------------------------------------------------------------

void  OledPrintDataValue (char* pszDataValue_p, int iDataValueBuffSize_p, uint8_t ui8PosX_p, uint8_t ui8PosY_p)
{

int  iIdx;


    // fill-up data value buffer with spaces until end of OLED line to
    // clear (overwrite) obsolete values from previous cycle
    for (iIdx=0; iIdx<(iDataValueBuffSize_p-1); iIdx++)
    {
        if (pszDataValue_p[iIdx] == '\0')
        {
            pszDataValue_p[iIdx] = ' ';
        }
    }
    pszDataValue_p[iIdx] = '\0';

    // print new data values on OLED
    Oled_U8x8_g.drawString(ui8PosX_p, ui8PosY_p, pszDataValue_p);
    TRACE3("x=%d/y=%d -> '%s'\n", ui8PosX_p, ui8PosY_p, pszDataValue_p);

    return;

}



//---------------------------------------------------------------------------
//  Process DHT Sensor (Temperature/Humidity)
//---------------------------------------------------------------------------

int  DhtSensorGetData (float* pflTemperature_p, float* pflHumidity_p)
{

float  flTemperature;
float  flHumidity;
int    iRes;


    iRes = 0;

    flTemperature = DhtSensor_g.readTemperature(false);                 // false = Temp in Celsius degrees, true = Temp in Fahrenheit degrees
    flHumidity    = DhtSensor_g.readHumidity();

    // check if values read from DHT22 sensor are valid
    if ( !isnan(flTemperature) && !isnan(flHumidity) )
    {
        // return valid DHT22 sensor values
        *pflTemperature_p = flTemperature;
        *pflHumidity_p    = flHumidity;
    }
    else
    {
        *pflTemperature_p = 0;
        *pflHumidity_p    = 0;
        iRes = -1;
    }

    return (iRes);

}



//---------------------------------------------------------------------------
//  Process SEN-HC-SR501 Sensor (IR Motion Sensor)
//---------------------------------------------------------------------------

int  HcSr501SensorGetData (bool* pfMotionActive_p, uint16_t* pui16MotionActiveTime_p)
{

bool      fMovementActive;
uint32_t  ui32CurrTick;
uint16_t  ui16TickCountMotionSegm;
uint16_t  ui16MotionActiveTime;
int       iRes;


    iRes = 0;

    fMovementActive = digitalRead(PIN_SEN_HC_SR501);

    // movement detected?
    if ( fMovementActive )
    {
        // start of new movement detected?
        if (ui32StartTickMotionSegm_g == 0)
        {
            // start new movement segment
            ui32StartTickMotionSegm_g = millis();
            ui16TickCountMotionSegm = 0;
        }
        else
        {
            // continue a running movement segment
            ui32CurrTick = millis();
            ui16TickCountMotionSegm = (ui32CurrTick - ui32StartTickMotionSegm_g) / 1000;
        }
    }
    else
    {
        // end of expired movement detected?
        if (ui32StartTickMotionSegm_g != 0)
        {
            // terminate movement segment
            ui32CurrTick = millis();
            ui16TickCountMotionSegm = (ui32CurrTick - ui32StartTickMotionSegm_g) / 1000;
            ui16MotionActiveTimeBase_g += ui16TickCountMotionSegm;
            ui32StartTickMotionSegm_g = 0;
        }
        ui16TickCountMotionSegm = 0;
    }

    ui16MotionActiveTime = ui16MotionActiveTimeBase_g + ui16TickCountMotionSegm;

    *pfMotionActive_p = fMovementActive;
    *pui16MotionActiveTime_p = ui16MotionActiveTime;

    return (iRes);

}



//---------------------------------------------------------------------------
//  Reset MotionActiveTime for SEN-HC-SR501 Sensor (IR Motion Sensor)
//---------------------------------------------------------------------------

void  HcSr501SensorRestMotionActiveTime ()
{

    ui16MotionActiveTimeBase_g = 0;
    return;

}



//---------------------------------------------------------------------------
//  Manage Pause Status for SEN-HC-SR501 Sensor (IR Motion Sensor)
//---------------------------------------------------------------------------

void  HcSr501SensorStartPause ()
{

    ui32StartTickPauseSr501_g = millis();
    fPauseActiveSr501_g       = true;

    return;

}

//---------------------------------------------------------------------------

bool  IsPausedHcSr501Sensor ()
{

uint32_t  ui32CurrTick;
bool      fMovementActive;


    // Is pause status active at all?
    if ( !fPauseActiveSr501_g )
    {
        // pause status is inactive
        return (false);
    }

    // If pause status <fPauseActiveSr501_g> is active, then check if <SEN_HC_SR501_PAUSE_PERIOD> is elapsed
    ui32CurrTick = millis();
    if ((ui32CurrTick - ui32StartTickPauseSr501_g) < SEN_HC_SR501_PAUSE_PERIOD)
    {
        // time period <SEN_HC_SR501_PAUSE_PERIOD> is not elapsed -> continue pause statue
        return (true);
    }

    // If pause status <fPauseActiveSr501_g> is active and <SEN_HC_SR501_PAUSE_PERIOD> is elapsed,
    // then check if <PIN_SEN_HC_SR501> is active
    fMovementActive = digitalRead(PIN_SEN_HC_SR501);
    if ( fMovementActive )
    {
        // <PIN_SEN_HC_SR501> is active -> continue pause status until <PIN_SEN_HC_SR501> changes to inactive
        // (leave pause status only when IR Motion Sensor is inactive)
        return (true);
    }

    // If <SEN_HC_SR501_PAUSE_PERIOD> is elapsed and <PIN_SEN_HC_SR501> is inactive,
    // then leave pause status
    ui32StartTickPauseSr501_g = 0;
    fPauseActiveSr501_g       = false;

    return (false);

}



//---------------------------------------------------------------------------
//  Process Light Sensor (AI0 @ ADS1115)
//---------------------------------------------------------------------------

int  GetAds1115LightSensorData (uint16_t* pui16AdcVal_p, uint8_t* pui8LightLevel_p)
{

int16_t  i16AdcVal;
uint     uiLightLevel;
int      iRes;


    iRes = 0;

    i16AdcVal = Ads1115_g.readADC_SingleEnded(ADC_CH_LIGHT_SENSOR);
    if (i16AdcVal < 0)
    {
        i16AdcVal = 0;
    }

    // an ADC output value of 16384 corresponds to an input voltage of 3V,
    // which in turn corresponds to an ambient light level of 100%
    uiLightLevel = ((uint)i16AdcVal * 100) / 16384;     // get ambient light level in [%]

    *pui16AdcVal_p = (uint16_t)i16AdcVal;
    *pui8LightLevel_p = (uint8_t)uiLightLevel;

    return (iRes);

}



//---------------------------------------------------------------------------
//  Process Car Battery Level (AI1 @ ADS1115)
//---------------------------------------------------------------------------

int  GetAds1115CarBattAinData (uint16_t* pui16AdcVal_p, float* pflCarBattLevel_p)
{

int16_t  i16AdcVal;
float    flVoltage;
float    flCarBattLevel;
int      iRes;


    iRes = 0;

    i16AdcVal = Ads1115_g.readADC_SingleEnded(ADC_CH_CAR_BATT_AIN);
    if (i16AdcVal < 0)
    {
        i16AdcVal = 0;
        iRes = -1;
    }
    flVoltage = (i16AdcVal * ADS1115_DIGIT)/1000;                       // convert ADC digits into native voltage level in [V]

    flCarBattLevel = flVoltage * ADC_CAR_BATT_VOLTAGE_DIVIDER;          // normalize voltage level to real input value (Input Voltage Divider = 1:4.78 -> FullScale = 3.3V * 4.78 = 15.78V)

    // limit float value to 1 decimal place
    flCarBattLevel = flCarBattLevel + 0.05;
    flCarBattLevel = (float) ((int)(flCarBattLevel*10));
    flCarBattLevel = flCarBattLevel/10;

    // Is CarBatt Level in valid range?
    if (flCarBattLevel > ADC_CAR_BATT_VALID_LEVEL_MIN)
    {
        // compensate voltage drop caused by forward voltage across the reverse polarity protection diode D2
        flCarBattLevel += ADC_CAR_BATT_RPD_FORWARD_VOLTAGE;
    }
    else
    {
        // reset CarBatt Level if no Batt connected (invalid values)
        flCarBattLevel = 0.0;
        iRes = -1;
    }

    *pui16AdcVal_p = (uint16_t)i16AdcVal;
    *pflCarBattLevel_p = flCarBattLevel;

    return (iRes);

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   G E N E R I C   F U N C T I O N S              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Get System Uptime
//---------------------------------------------------------------------------

String  GetSysUptime (uint32_t* pui32Uptime_p)
{

uint32_t  ui32Uptime;
String    strUptime;


    ui32Uptime = millis();
    strUptime = FormatDateTime(ui32Uptime, false, true);

    *pui32Uptime_p = ui32Uptime;
    return (strUptime);

}



//---------------------------------------------------------------------------
//  Format Date/Time
//---------------------------------------------------------------------------

String  FormatDateTime (uint32_t ui32TimeTicks_p, bool fForceDay_p, bool fForceTwoDigitsHours_p)
{

const uint32_t  MILLISECONDS_PER_DAY    = 86400000;
const uint32_t  MILLISECONDS_PER_HOURS  = 3600000;
const uint32_t  MILLISECONDS_PER_MINUTE = 60000;
const uint32_t  MILLISECONDS_PER_SECOND = 1000;


char      szTextBuff[64];
uint32_t  ui32TimeTicks;
uint32_t  ui32Days;
uint32_t  ui32Hours;
uint32_t  ui32Minutes;
uint32_t  ui32Seconds;
String    strDateTime;


    ui32TimeTicks = ui32TimeTicks_p;

    ui32Days = ui32TimeTicks / MILLISECONDS_PER_DAY;
    ui32TimeTicks = ui32TimeTicks - (ui32Days * MILLISECONDS_PER_DAY);

    ui32Hours = ui32TimeTicks / MILLISECONDS_PER_HOURS;
    ui32TimeTicks = ui32TimeTicks - (ui32Hours * MILLISECONDS_PER_HOURS);

    ui32Minutes = ui32TimeTicks / MILLISECONDS_PER_MINUTE;
    ui32TimeTicks = ui32TimeTicks - (ui32Minutes * MILLISECONDS_PER_MINUTE);

    ui32Seconds = ui32TimeTicks / MILLISECONDS_PER_SECOND;

    if ( (ui32Days > 0) || fForceDay_p )
    {
        snprintf(szTextBuff, sizeof(szTextBuff), "%ud/%02u:%02u:%02u", (uint)ui32Days, (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
    }
    else
    {
        if ( fForceTwoDigitsHours_p )
        {
            snprintf(szTextBuff, sizeof(szTextBuff), "%02u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
        else
        {
            snprintf(szTextBuff, sizeof(szTextBuff), "%u:%02u:%02u", (uint)ui32Hours, (uint)ui32Minutes, (uint)ui32Seconds);
        }
    }
    strDateTime = String(szTextBuff);

    return (strDateTime);

}





//=========================================================================//
//                                                                         //
//          D E B U G   F U N C T I O N S                                  //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  DEBUG: Dump Buffer
//---------------------------------------------------------------------------

#ifdef DEBUG_DUMP_BUFFER

void  DebugDumpBuffer (String strBuffer_p)
{

int            iBufferLen = strBuffer_p.length();
unsigned char  abDataBuff[iBufferLen];


    strBuffer_p.getBytes(abDataBuff, iBufferLen);
    DebugDumpBuffer(abDataBuff, strBuffer_p.length());

    return;

}

//---------------------------------------------------------------------------

void  DebugDumpBuffer (const void* pabDataBuff_p, unsigned int uiDataBuffLen_p)
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

#endif




// EOF
