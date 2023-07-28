/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  TestSuite for Application

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

using System;





namespace LoraPacketViewer
{

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    //          C L A S S   TestSuite                                      //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public class TestSuite
    {

        //=================================================================//
        //                                                                 //
        //      D A T A   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-------------------------------------------------------------------
        //  Test Data
        //-------------------------------------------------------------------

        //---------------- MsgID001: DevID01 - Bootup ----------------
        static readonly string strJsonPacket_MsgID001_DevID01_Bootup =
        @"{
          ""MsgID"": 1,
          ""MsgType"": ""StationBootup"",
          ""TimeStamp"": 1678546265,
          ""TimeStampFmt"": ""2023/03/11 - 15:51:05"",
          ""RSSI"": -34,
          ""DevID"": 1,
          ""FirmwareVer"": ""0.99"",
          ""DataPackCycleTm"": 180,
          ""CfgOledDisplay"": 1,
          ""CfgDhtSensor"": 1,
          ""CfgSr501Sensor"": 1,
          ""CfgAdcLightSensor"": 1,
          ""CfgAdcCarBatAin"": 1,
          ""CfgAsyncLoraEvent"": 0,
          ""Sr501PauseOnLoraTx"": 1,
          ""LoraTxPower"": 20,
          ""LoraSpreadFactor"": 12
        }
        ";



        //---------------- MsgID002: DevID01 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID002_DevID01_DataGen0 =
        @"{
          ""MsgID"": 2,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546328,
          ""TimeStampFmt"": ""2023/03/11 - 15:52:08"",
          ""RSSI"": -143,
          ""DevID"": 1,
          ""SequNum"": 1,
          ""Uptime"": 66,
          ""UptimeFmt"": ""0d/00:01:06"",
          ""Temperature"": 23.5,
          ""Humidity"": 41.0,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 360,
          ""MotionActiveCount"": 6400,
          ""LightLevel"": 60,
          ""CarBattLevel"": 11.5
        }
        ";



        //---------------- MsgID003: DevID01 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID003_DevID01_DataGen0 =
        @"{
          ""MsgID"": 3,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546503,
          ""TimeStampFmt"": ""2023/03/11 - 15:55:03"",
          ""RSSI"": -43,
          ""DevID"": 1,
          ""SequNum"": 2,
          ""Uptime"": 241,
          ""UptimeFmt"": ""0d/00:04:01"",
          ""Temperature"": 23.5,
          ""Humidity"": 41.0,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID004: DevID05 - Bootup ----------------
        static readonly string strJsonPacket_MsgID004_DevID05_Bootup =
        @"{
          ""MsgID"": 4,
          ""MsgType"": ""StationBootup"",
          ""TimeStamp"": 1678546592,
          ""TimeStampFmt"": ""2023/03/11 - 15:56:32"",
          ""RSSI"": -22,
          ""DevID"": 5,
          ""FirmwareVer"": ""1.23"",
          ""DataPackCycleTm"": 3600,
          ""CfgOledDisplay"": 1,
          ""CfgDhtSensor"": 1,
          ""CfgSr501Sensor"": 1,
          ""CfgAdcLightSensor"": 1,
          ""CfgAdcCarBatAin"": 1,
          ""CfgAsyncLoraEvent"": 0,
          ""Sr501PauseOnLoraTx"": 0,
          ""LoraTxPower"": 20,
          ""LoraSpreadFactor"": 12
        }
        ";



        //---------------- MsgID005: DevID05 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID005_DevID05_DataGen0 =
        @"{
          ""MsgID"": 5,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546651,
          ""TimeStampFmt"": ""2023/03/11 - 15:57:31"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 1,
          ""Uptime"": 62,
          ""UptimeFmt"": ""0d/00:01:02"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID006: DevID01 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID006_DevID01_DataGen0 =
        @"{
          ""MsgID"": 6,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546688,
          ""TimeStampFmt"": ""2023/03/11 - 15:58:08"",
          ""RSSI"": -43,
          ""DevID"": 1,
          ""SequNum"": 3,
          ""Uptime"": 426,
          ""UptimeFmt"": ""0d/00:07:06"",
          ""Temperature"": 23.5,
          ""Humidity"": 41.0,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID007: DevID05 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID007_DevID05_DataGen0 =
        @"{
          ""MsgID"": 7,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546827,
          ""TimeStampFmt"": ""2023/03/11 - 16:00:27"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 2,
          ""Uptime"": 238,
          ""UptimeFmt"": ""0d/00:03:58"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID008: DevID05 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID008_DevID05_DataGen0 =
        @"{
          ""MsgID"": 8,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678546862,
          ""TimeStampFmt"": ""2023/03/11 - 16:01:02"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 4,
          ""Uptime"": 601,
          ""UptimeFmt"": ""0d/00:10:01"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID009: DevID05 - StationDataGen1 ----------------
        static readonly string strJsonPacket_MsgID009_DevID05_DataGen1 =
        @"{
          ""MsgID"": 9,
          ""MsgType"": ""StationDataGen1"",
          ""TimeStamp"": 1678547003,
          ""TimeStampFmt"": ""2023/03/11 - 16:03:23"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 3,
          ""Uptime"": 414,
          ""UptimeFmt"": ""0d/00:06:54"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID010: DevID05 - StationDataGen2 ----------------
        static readonly string strJsonPacket_MsgID010_DevID05_DataGen2 =
        @"{
          ""MsgID"": 10,
          ""MsgType"": ""StationDataGen2"",
          ""TimeStamp"": 1678547042,
          ""TimeStampFmt"": ""2023/03/11 - 16:04:02"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 5,
          ""Uptime"": 780,
          ""UptimeFmt"": ""0d/00:13:00"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //---------------- MsgID011: DevID05 - StationDataGen0 ----------------
        static readonly string strJsonPacket_MsgID011_DevID05_DataGen0 =
        @"{
          ""MsgID"": 11,
          ""MsgType"": ""StationDataGen0"",
          ""TimeStamp"": 1678547175,
          ""TimeStampFmt"": ""2023/03/11 - 16:06:15"",
          ""RSSI"": -56,
          ""DevID"": 5,
          ""SequNum"": 4,
          ""Uptime"": 587,
          ""UptimeFmt"": ""0d/00:09:47"",
          ""Temperature"": 20.5,
          ""Humidity"": 46.5,
          ""MotionActive"": 1,
          ""MotionActiveTime"": 36,
          ""MotionActiveCount"": 64,
          ""LightLevel"": 6,
          ""CarBattLevel"": 0.0
        }
        ";



        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        static  uint  m_uiTestStep = 0;





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
        //  RunTestStep
        //-------------------------------------------------------------------

        public  static  void  RunTestStep (LpvAppForm AppForm_p)
        {

            switch (m_uiTestStep++)
            {
                case 0:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID001_DevID01_Bootup);     break;
                case 1:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID002_DevID01_DataGen0);   break;
                case 2:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID003_DevID01_DataGen0);   break;
                case 3:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID004_DevID05_Bootup);     break;
                case 4:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID005_DevID05_DataGen0);   break;
                case 5:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID006_DevID01_DataGen0);   break;
                case 6:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID007_DevID05_DataGen0);   break;
                case 7:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID008_DevID05_DataGen0);   break;
                case 8:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID009_DevID05_DataGen1);   break;
                case 9:     AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID010_DevID05_DataGen2);   break;
                case 10:    AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID011_DevID05_DataGen0);   break;
                default:    m_uiTestStep = 0;                                                                           break;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  RunTestBlock1
        //-------------------------------------------------------------------

        public  static  void  RunTestBlock1 (LpvAppForm AppForm_p)
        {

            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID001_DevID01_Bootup);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID002_DevID01_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID003_DevID01_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID004_DevID05_Bootup);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID005_DevID05_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID006_DevID01_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID007_DevID05_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID008_DevID05_DataGen0);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID009_DevID05_DataGen1);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID010_DevID05_DataGen2);
            AppForm_p.DispatchLoraNodePackets("strTopic_p", strJsonPacket_MsgID011_DevID05_DataGen0);

            return;

        }


    }   // public class TestSuite

}   // namespace LoraPacketViewer



// EOF
