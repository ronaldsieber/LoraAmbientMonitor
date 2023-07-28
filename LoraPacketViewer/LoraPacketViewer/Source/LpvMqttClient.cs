/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  MQTT Client Connection for LoRa Packet Viewer

  -------------------------------------------------------------------------

    The usage of MQTT in an C# application requires to install the
    "M2Mqtt" package. This can be done inside MS Visual Studio with
    the following steps:

        Extras -> NuGet Packet Manager -> Packet Manager Console

        PM> Install-Package M2Mqtt -Version 4.3.0

    For more details see:   https://www.nuget.org/packages/M2Mqtt

  -------------------------------------------------------------------------

    For detailed information about usage of "M2Mqtt" see:

        - https://m2mqtt.wordpress.com/m2mqtt_doc
        - https://m2mqtt.wordpress.com/using-mqttclient

        - https://code.msdn.microsoft.com/windowsapps/M2Mqtt-MQTT-client-library-ac6d3858

        - https://www.hivemq.com/blog/mqtt-client-library-encyclopedia-m2mqtt

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


using System;
using System.Text;
using uPLibrary.Networking.M2Mqtt;
using uPLibrary.Networking.M2Mqtt.Messages;




namespace LoraPacketViewer
{

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    //          C L A S S   LpvMqttClient                                  //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public  class  LpvMqttClient
    {

        //=================================================================//
        //                                                                 //
        //      D A T A   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-------------------------------------------------------------------
        //  Definitions
        //-------------------------------------------------------------------

        private  const String   MQTT_CLIENT_BASE_NAME   = "LoraPackView";
        private  const String   MQTT_USER_NAME          = "{empty}";
        private  const String   MQTT_PASSWORD           = "{empty}";
        private  const bool     MQTT_CLEAN_SESSION      = true;
        private  const ushort   MQTT_KEEPALIVE_PERIOD   = 60;
        private  const String   MQTT_SUBSCRIBE_TOPIC    = "LoraAmbMon/Data/#";



        //-------------------------------------------------------------------
        //  Types
        //-------------------------------------------------------------------

        private  delegate  void  DlgtDispatchLoraNodePackets (String strTopic_p, String strPayload_p);



        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        private  LpvAppForm                     m_AppForm;
        private  String                         m_strClientId;
        private  MqttClient                     m_MqttClientInst = null;

        private  DlgtDispatchLoraNodePackets    m_InvokeDlgt_DispatchLoraNodePackets;





        //=================================================================//
        //                                                                 //
        //      C O D E   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-----------------------------------------------------------------//
        //                                                                 //
        //      P U B L I C   M E T H O D E S                              //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  Standard Constructor
        //-------------------------------------------------------------------

        public  LpvMqttClient (LpvAppForm AppForm_p)
        {

            String  strGuid;


            m_AppForm = AppForm_p;

            // build unique ClientID string
            strGuid = Guid.NewGuid().ToString();
            strGuid = strGuid.Substring (0, 13);
            m_strClientId = MQTT_CLIENT_BASE_NAME + "_" + strGuid;

            m_InvokeDlgt_DispatchLoraNodePackets = new DlgtDispatchLoraNodePackets(InvokeDlgt_DispatchLoraNodePackets);

            return;

        }



        //-------------------------------------------------------------------
        //  Connect to MQTT Broker
        //-------------------------------------------------------------------

        public  bool  ConnectToBroker (String strBrokerHostName_p, int iBrokerPort_p)
        {

            byte    bBrokerResponseCode;
            ushort  usSubMsgId;


            // create a new client instance
            m_MqttClientInst = new MqttClient(strBrokerHostName_p,                          // HostName of MQTT Broker
                                              iBrokerPort_p,                                // PortNum of MQTT Broker
                                              false,                                        // Security
                                              MqttSslProtocols.None,                        // sslProtocol
                                              null,                                         // RemoteCertificateValidationCallback
                                              null);                                        // LocalCertificateSelectionCallback


            // connect the client instance to the broker
            try
            {
                bBrokerResponseCode = m_MqttClientInst.Connect(m_strClientId,               // ClientID
                                                               MQTT_USER_NAME,              // UserName
                                                               MQTT_PASSWORD,               // Password
                                                               MQTT_CLEAN_SESSION,          // Flag: CleanSession
                                                               MQTT_KEEPALIVE_PERIOD);      // KeepAlive Period [sec]
            }
            catch (Exception ExceptionInfo)
            {
                Console.WriteLine("ERROR: {0}" + ExceptionInfo.ToString());
                bBrokerResponseCode = 255;
            }

            // check the response code from the broker
            // from the MQTT specification it's value is 0 if the connection was accepted
            // or a number greater than 0 identifying the reason of connection failure
            if (bBrokerResponseCode != 0)
            {
                Console.WriteLine("MQTT Connection failed with Broker Response Code {0}" + Environment.NewLine, bBrokerResponseCode);
                return (false);
            }

            // register handler to process received messages
            m_MqttClientInst.MqttMsgPublishReceived += ClientHandler_MqttMsgPublishReceived;

            // subscribe topic to receive
            try
            {
                Console.WriteLine("MQTT: Subscribe Topic '{0}'...", MQTT_SUBSCRIBE_TOPIC);
                usSubMsgId = m_MqttClientInst.Subscribe(new string[] { MQTT_SUBSCRIBE_TOPIC },
                                                        new byte[] { MqttMsgBase.QOS_LEVEL_AT_MOST_ONCE });
            }
            catch (Exception ExceptionInfo)
            {
                Console.WriteLine("ERROR: {0}" + ExceptionInfo.ToString());
                return (false);
            }

            return (true);

        }



        //-------------------------------------------------------------------
        //  Disconnect from MQTT Broker
        //-------------------------------------------------------------------

        public  void  DisconnectFromBroker ()
        {

            // disconnect from MQTT broker
            try
            {
                if (m_MqttClientInst != null)
                {
                    if ( m_MqttClientInst.IsConnected )
                    {
                        m_MqttClientInst.Disconnect();
                    }
                }
            }
            catch (Exception ExceptionInfo)
            {
                Console.WriteLine("ERROR: {0}" + ExceptionInfo.ToString());
            }

            return;

        }



        //-------------------------------------------------------------------
        //  Check Connection State to MQTT Broker
        //-------------------------------------------------------------------

        public  bool  IsConnectedToBroker ()
        {

            bool  fIsConnected;


            fIsConnected = false;
            if (m_MqttClientInst != null)
            {
                fIsConnected = m_MqttClientInst.IsConnected;
            }

            return (fIsConnected);

        }



        //-------------------------------------------------------------------
        //  MQTT Handler to Process Subscribed Messages
        //-------------------------------------------------------------------

        void  ClientHandler_MqttMsgPublishReceived (object Sender_p, MqttMsgPublishEventArgs EventArgs_p)
        {

            String  strTopic;
            String  strPayload;


            // get topic and payload from received message
            strTopic   = EventArgs_p.Topic;
            strPayload = Encoding.UTF8.GetString(EventArgs_p.Message);

            // invoke back to main thread to have access to the GUI form
            m_AppForm.Invoke(m_InvokeDlgt_DispatchLoraNodePackets, new object[] {strTopic, strPayload});

            return;

        }



        //-------------------------------------------------------------------
        //  InvokeDelegate for passing received Packets to Form Thread
        //-------------------------------------------------------------------

        private  void  InvokeDlgt_DispatchLoraNodePackets (String strTopic_p, String strPayload_p)
        {

            m_AppForm.DispatchLoraNodePackets(strTopic_p, strPayload_p);
            return;

        }

    }   // public  class  LpvMqttClient

}   // namespace LoraPacketViewer




// EOF

