/****************************************************************************

  Copyright (c) 2023 Ronald Sieber

  Project:      GPIO IRQ Handling on Raspi
  Description:  Implementation of GPIO IRQ Handling on Raspi

  -------------------------------------------------------------------------

    Based on:
    https://www.iot-programmer.com/index.php/books/22-raspberry-pi-and-the-iot-in-c/chapters-raspberry-pi-and-the-iot-in-c/55-raspberry-pi-and-the-iot-in-c-input-and-interrupts?showall=1

  -------------------------------------------------------------------------

  Revision History:

  2023/03/25 -rs:   V1.00 Initial version

****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "GpioIrq.h"





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

#define BUFFER_SIZE     50



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Macro definitions
//---------------------------------------------------------------------------

#define tabentries(a)   (sizeof(a)/sizeof(*(a)))



//---------------------------------------------------------------------------
//  Local types
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Global variables
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//  Local variables
//---------------------------------------------------------------------------

static  int     aiGpioFd_l[32] = { 0 };             // Support GPIO0..GPIO31



//---------------------------------------------------------------------------
//  Prototypes of internal functions
//---------------------------------------------------------------------------





//=========================================================================//
//                                                                         //
//          P U B L I C   F U N C T I O N S                                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Initialize GPIO Library
//---------------------------------------------------------------------------

void  GpioInit (void)
{

    memset(aiGpioFd_l, 0, sizeof(aiGpioFd_l));

    return;

}



//---------------------------------------------------------------------------
//  Open GPIO Port
//---------------------------------------------------------------------------

int GpioOpen (unsigned int uiGpioNum_p, unsigned int uiDirection_p)
{

int          iFdGpio;
char         szBuff[BUFFER_SIZE];
int          iLen;
const char*  pszDirection;
int          iOpenFlag;

    // check calling parameter for validity
    if (uiGpioNum_p >= tabentries(aiGpioFd_l))
    {
        return (-1);
    }
    if (uiDirection_p > 1)
    {
        return (-2);
    }

    // check if GPIO is already open, close and remove if necessary
    if (aiGpioFd_l[uiGpioNum_p] != 0)
    {
        close(aiGpioFd_l[uiGpioNum_p]);
        iFdGpio = open("/sys/class/gpio/unexport", O_WRONLY);
        if (iFdGpio < 0)
        {
            printf("ERROR: Failed to open '%s', Error=%d\n", "/sys/class/gpio/unexport", aiGpioFd_l[uiGpioNum_p]);
            return (-3);
        }
        iLen = snprintf(szBuff, sizeof(szBuff), "%u", uiGpioNum_p);
        write(iFdGpio, szBuff, iLen);
        close(iFdGpio);
        aiGpioFd_l[uiGpioNum_p] = 0;
    }

    // make GPIO available (export GPIO)
    iFdGpio = open("/sys/class/gpio/export", O_WRONLY);
    if (iFdGpio < 0)
    {
        printf("ERROR: Failed to open '%s', Error=%d\n", "/sys/class/gpio/export", iFdGpio);
        return (-4);
    }
    iLen = snprintf(szBuff, sizeof(szBuff), "%u", uiGpioNum_p);
    write(iFdGpio, szBuff, iLen);
    close(iFdGpio);

    // configure GPIO data direction (in/out)
    if (uiDirection_p == GPIO_DIR_IN)
    {
        pszDirection = "in";
    }
    else
    {
        pszDirection = "out";
    }
    snprintf(szBuff, sizeof(szBuff), "/sys/class/gpio/gpio%d/direction", uiGpioNum_p);
    iFdGpio = open(szBuff, O_WRONLY);
    if (iFdGpio < 0)
    {
        printf("ERROR: Failed to open '%s', Error=%d\n", szBuff, iFdGpio);
        return (-5);
    }
    write(iFdGpio, pszDirection, strlen(pszDirection)+1);
    close(iFdGpio);

    // open GPIO for data access (read/write)
    if (uiDirection_p == GPIO_DIR_IN)
    {
        iOpenFlag = O_RDONLY;
    }
    else
    {
        iOpenFlag = O_WRONLY;
    }
    snprintf(szBuff, sizeof(szBuff), "/sys/class/gpio/gpio%d/value", uiGpioNum_p);
    aiGpioFd_l[uiGpioNum_p] = open(szBuff, iOpenFlag);
    if (aiGpioFd_l[uiGpioNum_p] < 0)
    {
        printf("ERROR: Failed to open '%s', Error=%d\n", szBuff, aiGpioFd_l[uiGpioNum_p]);
        return (-6);
    }

    return (0);

}



//---------------------------------------------------------------------------
//  Write Data to GPIO Port
//---------------------------------------------------------------------------

int  GpioWrite (unsigned int uiGpioNum_p, int iValue_p)
{

const char*  pszValue;

    // check calling parameter for validity
    if (uiGpioNum_p >= tabentries(aiGpioFd_l))
    {
        return (-1);
    }

    if (iValue_p == 0)
    {
        pszValue = "0";
    }
    else
    {
        pszValue = "1";
    }
    write(aiGpioFd_l[uiGpioNum_p], pszValue, strlen(pszValue));
    lseek(aiGpioFd_l[uiGpioNum_p], 0, SEEK_SET);

    return (0);

}



//---------------------------------------------------------------------------
//  Read Data from GPIO Port
//---------------------------------------------------------------------------

int  GpioRead (unsigned int uiGpioNum_p)
{

char  szGpioValue[3];

    // check calling parameter for validity
    if (uiGpioNum_p >= tabentries(aiGpioFd_l))
    {
        return (-1);
    }

    lseek(aiGpioFd_l[uiGpioNum_p], 0, SEEK_SET);
    read(aiGpioFd_l[uiGpioNum_p], szGpioValue, sizeof(szGpioValue));
    lseek(aiGpioFd_l[uiGpioNum_p], 0, SEEK_SET);
    if (szGpioValue[0] == '0')
    {
        return (0);
    }
    else
    {
        return (1);
    }

}



//---------------------------------------------------------------------------
//  Configure Edge Detection for GPIO Port
//---------------------------------------------------------------------------

int  GpioSetEdge (unsigned int uiGpioNum_p, unsigned int uiGpioEdge_p)
{

const char*  pszEdge;
char         szBuff[BUFFER_SIZE];
int          iFdGpio;

    // check calling parameter for validity
    if (uiGpioNum_p >= tabentries(aiGpioFd_l))
    {
        return (-1);
    }

    if (uiGpioEdge_p == GPIO_EDGE_FALLING)
    {
        pszEdge = "falling";
    }
    else
    {
        pszEdge = "rising";
    }

    snprintf(szBuff, sizeof(szBuff), "/sys/class/gpio/gpio%d/edge", uiGpioNum_p);
    iFdGpio = open(szBuff, O_WRONLY);
    if (iFdGpio < 0)
    {
        printf("ERROR: Failed to open GPIO '%s', Error=%d\n", szBuff, iFdGpio);
        return (-2);
    }
    write(iFdGpio, pszEdge, strlen(pszEdge)+1);
    close(iFdGpio);

    return (0);

}



//---------------------------------------------------------------------------
//  Get FD for GPIO Port
//---------------------------------------------------------------------------

int  GpioGetFD (unsigned int uiGpioNum_p)
{

    // check calling parameter for validity
    if (uiGpioNum_p >= tabentries(aiGpioFd_l))
    {
        return (-1);
    }

    return (aiGpioFd_l[uiGpioNum_p]);

}



// EOF






