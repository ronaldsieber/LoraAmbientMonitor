/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of Message File Writer

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#ifndef _WIN64
    #include <RH_RF95.h>
#else
    #define _CRT_SECURE_NO_WARNINGS
    typedef  unsigned int  uint;
    #define RH_RF95_MAX_PAYLOAD_LEN 255
#endif
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "LoraPacket.h"
#include "LoraPayloadDecoder.h"
#include "PacketProcessing.h"
#include "MessageQualification.h"
#include "MessageFileWriter.h"
#include "Trace.h"





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Configuration
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------

static  const char*     JSON_REC_DELIMITER  = "\n\n";



//---------------------------------------------------------------------------
//  Macro definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  int             iFdMessageFile_l    = -1;



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  std::string  BuildJsonRec (
    tJsonMessage* pJsonMessage_p);                      // [IN] Ptr to Json Message

static  inline  std::string  Trim (
    std::string& strData_p);





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  MfwOpen
//---------------------------------------------------------------------------

int  MfwOpen (
    const char* pszMsgFileName_p)                       // [IN] Path/Name of MessageFile
{

mode_t  OpenMode;


    if (pszMsgFileName_p == NULL)
    {
        return (-1);
    }

    OpenMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    iFdMessageFile_l = open(pszMsgFileName_p, (O_CREAT | O_WRONLY | O_APPEND | O_SYNC), OpenMode);
    TRACE2("\nOpen MsgFile: pszMsgFileName_p='%s' -> iFdMessageFile_l=%d\n", pszMsgFileName_p, iFdMessageFile_l);
    if (iFdMessageFile_l < 0)
    {
        return (-2);
    }

    return (0);

}



//---------------------------------------------------------------------------
//  MfwClose
//---------------------------------------------------------------------------

int  MfwClose ()
{

    if (iFdMessageFile_l < 0)
    {
        return (-1);
    }

    close(iFdMessageFile_l);
    iFdMessageFile_l = -1;

    return (0);

}



//---------------------------------------------------------------------------
//  MquIsMessageToBeProcessed
//---------------------------------------------------------------------------

int  MfwWriteMessage (
    tJsonMessage* pJsonMessage_p)                       // [IN] Ptr to Json Message
{

std::string  strJsonRecord;
const char*  pszMsgData;
size_t       nMsgDataLen;
int          iRes;


    if (pJsonMessage_p == NULL)
    {
        return (-1);
    }
    if (iFdMessageFile_l < 0)
    {
        return (-2);
    }

    strJsonRecord = BuildJsonRec(pJsonMessage_p);

    pszMsgData  = strJsonRecord.c_str();
    nMsgDataLen = strJsonRecord.length();
    iRes = write(iFdMessageFile_l, pszMsgData, nMsgDataLen);
    TRACE3("\nWrite MsgFile: pszMsgData='%s', nMsgDataLen=%d -> iRes=%d\n", pszMsgData, nMsgDataLen, iRes);
    if (iRes < 0)
    {
        return (-3);
    }

    return (0);

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  BuildJsonRec
//---------------------------------------------------------------------------

static  std::string  BuildJsonRec (
    tJsonMessage* pJsonMessage_p)                       // [IN] Ptr to Json Message
{

std::string  strJsonRecord;


    if (pJsonMessage_p != NULL)
    {
        strJsonRecord = pJsonMessage_p->m_strJsonRecord;
    }
    else
    {
        strJsonRecord = "{ NULL }";
    }

    Trim(strJsonRecord);
    strJsonRecord += JSON_REC_DELIMITER;

    return (strJsonRecord);

}




//---------------------------------------------------------------------------
//  String Trim
//---------------------------------------------------------------------------

static  inline  std::string  Trim (
    std::string& strData_p)
{

    strData_p.erase(0, strData_p.find_first_not_of(" \n\r\t"));
    strData_p.erase(strData_p.find_last_not_of(" \n\r\t")+1);

    return (strData_p);

}




// EOF


