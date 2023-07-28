# LoraAmbientMonitor

The *LoraAmbientMonitor* project aims to monitor environmental data in outbuildings such as car garages and transmit it to the main building using LoRa radio technology. The LoRa communication takes place directly point-to-point between transmitter and receiver (device-to-device), which limits the achievable distance to a few 100 meters.

In the *LoraAmbientMonitor* project, the following environmental data are collected:
- Temperature
- Humidity
- Movements in the room (PIR sensor)
- Ambient brightness
- Optional: voltage of the car start battery

The field of application of the *LoraAmbientMonitor* project is comparable to that of the [ESP32SmartBoard_MqttSensors](https://github.com/ronaldsieber/ESP32SmartBoard_MqttSensors) or [ESP32SmartBoard_MqttSensors_BleCfg](https://github.com/ronaldsieber/ESP32SmartBoard_MqttSensors_BleCfg) projects. The latter two projects use WLAN for data transmission, thus achieving ranges of several 10 meters and are thus well suited for monitoring living spaces within a building. Ambient data is recorded and transmitted every minute. In contrast, the *LoraAmbientMonitor* project described here relies on LoRa radio technology for data transmission and thus achieves ranges of several 100 meters, but at the cost of significantly lower acquisition and transmission rates at hourly intervals. Thus, the *LoraAmbientMonitor* project extends the application radius of the two *ESP32SmartBoard* projects from the immediate residential area to external outbuildings in the middle environment.

## Project Overview

![\[Project Overview\]](Documentation/Project_Overview.png)

The *LoraAmbientMonitor* project includes the following components:

**LoraAmbientMonitor**
- ESP32 based board with sensors to collect environmental data (temperature, humidity, motion detector, ambient brightness, voltage of the car start battery)
- Hardware: see project [LoRaAmbientMonitor_PCB](https://github.com/ronaldsieber/LoRaAmbientMonitor_PCB)
- Software: Embedded C++ Arduino project in the directory [LoRaAmbientMonitor](LoRaAmbientMonitor)

![\[LoraAmbientMonitor\]](Documentation/LoraAmbientMonitor_small.png)

**LoraPacketRecv**
- RaspberryPi based board with LoRa GPS HAT.
- Gateway that receives LoRa packets sent by one or more *LoraAmbientMonitor* sensor modules, decodes their payload and publishes the data as JSON record via MQTT to a central broker
- Software: C++ RaspberryPi/Linux project in the directory 
[LoraPacketRevc](LoraPacketRevc)

![\[LoraPacketRecv\]](Documentation/LoraPacketRecv_small.png)

**LoraPacketViewer**
- Windows/GUI application to observe the environment data transmitted by one or more *LoraAmbientMonitor*, especially for commissioning and diagnostics.
- Receives the JSON records transmitted from LoraPacketRecv to the broker via MQTT Subscribe.
- Software: C#/.NET WindowsForm project in the directory 
[LoraPacketViewer](LoraPacketViewer)

![\[LoraPacketViewer\]](Documentation/LoraPacketViewer_small.png)

The further processing of the sensor module data transmitted to the MQTT broker (JSON records) is not part of this project. The monitoring with the *LoraPacketViewer* is primarily only used for commissioning and diagnostics, but it does not enable any evaluation and is not designed for the permanent storage of environmental data. In productive use, the data can be received here from the MQTT broker via a Node-RED Flow, for example, and written to an InfluxDB database. From there they can be read out, evaluated and visualized via Grafana. However, the scope of this project ends with the transfer of the data to the MQTT broker by *LoraPacketRecv*. Storage in a database and further analysis are outside the scope of this project (highlighted in gray in the project overview above).

As shown in the project overview above, direct LoRa Device-to-Device communication between the sensor modules and the RaspberryPi based *LoraPacketRecv* receiver is used. This uses a proprietary protocol described in detail below. The use of LoRaWAN as a protocol was deliberately avoided in this project, since there are no LoRaWAN gateways already installed in the area of the planned application. Due to the direct LoRa Device-to-Device communication, the project can be implemented wherever there is direct radio contact between transmitter and receiver modules, regardless of the existing LoRaWAN infrastructure.

## LoRa Radio Transmission

The most critical pat of the overall system is the radio transmission of data via LoRa. A radio link is per se highly susceptible to errors, as it is exposed to numerous sources of interference (shared medium: multiple stations can all transmit simultaneously and thus interfere with each other, electromagnetic interference from the environment, etc.).

The following factors have a significant impact on the resilience of data transmitted by radio to interference factors:

- **Short Transmission Time**
The shorter the transmission time of a data packet over-the-air, the less likely it is that collisions with other packets or electromagnetic interference from the environment will occur during transmission. A short transmission time is achieved in particular by a low payload volume (data economy) and a high data rate (transmission speed).

- **High Energy Density**
The higher the energy density of the transmitted data, the better the receiver can extract the data from the general noise floor (signal-to-noise ratio). Accordingly, this also leads to an increase in the distance that can be covered by radio (range).

The antennas used also have a high influence on the achievable quality of the radio transmission. These should be optimally tuned to the 868 MHz frequency band (antenna gain), the transmitting and receiving antennas should be aligned harmoniously with each other and, in the best case, there should be a direct line of sight between them without any barriers.

The stick antennas supplied with the Heltec ESP32 Kit (Sensor Module) and the LORA PI HAT (Receiver) are only suitable for bridging short distances. To increase the range, it is better to use an external antenna (6dBi or better) on both sides with coaxial cable in productive use.

The configuration parameters relevant for LoRa transmission are defined on the transmitter side in [LoRaAmbientMonitor.ino](LoRaAmbientMonitor/LoRaAmbientMonitor.ino):

- **Spreading Factor** (`LORA_SPREADING_FACTOR`)
The spreading factor is a measure that indicates in how many individual steps (chips) a data symbol is transmitted. With a small spreading factor, the data symbols are encoded with few chips, which ultimately leads to a short transmission time of a data packet over-the-air. This reduces the probability that the data packet will be disturbed by external influences during transmission. Conversely, a large spreading factor causes a data symbol to be encoded with a high number of chips and thus stretched out in time. As a result, each data symbol is transmitted with a higher energy density, which in turn leads to increased interference immunity due to a better signal-to-noise ratio. In general, a higher spreading factor results in a longer range, but also in a higher probability of interference due to the longer over-the-air transmission time. Since the two aspects influence each other in opposite directions, it can be useful in the case of frequent transmission errors to determine the optimum value for the respective local conditions empirically.

- **Signal Bandwidth** (`LORA_SIGNAL_BANDWIDTH`)
The signal bandwidth is a measure for the frequency deviation with which the data symbols are encoded. Similar to the spreading factor, the signal bandwidth also affects the time required to transmit a data packet. Increasing the signal bandwidth causes a reduction in the transmission time of a data packet. This reduces the probability that the data packet will be disturbed by external influences during over-the-air transmission. In contrast, a reduction in signal bandwidth causes each data element to be stretched in time during transmission. This in turn results in higher noise immunity due to a better signal-to-noise ratio. Since the two aspects are opposed to each other, it may be advisable to empirically determine the optimum value for the respective local conditions in the case of frequent transmission errors.

- **Transmit Power** (`LORA_TX_POWER`)
A high transmit power results in a high signal-to-noise ratio, thus a high resilience to interference factors and ultimately also leads to a high achievable transmission range. However, high transmit power means uncooperative behavior towards other users. For this reason, the maximum transmit power is therefore limited by regulation to 20 dB.

In addition to the transmission parameters listed, the amount of data to be transmitted (payload volume) is of fundamental importance. A large payload volume requires a correspondingly long occupancy of the transmission channel, which is not available to other users during this time (shared medium). At the same time, a long over-the-air transmission increases susceptibility to interference. Therefore, the payload volume should be as small as possible.

For this reason, the individual data elements on the transmitter side are converted into a format that is as compact as possible. For example, temperature is transmitted in 0.5° steps only (instead of the 0.1° resolution of the sensor), humidity only as an integer value, ambient brightness only in 2% steps, etc. Due to the reduced resolution it is possible to store the complete sensor data record including minimal header information in a 64bit field of type `uint64_t`. The sensor data record is secured by an additional 16Bit CRC of type `uint16_t`. This results in a total size of 80 bits (= 10 bytes) for a data record including CRC.

To increase the probability of at least one successful reception, each sensor data record is transmitted in 3 consecutive LoRa packets (with a time interval of `LORA_PACKET_CYCLE_TIME` seconds). The data set marked with *kLoraPacketDataGen0* contains the current data at the time of transmission, followed by *kLoraPacketDataGen1* with the data from the previous time of transmission and *kLoraPacketDataGen2* with the data from the second to last time of transmission. Thus, even if 2 consecutive LoRa packets are lost, all data can still be reconstructed without gaps.

To be able to detect packet losses on the receiver side, each LoRa sensor data packet contains a consecutive 24Bit sequence number (`m_ui24SequNum`) in its header. When received without errors, this sequence number forms a strictly monotonically increasing sequence. A packet loss leads to jumps on the receiver side.

![\[LoRa Packet Scheme\]](Documentation/LoRa_Packet_Scheme.png)

A LoRa packet has a payload length of 40 bytes. It consists of a packet header and the above-mentioned 3 generations of sensor data records (Gen0/Gen1/Gen2). Each of these four elements is secured with its own 16Bit CRC of type `uint16_t` and has a length of 10 bytes. To decode a sensor data record, the CRC of the header and the CRC of the respective sensor data record (Gen0, Gen1 or Gen2) must be intact. This segmentation allows individual sensor record sets to be extracted from a LoRa packet even if the packet has partial transmission defects. This increases resilience to interference on the radio link. The detailed design of a complete LoRa packet is described in [LoRa_Payload_Design.pdf](Documentation/LoRa_Payload_Design.pdf).

The *LoraAmbientMonitor* project is designed to collect and transmit environmental data in parallel from up to 16 sensor modules. If several modules transmit at the same time, this leads to collisions on the radio link and thus usually to the loss of the data packets. Since a uniform firmware version is typically used for all devices, all devices also use the same transmission time regime based on the time constants `LORA_PACKET_INHIBIT_TIME`, `LORA_PACKET_FIRST_TIME` and `LORA_PACKET_CYCLE_TIME` (all defined in [LoRaAmbientMonitor.ino](LoRaAmbientMonitor/LoRaAmbientMonitor.ino)). To prevent the identical configuration from leading to permanent, systematic transmission collisions, the respective transmission time is varied by a random value in the range +/- 5%.

In sum, the overall result of the constants described here must satisfy the regulatory requirements for the duty cycle for use of the 868 MHz band. Here it is specified that a transmitter may only occupy the frequency band for a maximum of 1% of the time in order not to interfere with cooperative use by other subscribers. This means that the *LoraAmbientMonitor* may only use the radio channel for a maximum of 36 seconds per hour. As discussed, both transmit parameters and payload length have a significant impact on the time required for over-the-air transmission of the LoRa packet. This can be determined based on the individual parameters using the *"LoRa Air time calculator"*: https://loratools.nl/#/airtime.

## Structure of the LoRa Packets

The structure of LoRa packets for over-the-air transmission is described in the header file [LoraPacket.h](LoraAmbientMonitor/LoraAmbientMonitor/LoraPacket.h), which must be identical for sender (*LoRaAmbientMonitor*) and receiver (*LoraPacketRecv*).

Note: Since the Arduino IDE cannot process files outside the Sketch directory, it is not possible to place the header file *LoraPacket.h* centrally in a common include directory for both subprojects *LoRaAmbientMonitor* and *LoraPacketRecv* (see https://stackoverflow.com/questions/68733232/define-a-relative-path-to-h-file-c-arduino). Instead, both subprojects each use separate copies of *LoraPacket.h*.

The complete structure of an over-the-air transmitted data packet with 40 bytes length is defined by the structure tLoraDataPacket. It contains a header and as described above 3 generations of sensor data records (Gen0/Gen1/Gen2).

As the first packet after the system, a bootup packet is first sent once, whose packet header is defined by the structure `tLoraBootupHeader`. The 3 segments of type `tLoraDataRec` contain no user data and are completely filled with zero. The bootup packet describes the current device configuration (parameterization by 4-way DIP switch SW2, firmware version, static firmware configuration, LoRa transmit parameters).

All subsequent LoRa packets consist of a packet header (`tLoraDataHeader`) and the 3 generations of sensor data records (Gen0/Gen1/Gen2) defined by the type `tLoraDataRec` as described above.

On the transmitter side (*LoRaAmbientMonitor*), the encoding of the native sensor data into the `tLoraDataPacket` structure used for over-the-air transmission is done by the `LoraPayloadEncoder` class. The complement for the decoding on the receiver side (*LoraPacketRecv*) is the class `LoraPayloadDecoder`.

![\[LoRa Payload Encode/Decode\]](Documentation/LoRa_Payload_Encode_Decode.png)

## Timestamp Generation for Sensor Data Records

The ESP32 of the *LoRaAmbientMonitor* provides only a relative uptime, i.e. the number of seconds since the system start. An absolute real-time with date and time (RTC) is not available.

The complete uptime is stored as 32bit value in the header of a LoRa packet (2\^32 seconds => 136 years). It defines the transmission time of the LoRa packet on the LoRaAmbientMonitor. Each sensor record contains a shortened form of the uptime as a 12bit value in 10 second steps (2^12 = 4096 * 10 seconds => 11 hours).

The realtime timestamp with date and time is only generated on the receiver side (RaspberryPi) by the *LoraPacketRevc*. For this purpose the localtime of the RaspberryPi at the time of LoRa data reception is filled in the data packet of type `tLoraMsgData` (`tLoraMsgData.m_tmTimeStamp`). This receive timestamp is immediately taken as the timestamp for the Gen0 sensor data record. For the historical sensor records Gen1 and Gen2 the timestamp is reconstructed by reducing the current receive timestamp by the uptime difference between Gen0 and Gen1 or between Gen0 and Gen2.

## Best Practice (1): Mounting the LoRaAmbientMonitor Sensor Device

For optimal monitoring of the environment, the sensor module should be positioned in the room so that the motion detector (PIR sensor) directly detects the opening or closing of the garage door and temperature and humidity are measured approximately in the center of the room. For this purpose, an ABS plastic plate (approx. 16x20cm) is suitable, on which 4 mounting brackets (with oblong hole) are arranged in such a way that the sensor module can be clamped between them. The screws for the mounting brackets fix at the same time a flexible mounting tape on the back side of the plate. This can be easily bent and pushed through between the ceiling joist and the ceiling panel. The bending angle of the mounting strip allows the sensor module to be optimally aligned in the room so that it hangs almost vertically from the ceiling.

A rod antenna (6 dBi) offset with coaxial cable is used to transmit the data packets. This allows flexible wall/ceiling mounting and optimal alignment independent of the sensor module.

![\[LoraAmbientMonitor_Mounted\]](Documentation/LoraAmbientMonitor_Mounted.png)

## Best Practice (2): Commissioning the System

From a purely software perspective, commissioning the system is relatively trivial. However, the optimal positioning and alignment of the antennas poses a certain challenge. For this purpose, it is advisable to perform several tests at different locations in the room. To obtain feedback on the reception result directly on site when positioning the transmitting antenna, it helps to display the telemetry data on a mobile device (smartphone). For this purpose, a publicly available MQTT broker is temporarily used, which can also be accessed from the smartphone via a mobile internet connection (e.g. public.mqtthq.com):

**Step 1 - Start LoraPacketRecv (Receiver)**

The LoraPacketRecv software is instructed with the command line parameters *"-h=<host>"* and *"-t"* to connect to a publicly available broker and send it compact telemetry data packets well suited for display on the smartphone:

    sudo ./LoraPacketRecv -h= public.mqtthq.com -t

**Step 2 - Launch MQTT Client App on Mobile Device**

An MQTT client app is required on the mobile device, such as *"MyMQTT"*. This app also needs to be connected to the same publicly available broker as *LoraPacketRecv*. Then the topic *"LoraAmbMon/Status/Telemetry"* has to be subscribed to. This will display the telemetry data generated by *LoraPacketRecv* when it receives a LoRa packet. These contain the following information, among others:

- Sequence number: consecutive packet number, missing numbers are an indication of loss of the packets in question.

- RSSI value: signal strength of the received packet, the larger the RSSI value, the better the quality of the received signal.
Note: RSSI values are negative numbers, so the smaller the number value, the better the signal strength (-30dBi is better than -50dBi).

![\[MyMQTT_App\]](Documentation/MyMQTT_App.png)

**Step 3 - Start Sensor Module**

Especially for commissioning and diagnostics the *"Commissioning Mode"* is provided, which works with shortened LoRa transmission intervals. By sending the LoRa packets more frequently, actions like changing the antenna position or orientation can be evaluated faster.

To start the sensor module in *"Commissioning Mode"*, the CFG button must be pressed during boot-up (power-on or reset). The *"Commissioning Mode"* is indicated by a permanent flashing of the green LED.

By observing the RSSI value output on the mobile device together with the telemetry data, different positions and orientations of the transmitting antenna can now be conveniently tried out and evaluated.

Note: In addition to the telemetry data, all other JSON records with bootup and sensor data packets are still transmitted to the MQTT broker. For the time of the connection to a public broker, the complete environment data is thus also publicly accessible.

## Best Practice (3): Productive Use

For productive use, one or more *LoraAmbientMonitor* sensor modules and a RaspberryPi with the *LoraPacketRecv* gateway software as receiver are required. The Windows application *LoraPacketViewer* supports commissioning and diagnostics if required, but is not necessary for productive use itself.

- LoRaAmbientMonitor Sensor Assembly:
When using multiple *LoraAmbientMonitor* sensor modules, make sure that an individual Node or DevID is configured for each module at DIP switches DIP3/4. DIP2 optionally ensures that the motion detector is ignored during the transmission of the LoRa packets in order to suppress false triggering due to interference from the LoRa transmitter. DIP1 can be used to control whether, in addition to the cyclic LoRa packets, asynchronous packets are also sent in the case of special events such as triggering the motion detector or connecting or disconnecting the car start battery.

- LoraPacketRecv Gateway:
The gateway software should be started automatically when booting the RasperryPi to ensure high availability. The scripts contained in the subdirectory *"Autostart"* are responsible for the start/stop handling. The *"LoraPacketRecv"* script is used to configure the autostart of the gateway software. Details are described in the section *"Autostart for LoraPacketRecv"* in the [LoraAmbientMonitor_LoraPacketRecv.md](Documentation/LoraAmbientMonitor_LoraPacketRecv-md) documentation.

By providing the sensor data packet in the form of JSON records via MQTT, a variety of further processing and analysis is possible with numerous free software packages. For example, the *"Eclipse Mosquitto MQTT Broker"* can be installed directly on the same RaspberryPi as the *LoraPacketRecv* gateway. Further software packages for further processing and evaluation are *Node-RED*, *InfluxDB* and *Grafana*. It is irrelevant whether the packages also run on the RaspberryPi of the *LoraPacketRecv* Gateway or on different systems.

With the mentioned packages it is possible to read in the sensor data packets in Node-RED with the help of an MQTT input node and to store them in an InfluxDB via an InfluxDB output node. In Node-RED, current sensor values can also be clearly displayed in a dashboard. Grafana, on the other hand, can be used to display the chronological sequences in the form of charts, which uses the data stored in the InfluxDB for this purpose.

## Best Practice (4): Archiving the Data in a Database

The Windows application *LoraPacketViewer* is primarily used for commissioning and diagnostics, but it does not allow persistent saving and thus evaluation of the environmental data. For this it is necessary to store the data permanently in a database, such as an *InfluxDB*.

For a simple implementation, *Node-RED* is a good choice, since it supports nodes for receiving MQTT data packets as well as for writing data to an *InfluxDB*. The conversion of the sensor data from the JSON records transmitted via MQTT into the *InfluxDB* line format is done by simple JavaScript code within a user function node. A simple implementation in the form of a complete node-RED flow is shown in [Flow_Mqtt_InfluxDB.json](Node-RED/Flow_Mqtt_InfluxDB.json).

![\[Node-RED Flow MQTT to InfluxDB\]](Documentation/Flow_Mqtt_InfluxDB.png)


