/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Viewer
  Description:  Application Main Module

  -------------------------------------------------------------------------

  Dependencies:

    This Project uses following Packages, installed with NuGet Packet Manager:

    Package:  M2Mqtt
    Version:  4.3.0
    Author:   Paolo Patierno

    Package:  Newtonsoft.Json
    Version:  13.0.2
    Author:   James Newton-King

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;





namespace LoraPacketViewer
{
    static class Program
    {

        //=================================================================//
        //                                                                 //
        //      D A T A   S E C T I O N                                    //
        //                                                                 //
        //=================================================================//

        //-------------------------------------------------------------------
        //  Configuration
        //-------------------------------------------------------------------





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
        //  Application Main
        //-------------------------------------------------------------------

        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new LpvAppForm());

            return;

        }

    }   // static class Program

}   // namespace LoraPacketViewer




// EOF

