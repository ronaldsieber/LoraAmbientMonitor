# LoraPacketRecv

This Linux project is part of the main project [LoraAmbientMonitor](https://github.com/ronaldsieber/LoraAmbientMonitor) and contains the RaspberryPi console application for the receiver device. This gateway software receives the LoRa packets sent by one or more sensor devices described in the [LoraAmbientMonitor](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraAmbientMonitor) subproject, decodes the payload, and publishes the data as a JSON record via MQTT to a superordinate broker. To receive the LoRa data, the SEEED LORA/GPS RASPBERRY PI HAT is required.

The stick antenna included with the LORA PI HAT is well suited for development and short distances due to its compact and space-saving design. To increase the range, it is better to use an external antenna (6dBi or better) with coaxial cable in productive use.

The software was developed and tested on a Raspberry Pi 3 Model B+. The sources stored here can be compiled directly on the RaspberryPi by using the "make" command.

![\[Project Overview - LoraPacketRecv\]](../Documentation/Project_Overview_LoraPacketRecv.png)

## Running the Software

The *LoraPacketRecv* software requires root privileges to access the hardware. It must therefore be started in *"sudo"* mode. The following command line options are supported:

***-h=<host_url>***
Host URL of the MQTT broker in the format *URL[:Port]*.

***-l=<msg_file>***
Logging of all JSON records sent to the MQTT broker to the specified file (the log file is always opened in APPEND mode). The log file can later be displayed and evaluated using the GUI application implemented in the [LoraPacketViewer](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketViewer) subproject

***-a***
Forwarding of JSON records for all received LoRa packets to the MQTT broker, including any duplicates (Gen0/Gen1/Gen2)

***-t***
In addition to the JSON records, compact telemetry messages are published to the MQTT broker, which support commissioning and diagnostics in particular (see section *"MQTT Communication"*).

***-o***
Offline mode, without connection to the MQTT broker

***-v***
Verbose mode with detailed protocol outputs

***--help***
Display help screen and default configuration for host and port number of the MQTT broker

**Examples:**

    sudo ./LoraPacketRecv -h=127.0.0.1 -l=./LoraPacketLog.json -v

With help of the Linux command *"tee"* it is possible to save the output displayed in the terminal window parallel to the real-time output still in a log file for later evaluation:

    sudo ./LoraPacketRecv -h=127.0.0.1 -l=./LoraPacketLog.json -v | tee > ./LoraPacketRecv.log

## LoRa Data Reception

The SEEED LORA/GPS RASPBERRY PI HAT used for receiving LoRa data communicates with the RaspberryPi via SPI interface. On the software side, the *"RadioHead"* driver library is used to initialize the SX1276 LoRa chip and read out received data packets. The driver library is encapsulated in the *LoraPacketRecv* by the file *LibRf95.cpp*.

The LORA/GPS HAT signals the reception of a LoRa packet by a falling edge at GPIO25 of the RaspberryPi. The interrupt handling of the GPIO mapped in SysFS is completely encapsulated in *GpioIrq.cpp*. Within the main loop, GPIO25 is linked to the file descriptor `struct pollfd FdSet[1]`. The Linux poll function (`poll(FdSet,1,1000)`) waits for a signaling event (LoRa data reception), alternatively it returns after the timeout of 1000 ms. The main loop is thus executed after a LoRa packet is received or after 1 second at the latest, which minimizes CPU load without putting the main loop completely into sleep mode.

A received LoRa packet is read from the receive buffer of the SX1276 by the function `RF95GetRecvDataPacket()`. The current system time is assigned to the data packet as the receive timestamp, and the value of the `uiMsgID` variable is taken as the Message ID for the packet. Subsequently, the function `PprGainLoraDataRecord()` evaluates the packet and returns the decoded payload content of a packet of a *LoraAmbientMonitor* sensor module qualified as valid in the form of the data structure `tLoraMsgData`.

## Generation of JSON Records

The *PprBuildJsonMessages()* function converts the binary data of the received LoRa packets decoded in the `tLoraMsgData` data structure into corresponding JSON records. This applies to both bootup packets and sensor data packets. For the latter, a separate JSON record is generated from each of the 3 generations of sensor data records (`kLoraPacketDataGen0`, `kLoraPacketDataGen1`, and `kLoraPacketDataGen2`) along with header information.

The JSON record of a bootup package has the following exemplary structure:

    {
      "MsgID": 1,
      "MsgType": "StationBootup",
      "TimeStamp": 1678546265,
      "TimeStampFmt": "2023/03/11 - 15:51:05",
      "RSSI": -37,
      "DevID": 1,
      "FirmwareVer": "1.00",
      "DataPackCycleTm": 3600,
      "CfgOledDisplay": 1,
      "CfgDhtSensor": 1,
      "CfgSr501Sensor": 1,
      "CfgAdcLightSensor": 1,
      "CfgAdcCarBatAin": 1,
      "CfgAsyncLoraEvent": 0,
      "Sr501PauseOnLoraTx": 1,
      "CommissioningMode": 0,
      "LoraTxPower": 20,
      "LoraSpreadFactor": 12
    }

The JSON record of a sensor data packet has the following exemplary structure:

    {
      "MsgID": 2,
      "MsgType": "StationDataGen0",
      "TimeStamp": 1678546328,
      "TimeStampFmt": "2023/03/11 - 15:52:08",
      "RSSI": -45,
      "DevID": 1,
      "SequNum": 1,
      "Uptime": 66,
      "UptimeFmt": "0d/00:01:06",
      "Temperature": 21.5,
      "Humidity": 44.0,
      "MotionActive": 0,
      "MotionActiveTime": 10,
      "MotionActiveCount": 1,
      "LightLevel": 8,
      "CarBattLevel": 0.0
    }

The `PprBuildJsonMessages()` function returns the JSON records as a Vector object of type `std::vector<tJsonMessage>`. The Vector object contains up to 3 JSON records depending on the type (`StationBootup`, `StationDataGen0/1/2`).

## Processing of JSON Records

Unless *LoraPacketRecv* was started with the command line parameter *"-a"*, the function `MquIsMessageToBeProcessed()` determines the relevance of the publishing of the current JSON record to the MQTT broker. Records with ***"MsgType" = "StationDataGen0"*** are always transmitted. Records with ***"MsgType" = "StationDataGen1"*** are only processed if the previous packet with the Gen0 data was not received, ***"StationDataGen2"*** packets are only transmitted if both the Gen0 data and the Gen1 data were lost. The array `aui32SequNumHistList_l` (in *MessageQualification.cpp*), which carries a separate list of the last processed records for each *LoraAmbientMonitor* sensor device based on its LoRa packet sequence number, is used to evaluate relevance.

![\[LoraPacketRecv_Console\]](../Documentation/LoraPacketRecv_Console.png)

When *LoraPacketRecv* is started with the command line parameter *"-a"*, all JSON records are forwarded to the broker, including Gen1 data and Gen2 data from previously processed data.

## MQTT Communication

As MQTT client implementation for communication between the *LoraPacketRecv* and the broker the *"Paho MQTT Embedded/C"* library is used. This is encapsulated within *LoraPacketRecv* by the file *LibMqtt.cpp*.

For publishing messages to the MQTT broker the following topics are defined in *Main.cpp*:

    const char* MQTT_TOPIC_TMPL_BOOTUP  = "LoraAmbMon/Data/DevID%03u/Bootup";
    const char* MQTT_TOPIC_TMPL_ST_DATA = "LoraAmbMon/Data/DevID%03u/StData";
    const char* MQTT_TOPIC_TELEMETRY    = "LoraAmbMon/Status/Telemetry";
    const char* MQTT_TOPIC_KEEPALVIE    = "LoraAmbMon/Status/KeepAlive";

Separate topics are used for publishing bootup and sensor data packets respectively (`MQTT_TOPIC_TMPL_BOOTUP` and `MQTT_TOPIC_TMPL_ST_DATA`). The function `BuildMqttPublishTopic()` individualizes the topic for each sensor module by inserting the DevID (the node number set at the DIP switch). In addition, the MQTT publishing is done with the *"Retain"* flag, so that the broker saves the last received message of a topic in each case.

This has the effect that a client such as [LoraPacketViewer](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketViewer) belonging to the main project [LoraAmbientMonitor](https://github.com/ronaldsieber/LoraAmbientMonitor) receives its last sent bootup record (`MQTT_TOPIC_TMPL_BOOTUP` = device configuration) and sensor data record (`MQTT_TOPIC_TMPL_ST_DATA` = current station environment data) from each sensor module when it connects to the broker. The client thus knows the current state of the overall system immediately after connecting to the broker.

When *LoraPacketRecv* is started with the command line parameter *"-t"*, compact telemetry messages with the topic `MQTT_TOPIC_TELEMETRY` are published to the MQTT broker in addition to the JSON records. These contain the following information as ASCII string:
- Receive timestamp
- Message-ID (consecutive number over all packets on receiver side)
- DevID (Node-ID) of the sensor module
- Sequence number (consecutive number for all sent packets of a sensor module)
- RSSI level of the received packet

The telemetry message has the following exemplary structure:

    Time=2023/03/11-15:52:08, MsgID=2, Dev=1, Seq=1, RSSI=-45

To actively maintain the connection to the broker, *LoraPacketRecv* uses the topic `MQTT_TOPIC_KEEPALVIE` to send keep-alive messages. The constant `MQTT_KEEPALIVE_INTERVAL` manages the time base. The function `MqttKeepAlive()` is responsible for sending the messages.

## Autostart for LoraPacketRecv

A high availability of the *LoraPacketRecv* gateway software is an elementary requirement for the successful forwarding of the data sent by the sensor modules via LoRa to a central MQTT broker. Therefore, the gateway software should be started automatically when booting the RasperryPi. If there is an unintentional termination of the software during runtime, it shall also be restarted immediately ("respawn").

For the start/stop handling of the *LoraPacketRecv* Gateway software the scripts contained in the subdirectory *"Autostart"* are responsible. In order to run them on the RaspberryPi, they need appropriate permissions:

    cd /home/pi/LoraPacketRecv/Autostart
    chmod +x LoraPacketRecv
    chmod +x Start_LoraPacketRecv.sh
    chmod +x Stop_LoraPacketRecv.sh

- ***LoraPacketRecv:***
This script is used to start and stop the gateway software based on the Linux /etc/init.d/ process. Among other things, it ensures that the gateway software is automatically started by the Linux init process during bootup. For this purpose it uses the two scripts *"Start_LoraPacketRecv.sh"* and *"Start_LoraPacketRecv.sh"*. For the script to be called automatically, it must first be copied to the directory *"/etc/init.d/"* and registered there:

       cd home/pi/LoraPacketRecv/Autostart
       chmod +x LoraPacketRecv
       sudo cp ./LoraPacketRecv /etc/init.d/
       cd /etc/init.d
       sudo update-rc.d LoraPacketRecv defaults

- ***Start_LoraPacketRecv.sh:***
This script starts the *LoraPacketRecv* gateway software with root privileges as a background process. To do this, it first waits until the Ethernet interface is fully available and configured with a valid IP address. This is a prerequisite for establishing the connection to the MQTT broker. The broker address itself is read from the *"mqtt_host"* file, which must be adjusted accordingly. Since a background process cannot make direct outputs in a terminal, all outputs are written to the logfile configured via the environment variable `LORA_PACKET_RECV_LOG`. To avoid loading the SD card with permanent write accesses as much as possible, the file *"/run/LoraPacketRecv.log"* in the RAM file system of the RaspberryPi is used for the outputs by default. Logging of JSON records to the file specified via the command line parameter *"-l=<msg_file>"* is controlled by the `JSON_MSG_LOG` environment variable. Also for *"/run/LoraPacketRecv.log"*, the RAM file system is also used by default. The while loop at the end of the script implements respawn functionality, i.e. it restarts the gateway software should it have terminated in an uncontrolled manner. To support a controlled termination, the script writes its own process ID into the file defined by `PID_FILE` (*"/run/PID_LoraPacketRecv"*), which is evaluated by *"Stop_LoraPacketRecv.sh"* if required.

- ***Stop_LoraPacketRecv.sh:***
This script terminates the *LoraPacketRecv* gateway software in a controlled manner. To do this, it reads the process ID stored by the *"Start_LoraPacketRecv.sh"* script in the file defined by `PID_FILE` (*"/run/PID_LoraPacketRecv"*). This process ID is then used as a parameter for the call to `"kill -9 <pid>"`. This terminates both the *LoraPacketRecv* gateway software itself and the startup script with its respawn loop.

## Third Party Components used

1. **LoRa Radio**
For the SX1276 LoRa chipset of the SEEED LORA/GPS RASPBERRY PI HAT the *"RadioHead"* driver library is used:
https://github.com/idreamsi/RadioHead

2. **MQTT Client**
For the MQTT client implementation the *"Paho MQTT Embedded/C"* library is used:
https://github.com/eclipse/paho.mqtt.embedded-c



