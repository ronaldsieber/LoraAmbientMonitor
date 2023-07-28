# LoraPacketViewer

This Visual Studio C# project is part of the main project [LoraAmbientMonitor](https://github.com/ronaldsieber/LoraAmbientMonitor) and implements a GUI application to display the JSON records sent by the [LoraPacketRecv](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketRecv) subproject either online to the MQTT broker or the log files generated offline. This includes both the bootup and sensor data packets. This GUI application is primarily used for commissioning and diagnostics. It is not required for productive use.

![\[Project Overview - LoraPacketViewer\]](../Documentation/Project_Overview_LoraPacketViewer.png)

The *LoraPacketViewer* includes the following 4 tabsheets in its main window:

- **Bootup Summary:**
Displays in tabular form for each of the 16 possible DevID's the last processed bootup record (device configuration).

- **Device Summary:**
Displays in tabular form for each of the 16 possible DevID's the last processed sensor data record (current environment data)

- **Device History:**
Displays in tabular form the history of all sensor data records (current environment data) for all or selected DevID's in the order in which they were processed

- **Package Log:**
Lists status information and all JSON records (both bootup and sensor data packets) in order of processing

![\[LoraPacketViewer\]](../Documentation/LoraPacketViewer.png)

## Online Mode

In online mode, the *LoraPacketViewer* connects to the MQTT broker and receives from it all JSON records sent by the [LoraPacketRecv](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketRecv) subproject. For this purpose, the two fields *"Host URL"* and *"Host Port"* must be configured according to the IP address and port number of the broker. With the button *"Connect"* the connection to the broker is established. After the connection has been successfully established, the status indicator changes from red to green.

By marking the MQTT packets as *"Retain"*, the broker stores the last received message of a topic. This causes the *LoraPacketViewer* to receive its last sent boot record (device configuration) and sensor data record (current environment data) from each sensor module when it logs on to the broker. Thus, *LoraPacketViewer* knows the current state of the overall system immediately after it logs on to the broker.

## Offline Mode

In offline mode, a file with JSON records is read in and processed via *"File -> Load..."*. The file can either have been previously created by *LoraPacketViewer* itself via *"File -> Save as..."* or it was created by the [LoraPacketRecv](https://github.com/ronaldsieber/LoraAmbientMonitor/LoraPacketRecv) subproject with the command line parameter *"-l=<msg_file>"*.

## MQTT Communication

As MQTT client implementation for communication between broker and *LoraPacketViewer* the package *"M2Mqtt"* is used. This is encapsulated within *LoraPacketViewer* by the file *LpvMqttClient.cs*.

The `ClientHandler_MqttMsgPublishReceived` method is registered as the handler to receive subscribed MQTT messages:

    // register handler to process received messages
    m_MqttClientInst.MqttMsgPublishReceived += ClientHandler_MqttMsgPublishReceived;

This uses the delegate `InvokeDlgt_DispatchLoraNodePackets`  to call the method `DispatchLoraNodePackets` in the GUI thread, passing it the topic and payload:

    m_AppForm.DispatchLoraNodePackets(strTopic_p, strPayload_p);

Further processing of the JSON records received via MQTT is then performed in the context of the GUI thread.

## Parsing and Displaying the JSON Records

For parsing the received JSON records the package *"Newtonsoft.Json"* is used. The static method `JsonPacketSerializer.Deserialize` contained in it supports deserialization of JSON objects in custom types.

The method `DispatchLoraNodePackets` in the GUI thread forms the main instance for deserializing the JSON objects and their subsequent redistribution to the 4 tabsheets located in the main window. This method is responsible for processing JSON records received online via MQTT as well as JSON records read offline from a file.

First, for each JSON record to be processed, it is determined whether it is a boot or sensor data record. For this the method `JsonPacketSerializer.Deserialize` is called with the custom type `JsonPacketInfo`:

    JsonPacketReader = new JsonTextReader(new StringReader(strPayload_p));
    PacketInfo = JsonPacketSerializer.Deserialize<JsonPacketInfo>(JsonPacketReader);
    PacketType = PacketInfo.GetPacketType();

Parsing the bootup and sensor data records is then done in the same way using the custom types `JsonStationBootup` and `JsonStationData`. All custom types are implemented in the file *LpvJsonPackets.cs*.

Depending on the package type, the data is then displayed in the 4 tabsheets.

## Third Party Components used

 1. **MQTT Client**
Package: M2Mqtt
Version: 4.3.0
Author: Paolo Patierno
This package is installed within the Visual Studio IDE using the NuGet Packet Manager.

2. **JSON Deserialization**
Package: Newtonsoft.Json
Version: 13.0.2
Author: James Newton-King
This package is installed within the Visual Studio IDE using the NuGet Packet Manager.



