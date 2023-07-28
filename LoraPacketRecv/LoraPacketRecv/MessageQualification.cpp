/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      LoRa Packet Receiver
  Description:  Implementation of LoRa Message Qualification

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
#include "LoraPacket.h"
#include "LoraPayloadDecoder.h"
#include "PacketProcessing.h"
#include "MessageQualification.h"
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

static  uint32_t    aui32SequNumHistList_l[LORA_DEVICES][SEQU_NUM_HIST_LIST] = { { 0 }, { 0 } };



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------

static  int  MquClearMessageList (
    uint uiDevID_p);


static  int  MquGetHighestSequNum (
    uint uiDevID_p);


static  int  MquIsSequNumInMessageList (
    uint uiDevID_p,
    uint32_t ui32SequNum_p);


static  int  MquAppendSequNum (
    uint uiDevID_p,
    uint32_t ui32SequNum_p);





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  MquInitialize
//---------------------------------------------------------------------------

void  MquInitialize ()
{

uint  uiIdxDevID;
uint  uiIdxSequNum;


    for (uiIdxDevID=0; uiIdxDevID<LORA_DEVICES; uiIdxDevID++)
    {
        for (uiIdxSequNum=0; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
        {
            aui32SequNumHistList_l[uiIdxDevID][uiIdxSequNum] = 0;
        }
    }

    return;

}



//---------------------------------------------------------------------------
//  MquIsMessageToBeProcessed
//---------------------------------------------------------------------------

int  MquIsMessageToBeProcessed (
    tJsonMessage* pJsonMessage_p)                       // [IN]     Ptr to Json Message
{

tLoraPacketType  PacketType;
uint             uiDevID;
uint32_t         ui32SequNum;
uint32_t         ui32HighestSequNum;
int              iIsSequNumInMessageList;
int              iMessageToBeProcessed;


    if (pJsonMessage_p == NULL)
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    PacketType  = pJsonMessage_p->m_PacketType;
    uiDevID     = (uint)pJsonMessage_p->m_ui8DevID;
    ui32SequNum = pJsonMessage_p->m_ui32SequNum;

    if (uiDevID >= LORA_DEVICES)
    {
        TRACE0("ERROR: Invalid Parameter!\n");
        return (-1);
    }

    switch (PacketType)
    {
        case kLoraPacketBootup:
        {
            // when the SensorDevice is reset, the entire previous sequence history loses its validity
            MquClearMessageList(uiDevID);
            iMessageToBeProcessed = 1;          // always process <kLoraPacketBootup> message
            break;
        }

        case kLoraPacketDataGen0:
        {
            ui32HighestSequNum = (uint32_t)MquGetHighestSequNum(uiDevID);
            if (ui32SequNum < ui32HighestSequNum)
            {
                // a jump back in the sequence history means a reset of the SensorDevice with a simultaneous loss of the bootup message
                MquClearMessageList(uiDevID);
            }
            MquAppendSequNum(uiDevID, ui32SequNum);
            iMessageToBeProcessed = 1;          // always process <kLoraPacketDataGen0> message
            break;
        }

        case kLoraPacketDataGen1:
        case kLoraPacketDataGen2:
        {
            iIsSequNumInMessageList = MquIsSequNumInMessageList(uiDevID, ui32SequNum);
            if (iIsSequNumInMessageList == 1)
            {
                // message was already processed -> ignore duplicate
                iMessageToBeProcessed = 0;
            }
            else
            {
                // process message copy after loss of original Gen0 message
                MquAppendSequNum(uiDevID, ui32SequNum);
                iMessageToBeProcessed = 1;
            }
            break;
        }

        default:
        {
            TRACE1("ERROR: Unexpected PacketType (%d)!\n", (int)pJsonMessage_p->m_PacketType);
            iMessageToBeProcessed = -2;
            break;
        }
    }

    return (iMessageToBeProcessed);

}



//---------------------------------------------------------------------------
//  MquPrintSequNumHistList
//---------------------------------------------------------------------------

void  MquPrintSequNumHistList ()
{

uint  uiIdxDevID;
uint  uiIdxSequNum;


    printf("\n");
    printf("            ");

    for (uiIdxSequNum=0; uiIdxSequNum<(SEQU_NUM_HIST_LIST-1); uiIdxSequNum++)
    {
        printf("S[%d]   ", (int)uiIdxSequNum-(SEQU_NUM_HIST_LIST-1));
    }
    printf(" S[0]\n");

    for (uiIdxDevID=0; uiIdxDevID<LORA_DEVICES; uiIdxDevID++)
    {
        printf("DevID[%02u]:   ", uiIdxDevID);
        for (uiIdxSequNum=0; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
        {
            printf("%4u    ", aui32SequNumHistList_l[uiIdxDevID][uiIdxSequNum]);
        }
        printf("\n");
    }

    return;

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   F U N C T I O N S                              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  MquClearMessageList
//---------------------------------------------------------------------------

static  int  MquClearMessageList (
    uint uiDevID_p)
{

uint  uiIdxSequNum;


    if (uiDevID_p >= LORA_DEVICES)
    {
        return (-1);
    }

    for (uiIdxSequNum=0; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
    {
        aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum] = 0;
    }

    return (0);

}



//---------------------------------------------------------------------------
//  MquGetHighestSequNum
//---------------------------------------------------------------------------

static  int  MquGetHighestSequNum (
    uint uiDevID_p)
{

uint32_t  ui32HighestSequNum;
uint      uiIdxSequNum;


    if (uiDevID_p >= LORA_DEVICES)
    {
        return (-1);
    }

    ui32HighestSequNum = 0;
    for (uiIdxSequNum=0; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
    {
        if (ui32HighestSequNum < aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum])
        {
            ui32HighestSequNum = aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum];
        }
    }

    return ((int)ui32HighestSequNum);

}



//---------------------------------------------------------------------------
//  MquIsSequNumInMessageList
//---------------------------------------------------------------------------

static  int  MquIsSequNumInMessageList (
    uint uiDevID_p,
    uint32_t ui32SequNum_p)
{

uint  uiIdxSequNum;


    if (uiDevID_p >= LORA_DEVICES)
    {
        return (-1);
    }

    for (uiIdxSequNum=0; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
    {
        if (aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum] == ui32SequNum_p)
        {
            return (1);
        }
    }

    return (0);

}



//---------------------------------------------------------------------------
//  MquAppendSequNum
//---------------------------------------------------------------------------

static  int  MquAppendSequNum (
    uint uiDevID_p,
    uint32_t ui32SequNum_p)
{

uint  uiIdxSequNum;


    if (uiDevID_p >= LORA_DEVICES)
    {
        return (-1);
    }

    for (uiIdxSequNum=1; uiIdxSequNum<SEQU_NUM_HIST_LIST; uiIdxSequNum++)
    {
        aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum-1] = aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum];
    }
    aui32SequNumHistList_l[uiDevID_p][uiIdxSequNum-1] = ui32SequNum_p;

    return (0);

}



// EOF


