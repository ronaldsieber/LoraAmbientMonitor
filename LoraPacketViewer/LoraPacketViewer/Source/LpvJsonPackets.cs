/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  JSON Data Classes for received LoRa Packets

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
    //          C L A S S   JsonPacketInfo                                 //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public class JsonPacketInfo
    {

        //-------------------------------------------------------------------
        //  Definitions
        //-------------------------------------------------------------------
        public enum tLoraPacketType
        {
            kLoraPacketUnknown,
            kLoraPacketBootup,
            kLoraPacketDataGen0,
            kLoraPacketDataGen1,
            kLoraPacketDataGen2
        } ;



        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        public uint     MsgID               { get; set; }
        public String   MsgType             { get; set; }
        public uint     DevID               { get; set; }



        //-------------------------------------------------------------------
        //  Methodes
        //-------------------------------------------------------------------

        public tLoraPacketType GetPacketType ()
        {

            if (MsgType.IndexOf("Bootup", 0, StringComparison.OrdinalIgnoreCase) != -1)
            {
                return (tLoraPacketType.kLoraPacketBootup);
            }
            if (MsgType.IndexOf("DataGen0", 0, StringComparison.OrdinalIgnoreCase) != -1)
            {
                return (tLoraPacketType.kLoraPacketDataGen0);
            }
            if (MsgType.IndexOf("DataGen1", 0, StringComparison.OrdinalIgnoreCase) != -1)
            {
                return (tLoraPacketType.kLoraPacketDataGen1);
            }
            if (MsgType.IndexOf("DataGen2", 0, StringComparison.OrdinalIgnoreCase) != -1)
            {
                return (tLoraPacketType.kLoraPacketDataGen2);
            }
            return (tLoraPacketType.kLoraPacketUnknown);

        }
    
        //-------------------------------------------------------------------

        public int GetDevID ()
        {

            if (DevID > 15)
            {
                return (-1);
            }

            return ((int)DevID);

        }

    }   // public class JsonPacketInfo



 
    
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    //          C L A S S   JsonStationBootup                              //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public class JsonStationBootup
    {

        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        // LoRa Packet Header
        public uint     MsgID               { get; set; }
        public String   MsgType             { get; set; }
        public uint     TimeStamp           { get; set; }
        public String   TimeStampFmt        { get; set; }
        public int      RSSI                { get; set; }

        // LoraStationBootup
        public uint     DevID               { get; set; }
        public String   FirmwareVer         { get; set; }
        public uint     DataPackCycleTm     { get; set; }
        public uint     CfgOledDisplay      { get; set; }
        public uint     CfgDhtSensor        { get; set; }
        public uint     CfgSr501Sensor      { get; set; }
        public uint     CfgAdcLightSensor   { get; set; }
        public uint     CfgAdcCarBatAin     { get; set; }
        public uint     CfgAsyncLoraEvent   { get; set; }
        public uint     Sr501PauseOnLoraTx  { get; set; }
        public uint     CommissioningMode   { get; set; }
        public uint     LoraTxPower         { get; set; }
        public uint     LoraSpreadFactor    { get; set; }

    }   // public class JsonStationBootup





    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    //          C L A S S   JsonStationData                                //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public class JsonStationData
    {

        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        // LoRa Packet Header
        public uint     MsgID               { get; set; }
        public String   MsgType             { get; set; }
        public uint     TimeStamp           { get; set; }
        public String   TimeStampFmt        { get; set; }
        public int      RSSI                { get; set; }

        // LoraStationData.DataHeader
        public uint     DevID               { get; set; }
        public uint     SequNum             { get; set; }
        public uint     Uptime              { get; set; }
        public String   UptimeFmt           { get; set; }

        // LoraStationData.DataRec[nIdx]
        public float    Temperature         { get; set; }
        public float    Humidity            { get; set; }
        public uint     MotionActive        { get; set; }
        public uint     MotionActiveTime    { get; set; }
        public uint     MotionActiveCount   { get; set; }
        public uint     LightLevel          { get; set; }
        public float    CarBattLevel        { get; set; }



        //-------------------------------------------------------------------
        //  Methodes
        //-------------------------------------------------------------------

        public int GetDataGen ()
        {

            string  strGenNum;
            int     iGenNum;
            bool    fRes;
            
            strGenNum = MsgType.Substring(MsgType.Length -1);
            fRes = fRes = Int32.TryParse(strGenNum, out iGenNum);
            if ( !fRes )
            {
                iGenNum = -1;
            }
            return (iGenNum);

        }

    }   // public class JsonStationData


}   // namespace LoraPacketViewer



// EOF

