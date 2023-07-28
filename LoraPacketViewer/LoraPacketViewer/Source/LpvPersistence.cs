/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  Persistence (Save/Load) for LoRa Packet Viewer

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


using System;
using System.Collections.Generic;
using Newtonsoft.Json;
using System.IO;

namespace LoraPacketViewer
{

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //                                                                     //
    //          C L A S S   LpvPersistence                                 //
    //                                                                     //
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    public  class  LpvPersistence
    {

        //=================================================================//
        //                                                                 //
        //      D A T A   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-------------------------------------------------------------------
        //  Definitions
        //-------------------------------------------------------------------

        private  readonly  String[]         JSON_REC_DELIMITER_LIST =
                                            {
                                                "\r\n\r\n",
                                                "\n\n"
                                            };

        private  readonly  String           TOPIC_RESTORE = "{Restore}";



        //-------------------------------------------------------------------
        //  Types
        //-------------------------------------------------------------------



        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        private  LpvAppForm                 m_AppForm;





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

        public  LpvPersistence (LpvAppForm AppForm_p)
        {

            m_AppForm = AppForm_p;
            return;

        }



        //-------------------------------------------------------------------
        //  Save JSON PacketLists to File
        //-------------------------------------------------------------------

        public  bool  SavePacketListsToFile (String strJsonListFile_p, List<JsonStationBootup> lstJsonStationBootup_p, List<JsonStationData> lstJsonStationData_p)
        {

            StreamWriter       swrJsonListFile;
            JsonStationBootup  StationBootup;
            JsonStationData    StationData;
            String             strJsonStationData;
            int                iDataRec;


            // create JSON Data File
            try
            {
                swrJsonListFile = new StreamWriter(strJsonListFile_p);
            }
            catch
            {
                return (false);
            }

            m_AppForm.ProgressBarEnable(lstJsonStationBootup_p.Count + lstJsonStationData_p.Count);

            // serialize StationBootupList into JSON Data File
            for (iDataRec=0; iDataRec<lstJsonStationBootup_p.Count; iDataRec++)
            {
                StationBootup = lstJsonStationBootup_p[iDataRec];
                strJsonStationData = JsonConvert.SerializeObject(StationBootup);
                swrJsonListFile.WriteLine(strJsonStationData);
                swrJsonListFile.WriteLine();
                m_AppForm.ProgressBarPerformStep();
            }

            // serialize StationDataList into JSON Data File
            for (iDataRec=0; iDataRec<lstJsonStationData_p.Count; iDataRec++)
            {
                StationData = lstJsonStationData_p[iDataRec];
                strJsonStationData = JsonConvert.SerializeObject(StationData);
                swrJsonListFile.WriteLine(strJsonStationData);
                swrJsonListFile.WriteLine();
                m_AppForm.ProgressBarPerformStep();
            }

            swrJsonListFile.Flush();
            swrJsonListFile.Close();

            m_AppForm.ProgressBarDisable();

            return (true);

        }



        //-------------------------------------------------------------------
        //  Read JSON PacketLists from File
        //-------------------------------------------------------------------

        public  bool  ReadPacketListsFromFile (String strJsonListFile_p)
        {

            StreamReader  srdJsonListFile;
            String        strJsonListFile;
            String[]      astrJsonPackets;
            String        strJsonPacket;
            int           iDataRec;


            // open and read JSON Data File
            try
            {
                srdJsonListFile = new StreamReader(strJsonListFile_p);
                strJsonListFile = srdJsonListFile.ReadToEnd();
                astrJsonPackets = strJsonListFile.Split(JSON_REC_DELIMITER_LIST, StringSplitOptions.RemoveEmptyEntries);
            }
            catch
            {
                return (false);
            }

            m_AppForm.ProgressBarEnable(astrJsonPackets.Length);

			for (iDataRec=0; iDataRec<astrJsonPackets.Length; iDataRec++)
			{
				strJsonPacket = astrJsonPackets[iDataRec].Trim();
				if (strJsonPacket.Length > 0)
				{
					m_AppForm.DispatchLoraNodePackets(TOPIC_RESTORE, strJsonPacket);
				}
                m_AppForm.ProgressBarPerformStep();
			}

            m_AppForm.ProgressBarDisable();

            return (true);

        }

    }   // public  class  LpvPersistence

}   // namespace LoraPacketViewer




// EOF

