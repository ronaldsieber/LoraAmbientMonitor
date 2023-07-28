#! /bin/sh

#***************************************************************************#
#                                                                           #
#  Copyright (c) 2023 Ronald Sieber                                         #
#                                                                           #
#  File:         Stop_LoraPacketRecv.sh                                     #
#  Description:  Stop Script for LoraPacketRecv                             #
#                                                                           #
#  -----------------------------------------------------------------------  #
#                                                                           #
#  IMPORTANT:                                                               #
#                                                                           #
#  This script must be run in sudo mode:                                    #
#  sudo ./Stop_LoraPacketRecv.sh                                            #
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
PID_FILE=/run/PID_LoraPacketRecv



#----------------------------------------------------------------------------
#   Stop Process
#----------------------------------------------------------------------------

SCRIPT_PID=`cat $PID_FILE`

echo "" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "===========================================================" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
date >> $LORA_PACKET_RECV_LOG
uptime >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "  Kill LoraPacketRecv (PID=$SCRIPT_PID)" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG
echo "===========================================================" >> $LORA_PACKET_RECV_LOG
echo "" >> $LORA_PACKET_RECV_LOG

kill -9 $SCRIPT_PID
rm $PID_FILE


exit 0

