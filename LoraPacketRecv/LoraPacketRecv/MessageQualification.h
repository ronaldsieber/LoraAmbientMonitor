/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Declarations for LoRa Message Qualification

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _MESSAGEQUALIFICATION_H_
#define _MESSAGEQUALIFICATION_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------

const  uint  LORA_DEVICES           = 16;
const  uint  SEQU_NUM_HIST_LIST     = 10;



//---------------------------------------------------------------------------
//  Type definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

void  MquInitialize ();

int  MquIsMessageToBeProcessed (
    tJsonMessage* pJsonMessage_p);                      // [IN] Ptr to Json Message

void  MquPrintSequNumHistList ();



#endif  // #ifndef _MESSAGEQUALIFICATION_H_


// EOF

