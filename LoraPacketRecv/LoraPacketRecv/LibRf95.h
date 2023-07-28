/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Declarations for RF95 Function Library

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/

#ifndef _LIBRF95_H_
#define _LIBRF95_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

void  RF95Setup (uint8_t ui8GpioPinCS_p, uint8_t ui8GpioPinIRQ_p, uint8_t ui8GpioPinRST_p);
int   RF95ResetModule ();
int   RF95InitModule (int8_t i8TxPower_p, float flCentreFrequ_p);
bool  RF95GetRecvDataPacket (uint8_t* pabRxDataBuff_p, uint* puiRxDataBuffLen_p, int8_t* pi8LastRssi_p);
int   RF95DiagDumpRegs ();
int   RF95DiagPrintConfig ();





#endif  // #ifndef _LIBRF95_H_


// EOF

