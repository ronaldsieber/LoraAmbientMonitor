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

#ifndef _GPIOIRQ_H_
#define _GPIOIRQ_H_



//---------------------------------------------------------------------------
//  Constant definitions
//---------------------------------------------------------------------------

#define GPIO_DIR_IN         0
#define GPIO_DIR_OUT        1

#define GPIO_EDGE_FALLING   0
#define GPIO_EDGE_RISING    1



//---------------------------------------------------------------------------
//  Prototypes of public functions
//---------------------------------------------------------------------------

void  GpioInit    (void);
int   GpioOpen    (unsigned int uiGpioNum_p, unsigned int uiDirection_p);
int   GpioWrite   (unsigned int uiGpioNum_p, int iValue_p);
int   GpioRead    (unsigned int uiGpioNum_p);
int   GpioSetEdge (unsigned int uiGpioNum_p, unsigned int uiGpioEdge_p);
int   GpioGetFD   (unsigned int uiGpioNum_p);




#endif  // #ifndef _GPIOIRQ_H_


// EOF

