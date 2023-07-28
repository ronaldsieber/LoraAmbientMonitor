# LoraAmbientMonitor

This Arduino project is part of the main project [LoraAmbientMonitor](https://github.com/ronaldsieber/LoraAmbientMonitor) and includes the ESP32 firmware for the sensor device based on the [LoRaAmbientMonitor_PCB](https://github.com/ronaldsieber/LoRaAmbientMonitor_PCB) hardware project. The sensor device provides the following environmental data:

- Temperature (DHT22)
- Humidity (DHT22)
- Movements in the room (PIR sensor HC-SR501)
- Ambient brightness (ALS-PDIC243 @ ADS1115)
- Optional: voltage of car start battery (ADS1115)

All ambient data provided by the sensor device is transmitted to the receiver device described in the [LoraPacketRecv](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketRecv) subproject by using LoRa data packets.

The ESP32 module of the type *"Heltec ESP32 WiFi LoRa OLED"* used for the *LoraAmbientMonitor* already contains the LoRa chipset SX1276 as well as a 0.96" OLED display which is used for local display of the sensor values.

The stick antenna included with the *Heltec ESP32 Kit* is well suited for development and short distances due to its compact and space-saving design. To increase the range, it is better to use an external antenna (6dBi or better) remotely with coaxial cable in productive use.

![\[Project Overview - LoraAmbientMonitor\]](../Documentation/Project_Overview_LoraAmbientMonitor.png)

## Device Runtime Configuration

The device runtime configuration settings are made using the 4-way DIP switch SW2:

- **DIP3/4 = Node-ID:** The The Node- or DevID is embedded in the LoRa data packets and thus enables the receiver to assign the received packets to the respective source. This makes it possible to use the same firmware unchanged for several boards at the same time. The respective DevID of the board is shown in the OLED display.

- **DIP2 = Pause SEN-HC-SR501 (IR Motion Sensor):** Depending on the adjusted sensitivity, the motion sensor will also falsely trigger during the transmission of LoRa packets due to interference from the LoRa module. To prevent false triggering, the signal output of the motion detector is ignored during LoRa transmission for the time interval configured by means of `SEN_HC_SR501_PAUSE_PERIOD`.

- **DIP1 = Generate asynchronous LoRa Transmit Events:** Normally, sensor data is transmitted cyclically, typically every hour. If asynchronous LoRa send events are enabled, asynchronous LoRa packets are sent when the motion detector is triggered or when the vehicle start battery is connected or disconnected, so that the current information is immediately available on the receiver side. However, asynchronous LoRa packets are sent at the earliest after the time interval configured using `LORA_PACKET_INHIBIT_TIME` has elapsed. Detailed information about the LoRa time regime is described in the section *"LoRa Configuration Section"* below.

Especially for commissioning and diagnostics the CFG Button allows to shorten the LoRa transmission intervals (*"Commissioning Mode"*):

- **CFG Button:** If the CFG Button is pressed during boot-up (power-on or reset), the sensor module uses shortened transmission intervals. The associated more frequent transmission of data packets facilitates diagnostics in the event of an error. In addition, measures such as changing or aligning the antenna position can be evaluated more quickly.

When *"Commissioning Mode"* is activated, the green LED flashes permanently to indicate shortened LoRa transmission intervals.

## Status Indicators

The *LoraAmbientMonitor* Sensor Device uses 3 on-board LEDs with the following meanings as well as the OLED display:

- **LED Green:** Steady light of several seconds = sending a LoRa packet, permanently blinking = board in *"Commissioning Mode"* with shortened LoRa sending intervals.

- **LED Red:** Continuous light of several seconds = Triggering of the SEN-HC-SR501 (IR Motion Sensor)

- **LED Yellow:** Connected car start battery detected

- **OLED Display:** Display of device parameters and current sensor values

## LoRa Bootup Packet

The bootup packet is sent by the *LoraAmbientMonitor* once at the end of the sketch `setup()` function. It contains various information about the device firmware (version number, configuration), as well as the runtime configuration made by means of 4-way DIP switches. The detailed structure is described by the `tLoraBootupHeader` structure in the ***<LoraPacket.h>*** header file. The bootup header is protected by an additional 16Bit CRC of type `uint16_t`. This results in a total size of 80Bit (= 10 bytes).

Each LoRa data packet is defined by the structure `tLoraDataPacket`. In the case of the bootup packet, however, only the header with the structure `tLoraBootupHeader` is used. The following three elements of type `tLoraDataRec` intended for sensor data are completely filled with zero. In total, the bootup packet has a payload length of 40 bytes.

The encoding of the configuration data into the over-the-air format is done in the method `LoraPayloadEncoder:: EncodeTxBootupPacket()`.

## LoRa Sensor Data Packet

The sensor data packets are sent periodically by the *LoraAmbientMonitor* within the sketch `loop()` function. For this purpose the sensor data is transmitted in a compact 64Bit field (`uint64_t`) defined as a structure of type `tLoraDataRec`. This sensor data record is protected by an additional 16Bit CRC of the type `uint16_t`. This results in a total size of 80Bit (= 10 bytes) for a data set including CRC.

Each LoRa data packet is defined by the structure `tLoraDataPacket`. It contains a header of type `tLoraDataHeader` followed by three generations of sensor data each of type `tLoraDataRec` (as `kLoraPacketDataGen0`, `kLoraPacketDataGen1` and `kLoraPacketDataGen2`). Header and sensor data each comprise 64bit plus a 16bit CRC. The total size of a LoRa packet thus has a payload length of 40 bytes (4 * (64bit + 16bit) = 4 * 80bit = 320bit). The header file **<LoraPacket.h>** defines the structure of a LoRa packet in detail, the document **<LoRa_Payload_Design.pdf>** clarifies its schematic structure.

The encoding of the sensor data into the over-the-air format is done in the method `LoraPayloadEncoder:: EncodeTxDataPacket()`.

## Sensor Data Average Value

Due to the regulatory requirements for the duty cycle for using the 868 MHz band (max. 1% channel occupancy), the sensor data packets are only transmitted at longer intervals (typically every hour). In order to also take into account the trend development between the transmission times, a moving average is formed over the sensor data (temperature, humidity, CarBattLevel). For this purpose, the Simple Moving Average filter from the project [SimpleMovingAverage](https://github.com/ronaldsieber/SimpleMovingAverage) is used. The value transmitted in a LoRa data packet is therefore not the current sensor value at the time of transmission, but the average value over the data series of the respective sensor defined by `SMA_DHT_SAMPLE_WINDOW_SIZE` and `SMA_CARBATT_SAMPLE_WINDOW_SIZE`.

## Application Configuration Section

At the beginning of the sketch there is the following configuration section:

    const int   LORA_TX_POWER                 = 20;
    const int   LORA_SPREADING_FACTOR         = 12;
    const long  LORA_SIGNAL_BANDWIDTH         = 125E3;
    const int   LORA_CODING_RATE_DENOMINATOR  = 5;

A additional parameter set defines the transmission time regime, which is particularly important for maintaining the duty cycle for the use of the 868 MHz band (max. 1% channel occupancy):

    LORA_PACKET_FIRST_TIME
Defines the time between the bootup packet and the first sensor data packet (resp. `LORA_PACKET_FIRST_TIME_COMM_MODE` in *"Commissioning Mode"* with active CFG Button).

    LORA_PACKET_CYCLE_TIME
Defines the time between two consecutive sensor data packets (starting with the 2nd packet) (resp. `LORA_PACKET_CYCLE_TIME_COMM_MODE` in *"Commissioning Mode"* with active CFG Button).

    LORA_PACKET_INHIBIT_TIME
Defines the minimum time between two consecutive packets, is only relevant when activating asynchronous LoRa transmit events (DIP1) and specifies the inhibit time by which an asynchronous event packet is delayed after the last cyclic sensor data packet has been sent in order to respect the duty cycle.

To minimize mutual interference of multiple devices, the transmit interval between two consecutive packets is varied by a random value in the range +/- 5% (`LoraTransmitter::CalcNextTransmitCycleTime()`).

## Runtime Outputs in Serial Terminal Window

During runtime all relevant information is output in the serial terminal window (115200Bd). Especially during the system start (Sketch function `setup()`) error messages are also displayed here, which may be due to a faulty software configuration. These messages should be observed in any case, especially during the initial startup.

In the main loop of the program (Sketch function `main()`) the values of all sensors are displayed cyclically. In addition again messages inform about possible problems with the access to the sensors.

By activating the line `#define DEBUG` at the beginning of the sketch further, very detailed runtime messages are displayed. These are very helpful especially during program development or for debugging. By commenting out the line `#define DEBUG` the outputs are suppressed again.

## Third Party Components used

1. **LoRa Radio**
The following driver library is used for the on-board LoRa chipset SX1276 of the *"Heltec ESP32 WiFi LoRa OLED"* module:
https://github.com/sandeepmistry/arduino-LoRa
The installation is done with the Library Manager of the Arduino IDE.

2. **OLED Display**
For the on-board OLED display of the *"Heltec ESP32 WiFi LoRa OLED"* module the following driver library is used (U8x8lib):
https://github.com/olikraus/u8g2
The installation is done with the Library Manager of the Arduino IDE.

3. **ADC ADS1115**
For the ADS1115 type ADC, the driver library from Adafruit is used:
https://github.com/adafruit/Adafruit_ADS1X15
The installation is done with the Library Manager of the Arduino IDE.

4. **DHT Sensor**
For the DHT sensor (temperature, humidity) the driver library of Adafruit is used. The installation is done with the Library Manager of the Arduino IDE.



