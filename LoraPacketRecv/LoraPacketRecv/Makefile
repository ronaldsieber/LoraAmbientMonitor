#***************************************************************************#
#                                                                           #
#  Copyright (c) 2023 Ronald Sieber                                         #
#                                                                           #
#  File:         Makefile                                                   #
#  Description:  Makefile for LoRa Packet Receiver                          #
#                                                                           #
#  -----------------------------------------------------------------------  #
#                                                                           #
#  Revision History:                                                        #
#                                                                           #
#  2023/03/25 -rs:   V1.00 Initial version                                  #
#                                                                           #
#****************************************************************************


# --------- Project Settings ---------

ifeq ('$(TARGET_CFG)','')
#	TARGET_CFG	= RELEASE
	TARGET_CFG	= DEBUG
endif

#  Select between debug and release settings
ifeq ($(TARGET_CFG),RELEASE)
    DBG_MODE = NDEBUG
else
    DBG_MODE = _DEBUG
endif



# --------- Compile Settings ---------
CC					= g++
STRIP				= strip
CFLAGS				= -DRASPBERRY_PI -D$(DBG_MODE) -DBCM2835_NO_DELAY_COMPATIBILITY
LIBS				= -lbcm2835
SRC_RADIOHEAD		= ../RadioHead
SRC_GPIOIRQ			= ../GpioIrq
SRC_MQTT_PACKET		= ../Mqtt/paho_mqtt_embedded_c/MQTTPacket/src
SRC_MQTT_TRANSPORT	= ../Mqtt/Mqtt_Transport

INCLUDE				= -I$(SRC_RADIOHEAD) -I$(SRC_GPIOIRQ) -I$(SRC_MQTT_PACKET) -I$(SRC_MQTT_TRANSPORT)

EXEC				= LoraPacketRecv

OBJS				= Main.o \
					  LibRf95.o \
					  RH_RF95.o \
					  RHHardwareSPI.o \
					  RHSPIDriver.o \
					  RHGenericDriver.o \
					  RHGenericSPI.o \
					  RasPi.o \
					  LoraPayloadDecoder.o \
					  PacketProcessing.o \
					  MessageQualification.o \
					  MessageFileWriter.o \
					  GpioIrq.o \
					  LibMqtt.o \
					  MqttTransport_Posix.o \
					  MQTTConnectClient.o \
					  MQTTSubscribeClient.o \
					  MQTTUnsubscribeClient.o \
					  MQTTConnectServer.o \
					  MQTTSubscribeServer.o \
					  MQTTUnsubscribeServer.o \
					  MQTTPacket.o \
					  MQTTFormat.o \
					  MQTTSerializePublish.o \
					  MQTTDeserializePublish.o \
					  Trace.o



# --------- Default-Target ---------
all:				print_settings $(EXEC)



# --------- Print Settings ---------
print_settings:
					@echo
					@echo "Make Settings"
					@echo "   CFLAGS  = '$(CFLAGS)'"
					@echo "   INCLUDE = '$(INCLUDE)'"
					@echo "   LIBS    = '$(LIBS)'"
					@echo "   EXEC    = '$(EXEC)'"
					@echo



# --------- Compile single Source ---------

#           ----- MainApp -----
Main.o:				Makefile Main.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

LibRf95.o:			Makefile LibRf95.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

LibMqtt.o:			Makefile LibMqtt.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

LoraPayloadDecoder.o:	Makefile LoraPayloadDecoder.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

PacketProcessing.o:	Makefile PacketProcessing.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

MessageQualification.o:	Makefile MessageQualification.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o


MessageFileWriter.o:	Makefile MessageFileWriter.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o

Trace.o:			Makefile Trace.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(notdir $*.cpp) $(INCLUDE) -o $*.o


#           ----- RF95 -----
RH_RF95.o:			Makefile $(SRC_RADIOHEAD)/RH_RF95.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

RHHardwareSPI.o:	Makefile $(SRC_RADIOHEAD)/RHHardwareSPI.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

RHSPIDriver.o:		Makefile $(SRC_RADIOHEAD)/RHSPIDriver.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

RHGenericDriver.o:	Makefile $(SRC_RADIOHEAD)/RHGenericDriver.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

RHGenericSPI.o:		Makefile $(SRC_RADIOHEAD)/RHGenericSPI.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/$(notdir $*.cpp) $(INCLUDE) -o $*.o

RasPi.o:			Makefile $(SRC_RADIOHEAD)/RHutil/RasPi.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_RADIOHEAD)/RHutil/$(notdir $*.cpp) $(INCLUDE) -Wno-return-type -o $*.o


#           ----- GpioIRQ -----
GpioIrq.o:			Makefile $(SRC_GPIOIRQ)/GpioIrq.cpp
					@echo "Compiling '$(notdir $*.cpp)'..."
					@$(CC) $(CFLAGS) -c $(SRC_GPIOIRQ)/$(notdir $*.cpp) $(INCLUDE) -o $*.o


#           ----- MQTT -----
MQTTConnectClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTConnectClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSubscribeClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSubscribeClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTUnsubscribeClient.o:	Makefile $(SRC_MQTT_PACKET)/MQTTUnsubscribeClient.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTConnectServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTConnectServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSubscribeServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSubscribeServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTUnsubscribeServer.o:	Makefile $(SRC_MQTT_PACKET)/MQTTUnsubscribeServer.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTPacket.o:		Makefile $(SRC_MQTT_PACKET)/MQTTPacket.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTFormat.o:		Makefile $(SRC_MQTT_PACKET)/MQTTFormat.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTSerializePublish.o:	Makefile $(SRC_MQTT_PACKET)/MQTTSerializePublish.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MQTTDeserializePublish.o:	Makefile $(SRC_MQTT_PACKET)/MQTTDeserializePublish.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_PACKET)/$(notdir $*.c) $(INCLUDE) -o $*.o

MqttTransport_Posix.o:	Makefile $(SRC_MQTT_TRANSPORT)/MqttTransport_Posix.c
					@echo "Compiling '$(notdir $*.c)'..."
					@$(CC) $(CFLAGS) -c $(SRC_MQTT_TRANSPORT)/$(notdir $*.c) $(INCLUDE) -o $*.o



# --------- Link Executeable ---------
$(EXEC):			Makefile $(OBJS)
					@echo "Linking '$(EXEC)'..."
					@$(CC) -o $@ $(OBJS) $(LIBS)
ifeq ($(TARGET_CFG),RELEASE)
					@echo "Stripping '$(EXEC)'..."
					@$(STRIP) $@
endif
					@echo "Done."
					@echo



# --------- Clean Project ---------
clean:
					rm -f *.bak
					rm -f *.tmp
					rm -f $(EXEC)
					rm -f *.elf *.gdb *.o


