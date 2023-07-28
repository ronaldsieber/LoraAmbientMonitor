/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      MqttClientLib
  Description:  Public Definitions of Transport Layer for MqttClientLib

  -------------------------------------------------------------------------

    This implementation is based on 'transport.h', which is part of
    'paho.mqtt.embedded-c', downloaded from:

    https://github.com/eclipse/paho.mqtt.embedded-c

    This original source file includes the following copyright notice:

      -----------------------------------------------------------------

    Copyright (c) 2014 IBM Corp.

    All rights reserved. This program and the accompanying materials
    are made available under the terms of the Eclipse Public License v1.0
    and Eclipse Distribution License v1.0 which accompany this distribution.

    The Eclipse Public License is available at
        http://www.eclipse.org/legal/epl-v10.html
    and the Eclipse Distribution License is available at
        http://www.eclipse.org/org/documents/edl-v10.php.

    Contributors:
        Ian Craggs - initial API and implementation and/or initial documentation
        Sergio R. Caprile - "commonalization" from prior samples and/or documentation extension

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _MQTTTRANSPORT_H_
#define _MQTTTRANSPORT_H_



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

int  MqttTransport_LibStart         (void);
int  MqttTransport_LibStop          (void);
void MqttTransport_LibProcess       (void);

int  MqttTransport_Open             (char* pszHostUrl_p, int iHostPortNum_p);
int  MqttTransport_Close            (int iSocket_p);
int  MqttTransport_SendPacketBuffer (int iSocket_p, unsigned char* pabDataBuff_p, int iDataBuffLen_p);
int  MqttTransport_SetGetDataSocket (int iSocket_p);
int  MqttTransport_GetData          (unsigned char* pabDataBuff_p, int iDataBuffLen_p);
int  MqttTransport_GetDataNonBlock  (void *pvSocket_p, unsigned char* pabDataBuff_p, int iDataBuffLen_p);



#endif  // #ifdef _MQTTTRANSPORT_H_



