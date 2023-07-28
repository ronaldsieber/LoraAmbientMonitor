/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  Dialog Form of LoRa Packet Viewer

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;





namespace LoraPacketViewer
{
    public partial class LpvAppForm : Form
    {

        //=================================================================//
        //                                                                 //
        //      D A T A   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-------------------------------------------------------------------
        //  Types
        //-------------------------------------------------------------------



        //-------------------------------------------------------------------
        //  Configuration
        //-------------------------------------------------------------------

        private  const String       APP_NAME                = "LoRa Packet Viewer";
        private  const uint         APP_VER_MAIN            = 0;                // V1.xx
        private  const uint         APP_VER_REL             = 99;               // Vx.00

        private  const String       MQTT_BROKER_IPADDR      = "127.0.0.1";
        private  const int          MQTT_BROKER_PORTNUM     = 1883;

        private  const uint         MAX_DEVICES             = 16;

        private  readonly Color     COLOR_NOT_CONNECTED     = Color.DarkRed;
        private  readonly Color     COLOR_CONNECTED         = Color.Lime;



        //-------------------------------------------------------------------
        //  Externals/Imports
        //-------------------------------------------------------------------

        [DllImport("user32.dll", EntryPoint="LockWindowUpdate", SetLastError=true, CharSet=CharSet.Auto)]
        private static extern
        IntPtr User32Dll_LockWindowUpdate (IntPtr hWndHandle_p);



        //-------------------------------------------------------------------
        //  Attributes
        //-------------------------------------------------------------------

        private  LpvMqttClient              m_MqttClient;
        private  bool                       m_fConnected;
        private  uint                       m_PacketCounter;
        private  bool                       m_fDevHistAutoScrollSate;
        private  bool                       m_fPacketLogAutoScrollSate;
        private  DateTime                   m_dtConnectionStartTime;

        private  Color                      m_BackColorDefault;
        private  Color                      m_BackColorDataGen1 = Color.LemonChiffon;
        private  Color                      m_BackColorDataGen2 = Color.DarkOrange;

        private  LpvPersistence             m_Persistence;
        private  String                     m_strLogFileName;
        private  StreamWriter               m_StreamWriterLogFile;

        private  List<JsonStationBootup>    m_lstJsonStationBootup;
        private  List<JsonStationData>      m_lstJsonStationData;





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
        //  Form Initialisation
        //-------------------------------------------------------------------

        public LpvAppForm()
        {

            InitializeComponent();
            this.Text = APP_NAME;
            this.BtnCon_Connect.Text = "Connect";
            this.StatIndConnected.BackColor = COLOR_NOT_CONNECTED;
            this.StatIndConnected.Update();

            m_MqttClient = new LpvMqttClient(this);
            m_fConnected = false;
            m_PacketCounter = 0;
            this.Lbl_PacketCounter.Text = "---";

            m_Persistence = new LpvPersistence(this);
            m_strLogFileName = null;
            m_StreamWriterLogFile = null;
            this.ToolStripStatusLabel_LogFile.Text = "Logfile: ---";

            this.ToolStripStatusLabel_ConnectTime.Text = "Online: ---";

            this.ToolStripProgressBar_LoadFile.Visible = false;

            m_fDevHistAutoScrollSate = true;
            m_fPacketLogAutoScrollSate = true;

            #if DEBUG
                ToolStripMenuItem_Test.Visible = true;
            #else
                ToolStripMenuItem_Test.Visible = false;
            #endif

            return;

        }



        //-------------------------------------------------------------------
        //  Form Load
        //-------------------------------------------------------------------

        private void AppForm_Load(object sender, EventArgs e)
        {

            String  strBuildTimeStamp;

            // print Version- and BuildInfo            
            TabPagePacketLog_PrintTextMessage(String.Format("{0} - Version {1:d}.{2:d02}\n", APP_NAME, APP_VER_MAIN, APP_VER_REL));
            #if DEBUG
                TabPagePacketLog_PrintTextMessage("[Debug Version]\n");
            #endif
            strBuildTimeStamp = GetBuildTimestamp();
            TabPagePacketLog_PrintTextMessage("Application Build Timestamp: " + strBuildTimeStamp + "\n\n");

            m_BackColorDefault = this.DataGridViewDeviceSummary.DefaultCellStyle.BackColor;

            // set default URL:PortNum for MQTT Broker
            this.CmBoxCon_HostUrl.Text = MQTT_BROKER_IPADDR;
            this.CmBoxCon_PortNum.Text = MQTT_BROKER_PORTNUM.ToString();

            this.MenuItem_File_StartLogfile.Enabled = true;
            this.MenuItem_File_StopLogfile.Enabled  = false;

            this.ChckBox_DevHist_AutoScroll.Checked = m_fDevHistAutoScrollSate;
            this.ChckBox_PacketLog_AutoScroll.Checked = m_fPacketLogAutoScrollSate;

            // clear all data objects related to received packets (e.g. Lists and GridViews)
            ClearAllData(false);

            return;

        }



        //-------------------------------------------------------------------
        //  Form Close
        //-------------------------------------------------------------------

        private void AppForm_Closing(object sender, FormClosingEventArgs e)
        {

            if ( m_fConnected )
            {
                m_MqttClient.DisconnectFromBroker();
            }

            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      G U I   H A N D L E R                                      //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  OnClick Handler for Button "Connect"
        //-------------------------------------------------------------------

        private void OnClick_BtnCon_Connect(object sender, EventArgs e)
        {

            String  strBrokerHostName;
            String  strBrokerPortNum;
            String  strLogMessage;
            int     iBrokerPort;
            bool    fRes;


            if ( !m_fConnected )
            {
                ClearAllData(true);

                strBrokerHostName = this.CmBoxCon_HostUrl.Text;
                strBrokerPortNum  = this.CmBoxCon_PortNum.Text;
                fRes = Int32.TryParse(strBrokerPortNum, out iBrokerPort);
                if ( !fRes )
                {
                    MessageBox.Show(this, "Invalid PortNumber!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                strLogMessage = String.Format("\nMQTT: Connect to Broker at {0}:{1}... ", strBrokerHostName, iBrokerPort);
                TabPagePacketLog_PrintTextMessage(strLogMessage);

                fRes = m_MqttClient.ConnectToBroker(strBrokerHostName, iBrokerPort);
                if ( fRes )
                {
                    TabPagePacketLog_PrintTextMessage("ok\n\n");
                }
                else
                {
                    TabPagePacketLog_PrintTextMessage("FAILED!\n\n");
                    MessageBox.Show(this, "Connecting to MQTT Broker failed!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
                
                this.BtnCon_Connect.Text = "Disconnect";
                this.CmBoxCon_HostUrl.Enabled = false;
                this.CmBoxCon_PortNum.Enabled = false;
                this.StatIndConnected.BackColor = COLOR_CONNECTED;
                this.StatIndConnected.Update();

                m_dtConnectionStartTime = DateTime.Now;
                this.Timer_500ms.Enabled = true;
                m_fConnected = true;
            }
            else
            {
                TabPagePacketLog_PrintTextMessage("\nMQTT: Disconnect from Broker... ");
                m_MqttClient.DisconnectFromBroker();
                TabPagePacketLog_PrintTextMessage("done\n\n");
                this.BtnCon_Connect.Text = "Connect";
                this.CmBoxCon_HostUrl.Enabled = true;
                this.CmBoxCon_PortNum.Enabled = true;
                this.StatIndConnected.BackColor = COLOR_NOT_CONNECTED;
                this.StatIndConnected.Update();
                this.ToolStripStatusLabel_ConnectTime.Text = "Online: ---";
                m_dtConnectionStartTime = new DateTime(0);
                this.Timer_500ms.Enabled = false;
                m_fConnected = false;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  OnClick Handler for Button "Clear All"
        //-------------------------------------------------------------------

        private void OnClick_BtnData_ClearAll(object sender, EventArgs e)
        {

            DialogResult  DlgRes;


            DlgRes = MessageBox.Show(this, "Are you sure to clear all data?", "Clear All Data",
                                     MessageBoxButtons.YesNo,
                                     MessageBoxIcon.Question,
                                     MessageBoxDefaultButton.Button2);
            if (DlgRes == DialogResult.Yes)
            {
                // clear all data objects related to received packets (e.g. Lists and GridViews)
                ClearAllData(true);
            }

            return;

        }



        //-------------------------------------------------------------------
        //  GUI Handler for MenuItem "File -> Save as..."
        //-------------------------------------------------------------------

        private void OnMenuItem_File_SaveAs(object sender, EventArgs e)
        {

            SaveFileDialog  DlgSaveJsonFile;
            DialogResult    DlgRes;
            String          strJsonFileName;
            bool            fResult;


            DlgSaveJsonFile = new SaveFileDialog();
            DlgSaveJsonFile.Filter = "Json Files (*.json)|*.json|Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
            DlgSaveJsonFile.OverwritePrompt = true;
            //DlgSaveLogfile.InitialDirectory = Directory.GetCurrentDirectory();

            DlgRes = DlgSaveJsonFile.ShowDialog();
            if (DlgRes != DialogResult.OK)
            {
                return;
            }
            strJsonFileName = DlgSaveJsonFile.FileName;

            TabPagePacketLog_PrintTextMessage("\n\n\n******** Save to File ********\n");
            TabPagePacketLog_PrintTextMessage("Filename: " + strJsonFileName + "\n\n");

            fResult = m_Persistence.SavePacketListsToFile(strJsonFileName, m_lstJsonStationBootup, m_lstJsonStationData);
            if ( !fResult )
            {
                MessageBox.Show(this, "Saving Data to File failed!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
                TabPagePacketLog_PrintTextMessage("\n\nERROR: Saving Data to File failed!\n\n");
                return;
            }

            TabPagePacketLog_PrintTextMessage("\n******** Save to File done. ********\n\n\n");

            return;

        }



        //-------------------------------------------------------------------
        //  GUI Handler for MenuItem "File -> Load..."
        //-------------------------------------------------------------------

        private void OnMenuItem_File_Load(object sender, EventArgs e)
        {

            OpenFileDialog  DlgOpenJsonFile;
            DialogResult    DlgRes;
            String          strJsonFileName;
            bool            fResult;


            DlgOpenJsonFile = new OpenFileDialog();
            DlgOpenJsonFile.Filter = "Json Files (*.json)|*.json|Text Files (*.txt)|*.txt|All Files (*.*)|*.*";

            DlgRes = DlgOpenJsonFile.ShowDialog();
            if (DlgRes != DialogResult.OK)
            {
                return;
            }
            strJsonFileName = DlgOpenJsonFile.FileName;

            // clear all data objects related to received packets (e.g. Lists and GridViews)
            ClearAllData(true);

            TabPagePacketLog_PrintTextMessage("\n\n\n******** Load from File ********\n");
            TabPagePacketLog_PrintTextMessage("Filename: " + strJsonFileName + "\n\n");

            fResult = m_Persistence.ReadPacketListsFromFile(strJsonFileName);
            if ( !fResult )
            {
                MessageBox.Show(this, "Loading Data from File failed!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
                TabPagePacketLog_PrintTextMessage("\n\nERROR: Loading Data from File failed!\n\n");
                return;
            }

            TabPagePacketLog_PrintTextMessage("\n******** Load from File done. ********\n\n\n");

            return;

        }


        //-------------------------------------------------------------------
        //  GUI Handler for MenuItem "File -> Start Logfile"
        //-------------------------------------------------------------------

        private void OnMenuItem_File_StartLogfile(object sender, EventArgs e)
        {

            SaveFileDialog  DlgSaveLogfile;
            DialogResult    DlgRes;
            String          strLogFileName;
            bool            fResult;


            DlgSaveLogfile = new SaveFileDialog();
            DlgSaveLogfile.Filter = "Log Files (*.log)|*.log|Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
            DlgSaveLogfile.OverwritePrompt = true;
            //DlgSaveLogfile.InitialDirectory = Directory.GetCurrentDirectory();

            DlgRes = DlgSaveLogfile.ShowDialog();
            if (DlgRes != DialogResult.OK)
            {
                return;
            }
            strLogFileName = DlgSaveLogfile.FileName;

            fResult = StartLogfile(strLogFileName);
            if ( fResult )
            {
                this.MenuItem_File_StartLogfile.Enabled = false;
                this.MenuItem_File_StopLogfile.Enabled  = true;
                this.ToolStripStatusLabel_LogFile.Text = "Logfile: " + strLogFileName;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  GUI Handler for MenuItem "File -> Stop Logfile"
        //-------------------------------------------------------------------

        private void OnMenuItem_File_StopLogfile(object sender, EventArgs e)
        {

            StopLogfile();

            this.MenuItem_File_StartLogfile.Enabled = true;
            this.MenuItem_File_StopLogfile.Enabled  = false;

            this.ToolStripStatusLabel_LogFile.Text = "Logfile: ---";

            return;

        }



        //-------------------------------------------------------------------
        //  GUI Handler for MenuItem "File -> Exit"
        //-------------------------------------------------------------------

        private void StripMenuItem_Click_File_Exit(object sender, EventArgs e)
        {

            StopLogfile();
            System.Windows.Forms.Application.Exit();

            return;

        }



        //-------------------------------------------------------------------
        //  Handler for MenuItem "Test -> RunTestStep"
        //-------------------------------------------------------------------

        private void StripMenuItem_Click_Test_RunTestStep(object sender, EventArgs e)
        {

            TestSuite.RunTestStep(this);
            return;

        }



        //-------------------------------------------------------------------
        //  Handler for MenuItem "Test -> RunTestBlock1"
        //-------------------------------------------------------------------

        private void StripMenuItem_Click_Test_RunTestBlock1(object sender, EventArgs e)
        {

            TestSuite.RunTestBlock1(this);
            return;

        }



        //-------------------------------------------------------------------
        //  Handler for DropDownBox "DevID"
        //-------------------------------------------------------------------

        private void CmbBox_DevHist_OnSelectedIndexChanged(object sender, EventArgs e)
        {

            uint uiSelDevID;

 
            uiSelDevID = (uint)this.CmbBox_DevHist_DevID.SelectedIndex;
            TabPageDeviceHistory_ListHistory(uiSelDevID);

            return;

        }



        //-------------------------------------------------------------------
        //  Handler for CheckBox "DeviceHistory/AutoScroll"
        //-------------------------------------------------------------------

        private void ChckBox_DevHist_AutoScroll_CheckedChanged(object sender, EventArgs e)
        {

            m_fDevHistAutoScrollSate = this.ChckBox_DevHist_AutoScroll.Checked;
            if ( m_fDevHistAutoScrollSate )
            {
                this.DataGridViewDeviceHistory.FirstDisplayedScrollingRowIndex = this.DataGridViewDeviceHistory.RowCount-1;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  Handler for CheckBox "PacketLog/AutoScroll"
        //-------------------------------------------------------------------

        private void ChckBox_PacketLog_AutoScroll_CheckedChanged(object sender, EventArgs e)
        {

            m_fPacketLogAutoScrollSate = this.ChckBox_PacketLog_AutoScroll.Checked;
            if ( m_fPacketLogAutoScrollSate )
            {
                this.RichTextBox_PacketLog.SelectionStart = this.RichTextBox_PacketLog.Text.Length;
                this.RichTextBox_PacketLog.ScrollToCaret();
            }

            return;

        }



        //-------------------------------------------------------------------
        //  Handler for Timer to show ConnectTime
        //-------------------------------------------------------------------

        private void OnTick_Timer(object sender, EventArgs e)
        {

            TimeSpan  tsConnectTime;

            if ( !m_MqttClient.IsConnectedToBroker() )
            {
                this.Timer_500ms.Enabled = false;
                TabPagePacketLog_PrintTextMessage("\nERROR: MQTT Connection Failed");
                MessageBox.Show(this, "Connection to MQTT Broker Failed!", "Error",  MessageBoxButtons.OK, MessageBoxIcon.Error);
                OnClick_BtnCon_Connect(null, null);
                return;
            }

            tsConnectTime = DateTime.Now - m_dtConnectionStartTime;
            this.ToolStripStatusLabel_ConnectTime.Text = "Online: " + tsConnectTime.ToString("hh':'mm':'ss");

            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      I N V O K E   H A N D L E R                                //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  Dispatching of received Lora Node Packets
        //-------------------------------------------------------------------
        public void DispatchLoraNodePackets (String strTopic_p, String strPayload_p)
        {

            JsonTextReader                  JsonPacketReader;
            JsonSerializer                  JsonPacketSerializer;
            JsonPacketInfo.tLoraPacketType  PacketType;
            JsonPacketInfo                  PacketInfo;
            JsonStationBootup               StationBootup;
            JsonStationData                 StationData;
            int                             iDevID;


            // update PacketCounter
            m_PacketCounter++;
            this.Lbl_PacketCounter.Text = m_PacketCounter.ToString();

            // print received Message in LogWindow
            TabPagePacketLog_ProcLoraNodePacket(strTopic_p, strPayload_p);

            // prepare deserialization of received JSON String
            JsonPacketSerializer = new JsonSerializer();

            // get PacketType and DevID from received JSON String
            try
            {
                JsonPacketReader = new JsonTextReader(new StringReader(strPayload_p));          // load received JSON String to JsonTextReader
                PacketInfo = JsonPacketSerializer.Deserialize<JsonPacketInfo>(JsonPacketReader);
                PacketType = PacketInfo.GetPacketType();
            }
            catch
            {
                TabPagePacketLog_PrintTextMessage("\nERROR: Unknow JSON Format (getting PacketType failed)!\n");
                return;
            }
            iDevID = PacketInfo.GetDevID();
            if (iDevID < 0)
            {
                TabPagePacketLog_PrintTextMessage("\nERROR: Unknow JSON Format (invalid DevID)!\n");
                return;
            }

            // process received JSON String depending of PacketType
            switch (PacketType)
            {
                case JsonPacketInfo.tLoraPacketType.kLoraPacketBootup:
                {
                    try
                    {
                        JsonPacketReader = new JsonTextReader(new StringReader(strPayload_p));  // reload received JSON String to JsonTextReader again
                        StationBootup = JsonPacketSerializer.Deserialize<JsonStationBootup>(JsonPacketReader);
                    }
                    catch
                    {
                        TabPagePacketLog_PrintTextMessage("\nERROR: Unknow JSON Format (parsing of Bootup record failed)!\n");
                        return;
                    }

                    // append received Packet to PacketList
                    m_lstJsonStationBootup.Add(StationBootup);

                    // process received Lora Node Packet
                    TabPageBootupSummary_ProcLoraNodePacket(PacketInfo, StationBootup);
                    break;
                }

                case JsonPacketInfo.tLoraPacketType.kLoraPacketDataGen0:
                case JsonPacketInfo.tLoraPacketType.kLoraPacketDataGen1:
                case JsonPacketInfo.tLoraPacketType.kLoraPacketDataGen2:
                {
                    try
                    {
                        JsonPacketReader = new JsonTextReader(new StringReader(strPayload_p));  // reload received JSON String to JsonTextReader again
                        StationData = JsonPacketSerializer.Deserialize<JsonStationData>(JsonPacketReader);
                    }
                    catch
                    {
                        TabPagePacketLog_PrintTextMessage("\nERROR: Unknow JSON Format (parsing of DataGen0/1/2 record failed)!\n");
                        return;
                    }

                    if (StationData.DevID >= MAX_DEVICES)
                    {
                        TabPagePacketLog_PrintTextMessage("\nERROR: Invalid DevID!\n");
                        return;
                    }

                    // append received Packet to PacketList
                    m_lstJsonStationData.Add(StationData);

                    // process received Lora Node Packet
                    TabPageDeviceSummary_ProcLoraNodePacket(PacketInfo, StationData);
                    TabPageDeviceHistory_ProcLoraNodePacket(PacketInfo, StationData);
                    break;
                }

                default:
                {
                    TabPagePacketLog_PrintTextMessage("\nERROR: Unknow JSON Format (unexpectetd PacketType (not Bootup or DataGen0/1/2))!\n");
                    return;
                }
            }

            // update LastMsgID and LastDevID
            this.Lbl_LastMsgID.Text = PacketInfo.MsgID.ToString();
            this.Lbl_LastDevID.Text = PacketInfo.DevID.ToString();

            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      T A B P A G E   B O O T U P   S U M M A R Y                //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  TabPageDeviceSummary: Process received Lora Node Packet
        //-------------------------------------------------------------------

        private void TabPageBootupSummary_ProcLoraNodePacket (JsonPacketInfo PacketInfo_p, JsonStationBootup StationBootup_p)
        {

            int  iRowIdx;


            iRowIdx = PacketInfo_p.GetDevID();
            if ((iRowIdx < 0) || (iRowIdx >= this.DataGridViewBootupSummary.Rows.Count))
            {
                TabPagePacketLog_PrintTextMessage("\nERROR: Invalid DevID!\n");
                return;
            }

            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColDevID"].Value = StationBootup_p.DevID.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColMsgID"].Value = StationBootup_p.MsgID.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColTimeStamp"].Value = StationBootup_p.TimeStampFmt;
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColRssi"].Value = String.Format("{0} dB", StationBootup_p.RSSI);
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColFwVer"].Value = StationBootup_p.FirmwareVer;
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColDataPackCycleTime"].Value = String.Format("{0} s", StationBootup_p.DataPackCycleTm);
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgOledDisp"].Value = StationBootup_p.CfgOledDisplay.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgDhtSensor"].Value = StationBootup_p.CfgDhtSensor.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgSr501Sensor"].Value = StationBootup_p.CfgSr501Sensor.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgAdcLightSensor"].Value = StationBootup_p.CfgAdcLightSensor.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgAdcCarBatAin"].Value = StationBootup_p.CfgAdcCarBatAin.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCfgAsyncLoraEvent"].Value = StationBootup_p.CfgAsyncLoraEvent.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColSr501PauseOnLoraTx"].Value = StationBootup_p.Sr501PauseOnLoraTx.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColCommissioningMode"].Value = StationBootup_p.CommissioningMode.ToString();
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColLoraTxPower"].Value = String.Format("{0} dB", StationBootup_p.LoraTxPower);
            this.DataGridViewBootupSummary.Rows[iRowIdx].Cells["BootSumColSpreadFactor"].Value = StationBootup_p.LoraSpreadFactor.ToString();

            this.DataGridViewBootupSummary.ClearSelection();

            return;

        }



        //-------------------------------------------------------------------
        //  TabPageBootupSummary: OnVisibleChanged
        //-------------------------------------------------------------------

        private void OnVisibleChanged_TabPageBootupSummary(object sender, EventArgs e)
        {

            this.DataGridViewBootupSummary.ClearSelection();
            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      T A B P A G E   D E V I C E   S U M M A R Y                //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  TabPageDeviceSummary: Process received Lora Node Packet
        //-------------------------------------------------------------------

        private void TabPageDeviceSummary_ProcLoraNodePacket (JsonPacketInfo PacketInfo_p, JsonStationData StationData_p)
        {

            int     iRowIdx;
            int     iGenNum;
            String  strGenNum;


            iRowIdx = PacketInfo_p.GetDevID();
            if ((iRowIdx < 0) || (iRowIdx >= this.DataGridViewDeviceSummary.Rows.Count))
            {
                TabPagePacketLog_PrintTextMessage("\nERROR: Invalid DevID!\n");
                return;
            }

            iGenNum = StationData_p.GetDataGen();
            if (iGenNum >= 0)
            {
                strGenNum = iGenNum.ToString();
            }
            else
            {
                strGenNum = "?";
            }

            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColDevID"].Value = StationData_p.DevID.ToString();
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColMsgID"].Value = StationData_p.MsgID.ToString();
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColDataGen"].Value = strGenNum;
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColTimeStamp"].Value = StationData_p.TimeStampFmt;
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColRssi"].Value = String.Format("{0} dB", StationData_p.RSSI);
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColSequNum"].Value = StationData_p.SequNum.ToString();
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColUptime"].Value = StationData_p.UptimeFmt;
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColTemperature"].Value = String.Format("{0:0.0} °C", StationData_p.Temperature);
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColHumidity"].Value = String.Format("{0} %", StationData_p.Humidity);
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColMotionActive"].Value = StationData_p.MotionActive.ToString();
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColMotionActiveCount"].Value = StationData_p.MotionActiveCount.ToString();
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColMotionActiveTime"].Value = String.Format("{0} s", StationData_p.MotionActiveTime);
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColLightLevel"].Value = String.Format("{0} %", StationData_p.LightLevel);
            this.DataGridViewDeviceSummary.Rows[iRowIdx].Cells["DevSumColCarBattLevel"].Value = String.Format("{0:0.0} V", StationData_p.CarBattLevel);

            switch (iGenNum)
            {
                case 1:
                {
                    this.DataGridViewDeviceSummary.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDataGen1;
                    break;
                }

                case 2:
                {
                    this.DataGridViewDeviceSummary.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDataGen2;
                    break;
                }

                default:
                {
                    this.DataGridViewDeviceSummary.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDefault;
                    break;
                }
            }

            this.DataGridViewDeviceSummary.ClearSelection();

            return;

        }



        //-------------------------------------------------------------------
        //  TabPageDeviceSummary: OnVisibleChanged
        //-------------------------------------------------------------------

        private void OnVisibleChanged_TabPageDeviceSummary(object sender, EventArgs e)
        {

            this.DataGridViewDeviceSummary.ClearSelection();
            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      T A B P A G E   D E V I C E   H I S T O R Y                //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  TabPageDeviceHistory: Process received Lora Node Packet
        //-------------------------------------------------------------------

        private void TabPageDeviceHistory_ProcLoraNodePacket (JsonPacketInfo PacketInfo_p, JsonStationData StationData_p)
        {

            uint  uiSelDevID;


            if (StationData_p.DevID >= MAX_DEVICES)
            {
                return;
            }

            // if received Packet matches to selected Device then append Packet to GridView
            uiSelDevID = (uint)this.CmbBox_DevHist_DevID.SelectedIndex;
            if ((StationData_p.DevID == uiSelDevID) || (uiSelDevID == MAX_DEVICES))
            {
                TabPageDeviceHistory_AppendLoraNodePacket(StationData_p);
            }

            return;

        }



        //-------------------------------------------------------------------
        //  TabPageDeviceHistory: List History for selected NodeID
        //-------------------------------------------------------------------
        private void TabPageDeviceHistory_ListHistory (uint uiSelDevID_p)
        {

            JsonStationData  StationData;
            int              iDataRec;


            if (m_lstJsonStationData == null)
            {
                return;
            }

            this.DataGridViewDeviceHistory.Rows.Clear();
            for (iDataRec=0; iDataRec<m_lstJsonStationData.Count; iDataRec++)
            {
                StationData = m_lstJsonStationData[iDataRec];
                if ((StationData.DevID == uiSelDevID_p) || (uiSelDevID_p == MAX_DEVICES))
                {
                    TabPageDeviceHistory_AppendLoraNodePacket(StationData);
                }
            }
            this.DataGridViewDeviceHistory.ClearSelection();

            return;

        }



        //-------------------------------------------------------------------
        //  TabPageDeviceHistory: Append Lora Node Packet to GridView
        //-------------------------------------------------------------------

        private void TabPageDeviceHistory_AppendLoraNodePacket (JsonStationData StationData_p)
        {

            int     iRowIdx;
            int     iGenNum;
            String  strGenNum;


            this.DataGridViewDeviceHistory.Rows.Add();
            iRowIdx = this.DataGridViewDeviceHistory.Rows.Count - 2;
            if (iRowIdx < 0)
            {
                iRowIdx = 0;
            }

            iGenNum = StationData_p.GetDataGen();
            if (iGenNum >= 0)
            {
                strGenNum = iGenNum.ToString();
            }
            else
            {
                strGenNum = "?";
            }

            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColDevID"].Value = StationData_p.DevID.ToString();
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColMsgID"].Value = StationData_p.MsgID.ToString();
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColDataGen"].Value = strGenNum;
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColTimeStamp"].Value = StationData_p.TimeStampFmt;
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColRssi"].Value = String.Format("{0} dB", StationData_p.RSSI);
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColSequNum"].Value = StationData_p.SequNum.ToString();
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColUptime"].Value = StationData_p.UptimeFmt;
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColTemperature"].Value = String.Format("{0:0.0} °C", StationData_p.Temperature);
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColHumidity"].Value = String.Format("{0} %", StationData_p.Humidity);
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColMotionActive"].Value = StationData_p.MotionActive.ToString();
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColMotionActiveCount"].Value = StationData_p.MotionActiveCount.ToString();
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColMotionActiveTime"].Value = String.Format("{0} s", StationData_p.MotionActiveTime);
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColLightLevel"].Value = String.Format("{0} %", StationData_p.LightLevel);
            this.DataGridViewDeviceHistory.Rows[iRowIdx].Cells["DevHisColCarBattLevel"].Value = String.Format("{0:0.0} V", StationData_p.CarBattLevel);

            switch (iGenNum)
            {
                case 1:
                {
                    this.DataGridViewDeviceHistory.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDataGen1;
                    break;
                }

                case 2:
                {
                    this.DataGridViewDeviceHistory.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDataGen2;
                    break;
                }

                default:
                {
                    this.DataGridViewDeviceHistory.Rows[iRowIdx].DefaultCellStyle.BackColor = m_BackColorDefault;
                    break;
                }
            }

            this.DataGridViewDeviceHistory.ClearSelection();

            if ( m_fDevHistAutoScrollSate )
            {
                this.DataGridViewDeviceHistory.FirstDisplayedScrollingRowIndex = this.DataGridViewDeviceHistory.RowCount-1;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  TabPageDeviceHistory: OnVisibleChanged
        //-------------------------------------------------------------------

        private void OnVisibleChanged_TabPageDeviceHistory(object sender, EventArgs e)
        {

            this.DataGridViewDeviceHistory.ClearSelection();
            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      T A B P A G E   P A C K E T   L O G                        //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  TabPagePacketLog: Process received Lora Node Packet
        //-------------------------------------------------------------------

        private void TabPagePacketLog_ProcLoraNodePacket (String strTopic_p, String strPayload_p)
        {

            String  strLogMessage;


            strLogMessage = String.Format("\n---- Packet Counter: {0} ----\n", m_PacketCounter.ToString());
            TabPagePacketLog_PrintTextMessage(strLogMessage);

            strLogMessage = String.Format("Topic: '{0}'\nPayload: {1}\n", strTopic_p, strPayload_p);
            TabPagePacketLog_PrintTextMessage(strLogMessage);

            return;

        }



        //-------------------------------------------------------------------
        //  Print Message String into TabPagePacketLog
        //-------------------------------------------------------------------

        private  void  TabPagePacketLog_PrintTextMessage (String strMessage_p)
        {
            int  iSelctionStartOld;


            // print Message into TabPagePacketLog
            if ( (RichTextBox_PacketLog.SelectionStart == RichTextBox_PacketLog.Text.Length) &&
                 (m_fPacketLogAutoScrollSate == true) )
            {
                this.RichTextBox_PacketLog.AppendText(strMessage_p);
                this.RichTextBox_PacketLog.ScrollToCaret();
            }
            else
            {
                User32Dll_LockWindowUpdate(this.RichTextBox_PacketLog.Handle);
                {
                    iSelctionStartOld = RichTextBox_PacketLog.SelectionStart;
                    this.RichTextBox_PacketLog.AppendText(strMessage_p);
                    this.RichTextBox_PacketLog.SelectionStart = iSelctionStartOld;
                    this.RichTextBox_PacketLog.ScrollToCaret();
                }
                User32Dll_LockWindowUpdate(IntPtr.Zero);
            }

            // write Message into LogFile (if active)
            if ((m_strLogFileName != null) && (m_StreamWriterLogFile != null))
            {
                m_StreamWriterLogFile.WriteLine(strMessage_p);
                m_StreamWriterLogFile.Flush();
            }

            return;

        }





        //-----------------------------------------------------------------//
        //                                                                 //
        //      P R O G R E S S B A R   H A N D L E R                      //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  Enable and Initialize ProgressBar
        //-------------------------------------------------------------------

        public  void  ProgressBarEnable (int iMaxValue_p)
        {

            this.ToolStripProgressBar_LoadFile.Visible = true;
            this.ToolStripProgressBar_LoadFile.Maximum = iMaxValue_p;
            this.ToolStripProgressBar_LoadFile.Step = 1;
            this.ToolStripProgressBar_LoadFile.Value = 0;
            return;

        }



        //-------------------------------------------------------------------
        //  Perform Step
        //-------------------------------------------------------------------

        public  void  ProgressBarPerformStep ()
        {

            this.ToolStripProgressBar_LoadFile.PerformStep();
            return;
        
        }



        //-------------------------------------------------------------------
        //  Disable and Hide ProgressBar
        //-------------------------------------------------------------------

        public  void  ProgressBarDisable ()
        {

            this.ToolStripProgressBar_LoadFile.Value = 0;
            this.ToolStripProgressBar_LoadFile.Visible = false;
            return;

        }



        //-----------------------------------------------------------------//
        //                                                                 //
        //      P R I V A T E   M E T H O D S                              //
        //                                                                 //
        //-----------------------------------------------------------------//

        //-------------------------------------------------------------------
        //  Clear all Data Objects related to received Packets
        //-------------------------------------------------------------------

        private  void  ClearAllData (bool fNoteEntryInLogWindow_p = false)
        {

            uint  uiIdx;


            // adjust BootupSummary GridView to max. number of devices
            this.DataGridViewBootupSummary.Rows.Clear();
            while (this.DataGridViewBootupSummary.Rows.Count < MAX_DEVICES)
            {
                this.DataGridViewBootupSummary.Rows.Add();
            }
            this.DataGridViewBootupSummary.ClearSelection();

            // adjust DeviceSummary GridView to max. number of devices
            this.DataGridViewDeviceSummary.Rows.Clear();
            while (this.DataGridViewDeviceSummary.Rows.Count < MAX_DEVICES)
            {
                this.DataGridViewDeviceSummary.Rows.Add();
            }
            this.DataGridViewDeviceSummary.ClearSelection();

            // fill DropDownBox 'DeviceID' with all valid DevID's
            this.CmbBox_DevHist_DevID.Items.Clear();
            for (uiIdx=0; uiIdx<MAX_DEVICES; uiIdx++)
            {
                this.CmbBox_DevHist_DevID.Items.Add(uiIdx.ToString());
            }
            this.CmbBox_DevHist_DevID.Items.Add("-[all]-");
            this.CmbBox_DevHist_DevID.SelectedIndex = (int)MAX_DEVICES;     // select entry "-[all]-" as default
            this.DataGridViewDeviceHistory.ClearSelection();

            // create lists to store received Packets for (Bootup and StationData) each Device
            m_lstJsonStationBootup = new List<JsonStationBootup>();
            m_lstJsonStationData = new List<JsonStationData>();

            // clear Counter of received Packets
            m_PacketCounter = 0;
            this.Lbl_PacketCounter.Text = "---";

            // clear LastMsgID and LastDevID
            this.Lbl_LastMsgID.Text = "---";
            this.Lbl_LastDevID.Text = "---";

            // note entry in PacketLog
            if ( fNoteEntryInLogWindow_p )
            {
                TabPagePacketLog_PrintTextMessage("\n\n\n******** CLEAR ALL DATA********\n\n\n");
            }

            return;

        }



        //-------------------------------------------------------------------
        //  Start Logfile
        //-------------------------------------------------------------------

        private  bool  StartLogfile (String strLogFileName_p)
        {

            StringBuilder  strbldrInfoText;
            String         strMessage;


            m_strLogFileName = strLogFileName_p;

            // create/open logfile
            try
            {
                m_StreamWriterLogFile = new StreamWriter(m_strLogFileName);
            }
            catch
            {
                m_StreamWriterLogFile = null;
            }
            if (m_StreamWriterLogFile == null)
            {
                return (false);
            }

            // write header to logfile
            strbldrInfoText = new StringBuilder();
            strbldrInfoText.AppendLine  ();
            strbldrInfoText.AppendLine  ("=========================================================");
            strbldrInfoText.AppendFormat("  Logfile created by {0} - Version {1:d}.{2:d02}\n", APP_NAME, APP_VER_MAIN, APP_VER_REL);
            strbldrInfoText.AppendFormat("  Date/Time: {0}" + Environment.NewLine, GetTimeStampString(DateTime.Now));
            strbldrInfoText.AppendLine  ("=========================================================");
            strbldrInfoText.AppendLine  ();
            strMessage = strbldrInfoText.ToString();
            m_StreamWriterLogFile.WriteLine(strMessage);

            // append full text already printed to logwindow to logfile
            m_StreamWriterLogFile.WriteLine(this.RichTextBox_PacketLog.Text);
            m_StreamWriterLogFile.Flush();

            return (true);

        }



        //-------------------------------------------------------------------
        //  Stop Logfile
        //-------------------------------------------------------------------

        private  void  StopLogfile()
        {

            if (m_StreamWriterLogFile != null)
            {
                m_StreamWriterLogFile.Flush();
                m_StreamWriterLogFile.Close();
                m_StreamWriterLogFile = null;
            }

            return;

        }



        //-------------------------------------------------------------------
        //  Get TimeStamp as String
        //-------------------------------------------------------------------

        private  String  GetTimeStampString (DateTime dtTimeStamp_p)
        {

            String  strTimeStamp;


            if (dtTimeStamp_p != new DateTime(0))
            {
                strTimeStamp = String.Format("{0:d04}/{1:d02}/{2:d02} - {3:d02}:{4:d02}:{5:d02}",
                                             dtTimeStamp_p.Year, dtTimeStamp_p.Month, dtTimeStamp_p.Day,
                                             dtTimeStamp_p.Hour, dtTimeStamp_p.Minute, dtTimeStamp_p.Second);
            }
            else
            {
                strTimeStamp = "----/--/-- - --:--:--";
            }


            return (strTimeStamp);

        }



        //-------------------------------------------------------------------
        //  GetBuildTimestamp()
        //-------------------------------------------------------------------
        //
        //  Requirements:
        //  -------------
        //
        //  (1) PreBuildStep:
        //          echo %date% > "$(ProjectDir)BuildTimeStamp.txt"
        //          echo %time% >> "$(ProjectDir)BuildTimeStamp.txt"
        //
        //  (2) BuildTimeStamp.txt Property:
        //          Embedded Resource
        //
        //-------------------------------------------------------------------

        public  String  GetBuildTimestamp()
        {

            Assembly      RuntimeAssembly;
            Stream        strmRessource;
            StreamReader  strmrdrRessource;
            String        strResourceBuildTimeStamp;
            String[]      astrResourceBuildTimeStamp;
            String[]      astrResourceDate;
            String[]      astrResourceTime;
            uint          uiDay;
            uint          uiMonth;
            uint          uiYear;
            uint          uiHour;
            uint          uiMinute;
            uint          uiSecond;
            String        strBuildTimeStamp;


            try
            {
                RuntimeAssembly = Assembly.GetEntryAssembly();

                strmRessource = RuntimeAssembly.GetManifestResourceStream("LoraPacketViewer.BuildTimeStamp.txt");
                strmrdrRessource = new StreamReader(strmRessource);

                strResourceBuildTimeStamp = strmrdrRessource.ReadToEnd();
                astrResourceBuildTimeStamp = strResourceBuildTimeStamp.Split('\n');                     // split into Date and Time

                astrResourceBuildTimeStamp[0] = astrResourceBuildTimeStamp[0].Trim();                   // [0] -> Date
                astrResourceBuildTimeStamp[1] = astrResourceBuildTimeStamp[1].Trim();                   // [1] -> Time
                
                astrResourceDate = astrResourceBuildTimeStamp[0].Split('.');                            // split Date into [dd] [mm] [YYYY]
                uint.TryParse(astrResourceDate[0], out uiDay);
                uint.TryParse(astrResourceDate[1], out uiMonth);
                uint.TryParse(astrResourceDate[2], out uiYear);

                astrResourceTime = astrResourceBuildTimeStamp[1].Split(':');                            // split Time into [HH] [MM] [SS,ms]
                astrResourceTime[2] = astrResourceTime[2].Substring(0, astrResourceTime[2].Length-3);   // remove Millis from Seconds
                uint.TryParse(astrResourceTime[0], out uiHour);
                uint.TryParse(astrResourceTime[1], out uiMinute);
                uint.TryParse(astrResourceTime[2], out uiSecond);

                strBuildTimeStamp = String.Format("{0:d4}/{1:d02}/{2:d02} - {3:d02}:{4:d02}:{5:d02}", uiYear, uiMonth, uiDay, uiHour, uiMinute, uiSecond);
            }
            catch
            {
                strBuildTimeStamp = "???";
            }

            return (strBuildTimeStamp);

        }

    }   // public partial class LpvAppForm : Form


}   // namespace LoraPacketViewer




// EOF


