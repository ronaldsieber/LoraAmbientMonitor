#! /bin/sh

#***************************************************************************#
#                                                                           #
#  Copyright (c) 2023 Ronald Sieber                                         #
#                                                                           #
#  File:         Start_LoraPacketRecv.sh                                    #
#  Description:  Start Script for LoraPacketRecv                            #
#                                                                           #
#  -----------------------------------------------------------------------  #
#                                                                           #
#  IMPORTANT:                                                               #
#                                                                           #
#  This script must be run in sudo mode:                                    #
#  sudo ./Start_LoraPacketRecv.sh                                           #
#                                                                           #
#  -----------------------------------------------------------------------  #
#                                                                           #
#  Revision History:                                                        #
#                                                                           #
#  2023/03/25 -rs:   V1.00 Initial version                                  #
#                                                                           #
#****************************************************************************


#----------------------------------------------------------------------------
#   Configuration Section
#----------------------------------------------------------------------------

LORA_PACKET_RECV_LOG=/run/LoraPacketRecv.log
JSON_MSG_LOG=/run/LoraMessages.json
PID_FILE=/run/PID_LoraPacketRecv

LORA_PACKET_RECV_BIN=/home/pi/LoraPacketRecv/LoraPacketRecv/LoraPacketRecv
MQTT_HOST_FILE=/home/pi/LoraPacketRecv/Autostart/mqtt_host
MQTT_HOST_DEFAULT=127.0.0.1



#----------------------------------------------------------------------------
#   Wait until ETH0 is up
#----------------------------------------------------------------------------

echo "" >> $LORA_PACKET_RECV_LOG

# wait until ETH0 is up
echo "Wait until ETH0 is up..." >> $LORA_PACKET_RECV_LOG
WAIT_TIMEOUT_SECONDS=15
ETH0_STATUS=`cat /sys/class/net/eth0/carrier`
until [ $ETH0_STATUS -eq 1 ];
do
    if [ $WAIT_TIMEOUT_SECONDS -eq 0 ]; then
        echo "" >> $LORA_PACKET_RECV_LOG
        echo "Warning: ETH0 was not UP within expected time span" >> $LORA_PACKET_RECV_LOG
        echo "" >> $LORA_PACKET_RECV_LOG
        break
    fi

    echo "Wait for ETH0: $WAIT_TIMEOUT_SECONDS" >> $LORA_PACKET_RECV_LOG
    sleep 1
    WAIT_TIMEOUT_SECONDS=$((WAIT_TIMEOUT_SECONDS-1))
    ETH0_STATUS=`cat /sys/class/net/eth0/carrier`
done
echo "" >> $LORA_PACKET_RECV_LOG


# wait until 'hostname -I' reports valid IPAddress
echo "Wait until 'hostname -I' reports valid IPAddress..." >> $LORA_PACKET_RECV_LOG
WAIT_TIMEOUT_SECONDS=15
HOSTNAME_I=`hostname -I`
until [ "$HOSTNAME_I" != "" ];
do
    if [ $WAIT_TIMEOUT_SECONDS -eq 0 ]; then
        echo "" >> $LORA_PACKET_RECV_LOG
        echo "Warning: 'hostname -I' doesn't report valid IPAddress within expected time span" >> $LORA_PACKET_RECV_LOG
        echo "" >> $LORA_PACKET_RECV_LOG
        break
    fi

    echo "Wait for 'hostname -I': $WAIT_TIMEOUT_SECONDS" >> $LORA_PACKET_RECV_LOG
    sleep 1
    WAIT_TIMEOUT_SECONDS=$((WAIT_TIMEOUT_SECONDS-1))
    HOSTNAME_I=`hostname -I`
done
echo "HOSTNAME_I=$HOSTNAME_I" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG



#----------------------------------------------------------------------------
#   Startup Section
#----------------------------------------------------------------------------

echo "" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "==== System Start ====" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
date >> $LORA_PACKET_RECV_LOG
uptime >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG

SCRIPT_PID=$$
echo "PID of this script: $SCRIPT_PID" >> $LORA_PACKET_RECV_LOG
echo $SCRIPT_PID > $PID_FILE

if [ -f $MQTT_HOST_FILE ]
then
    echo "Get MQTT_HOST from file '$MQTT_HOST_FILE'" >> $LORA_PACKET_RECV_LOG
    MQTT_HOST=`cat $MQTT_HOST_FILE`
else
    echo "Get MQTT_HOST from file 'MQTT_HOST_DEFAULT'" >> $LORA_PACKET_RECV_LOG
    MQTT_HOST=$MQTT_HOST_DEFAULT
fi
echo "MQTT Host: $MQTT_HOST" >> $LORA_PACKET_RECV_LOG



#----------------------------------------------------------------------------
#   Respawn Loop
#----------------------------------------------------------------------------

while true
do
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "===========================================================" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    date >> $LORA_PACKET_RECV_LOG
    uptime >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "  Start '$LORA_PACKET_RECV_BIN -h=$MQTT_HOST -l=$JSON_MSG_LOG -v'" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "===========================================================" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    $LORA_PACKET_RECV_BIN -h=$MQTT_HOST -l=$JSON_MSG_LOG -v >> $LORA_PACKET_RECV_LOG 2>&1
    EXIT_CODE=$?
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "#### PROCESS TERMINATED ####" >> $LORA_PACKET_RECV_LOG
    echo "     ExitCode: " $EXIT_CODE >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    date >> $LORA_PACKET_RECV_LOG
    uptime >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    echo "" >> $LORA_PACKET_RECV_LOG
    if [ $EXIT_CODE -eq 252 ]; then
        echo "" >> $LORA_PACKET_RECV_LOG
        echo "Warning: MQTT Broker not available" >> $LORA_PACKET_RECV_LOG
        echo "" >> $LORA_PACKET_RECV_LOG
        sleep 10
    fi 
    sleep 1
done


exit 0

