/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Declarations for MQTT Client Library

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LIBMQTT_H_
#define _LIBMQTT_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------

// QoS Level
typedef enum
{
    kMqttQoS0                       =  0,
    kMqttQoS1                       =  1,
    kMqttQoS2                       =  2

} tMqttQoSLevel;



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

int  MqttConnect (
    const char* pszHostUrl_p,                           // [IN]     Host URL
    unsigned int uiHostPortNum_p,                       // [IN]     Host PortNumber
    const char* pszClientName_p,                        // [IN]     Client Name
    unsigned int uiKeepAliveTimeout_p,                  // [IN]     KeepAlive Timeout in [sec]
    const char* pszUserName_p,                          // [IN]     User Authentication - UserName
    const char* pszPassword_p);                         // [IN]     User Authentication - Password


int  MqttReconnect ();


int  MqttDisconnect ();


int  MqttPublishMessage (
    const char* pszTopic_p,                             // [IN]     Topic string
    const unsigned char* pabPayloadBuff_p,              // [IN]     Address of Payload buffer
    unsigned int uiPayloadLen_p,                        // [IN]     Size of Payload buffer
    tMqttQoSLevel PublishQos_p,                         // [IN]     MQTT QoS value
    unsigned int fRetainedFlag_p);                      // [IN]     MQTT retained flag


int  MqttKeepAlive (
    const char* pszTopic_p);                            // [IN]     Topic string


void  MqttPrintMessage (
    const char* pszTopic_p,                             // [IN]     Topic string
    const unsigned char* pabPayloadBuff_p,              // [IN]     Address of Payload buffer
    unsigned int uiPayloadLen_p);                       // [IN]     Size of Payload buffer



#endif  // #ifndef _LIBMQTT_H_


// EOF

