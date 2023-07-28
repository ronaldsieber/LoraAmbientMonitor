/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Declarations for Message File Writer

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _MESSAGEFILEWRITER_H_
#define _MESSAGEFILEWRITER_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

int  MfwOpen (
    const char* pszMsgFileName_p);                      // [IN] Path/Name of MessageFile

int  MfwClose ();

int  MfwWriteMessage (
    tJsonMessage* pJsonMessage_p);                      // [IN] Ptr to Json Message




#endif  // #ifndef _MESSAGEFILEWRITER_H_


// EOF

