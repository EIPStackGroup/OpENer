// Scraps of this module were pulled from the following demo program:

//*****************************************************************************
//
// enet_lwip.c - Sample WebServer Application using lwIP.
//
// Copyright (c) 2007-2008 Luminary Micro, Inc.  All rights reserved.
// Software License Agreement
//
// Luminary Micro, Inc. (LMI) is supplying this software for use solely and
// exclusively on LMI's micro controller products.
//
// The software is owned by LMI and/or its suppliers, and is protected under
// applicable copyright laws.  All rights are reserved.  You may not combine
// this software with "viral" open-source software in order to form a larger
// program.  Any use in violation of the foregoing restrictions may subject
// the user to criminal sanctions under applicable laws, as well as to civil
// liability for the breach of the terms and conditions of this license.
//
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// LMI SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
//
// This is part of revision 3618 of the EK-LM3S6965 Rev C Firmware Package.
//
//*****************************************************************************

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_nvic.h"
#include "hw_types.h"
#include "driverlib/ethernet.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "lwiplib.h"

#include "flashmgr.h"
#include "networkhandler.h"
#include <opener_api.h>
#include "cipcommon.h"
#include "random.h"
#include <trace.h>

//define here instead of ptpd.h
volatile unsigned long g_ulSystemTimeSecfunctiononds;
volatile unsigned long g_ulSystemTimeNanoSeconds;

// cast an int as a struct_inaddr (check the "inet_ntoa" man page -- it wants a struct_inaddr passed by value, not an int)
// static IP-Address?
//#define useStaticIP 1
#define useStaticIP 0

#ifndef useStaticIP
#error "useStaticIP undefined"
#endif

struct parm
{
  int useStatic; // 1 use static IP address, 0 use DHCP
  unsigned long ip; // my ip address
  unsigned long nm; // net mask
  unsigned long gw; // gateway ip address
};


//*****************************************************************************
//
// Defines for setting up the system clock.
//
//*****************************************************************************
#define SYSTICKHZ               100
#define SYSTICKMS               (1000 / SYSTICKHZ)
#define SYSTICKUS               (1000000 / SYSTICKHZ)
#define SYSTICKNS               (1000000000 / SYSTICKHZ)


//*****************************************************************************
//
// Interrupt priority definitions.  The top 3 bits of these values are
// significant with lower values indicating higher priority interrupts.
//
//*****************************************************************************
#define SYSTICK_INT_PRIORITY    0xF0
#define ETHERNET_INT_PRIORITY   0xC2

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
  {
  }
#endif

// change my IP address etc.
void
setCIPaddress(unsigned long addr, // my IP address, in network order
    unsigned long mask, // netmask, in network order
    unsigned long gw) // gateway, in network order
{
  struct in_addr inAddr;
  inAddr.s_addr = addr;
  char acIPAddr[16];
  strncpy(acIPAddr, inet_ntoa(inAddr), 16);
  inAddr.s_addr = mask;
  char acNetMask[16];
  strncpy(acNetMask, inet_ntoa(inAddr), 16);
  inAddr.s_addr = gw;
  char acGW[16];
  strncpy(acGW, inet_ntoa(inAddr), 16);

  configureNetworkInterface(acIPAddr, acNetMask, acGW);
  configureDomainName("test");
  configureHostName("karl");
}

// this gets called every 100 usec by the lwip timer handler
void
lwIPHostTimerHandler(void)
{
  static unsigned long ulLastIPAddress = 0;
  unsigned long ulIPAddress;
  unsigned long ulNetmask;
  unsigned long ulGateway;

  ulIPAddress = lwIPLocalIPAddrGet();

  if (ulLastIPAddress != ulIPAddress)
    {
      ulLastIPAddress = ulIPAddress;
      ulNetmask = lwIPLocalNetMaskGet();
      ulGateway = lwIPLocalGWAddrGet();
      setCIPaddress(ulIPAddress, ulNetmask, ulGateway);
    }
}

int
main(void)
{
  int i;
  unsigned long ulUser0, ulUser1;
  unsigned char pucMACArray[8];

  unsigned long ip, nm, gw;
  int valid;

  struct parm parm;
  struct parm *p;
  unsigned long *pl;

  IntPriorityGroupingSet(4);

  //
  // Set the clocking to run directly from the crystal.
  //
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN
                  | SYSCTL_XTAL_8MHZ);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
  SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

  // Enable Port F for Ethernet LEDs.
  //  LED0        Bit 3   Output
  //  LED1        Bit 2   Output
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_DIR_MODE_HW);
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3,
                  GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

  //
  // Configure the GPIOs used to read the state of the on-board push buttons.
  //
  GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
                  | GPIO_PIN_3);
  GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
                  | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
                  GPIO_PIN_TYPE_STD_WPU);

  // configure the user LED output
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);



  //
  // Configure SysTick for a 100 Hz (10 ms) interrupt.
  //
  SysTickPeriodSet(SysCtlClockGet() / SYSTICKHZ);
  SysTickEnable();
  SysTickIntEnable();

  //
  // Enable processor interrupts.
  //
  IntMasterEnable();

  //
  // Configure the hardware MAC address for Ethernet Controller filtering of
  // incoming packets.
  //
  // For the LM3S6965 Evaluation Kit, the MAC address will be stored in the
  // non-volatile USER0 and USER1 registers.  These registers can be read
  // using the FlashUserGet function, as illustrated below.
  //
  FlashUserGet(&ulUser0, &ulUser1);
  if ((ulUser0 == 0xffffffff) || (ulUser1 == 0xffffffff))
    {
      //
      // We should never get here.  This is an error if the MAC address has
      // not been programmed into the device.  Exit the program.
      //

      while (1)
        {
        }
    }

  //
  // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
  // address needed to program the hardware registers, then program the MAC
  // address into the Ethernet Controller registers.
  //

  pucMACArray[0] = ((ulUser0 >> 0) & 0xff);
  pucMACArray[1] = ((ulUser0 >> 8) & 0xff);
  pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
  pucMACArray[3] = ((ulUser1 >> 0) & 0xff);
  pucMACArray[4] = ((ulUser1 >> 8) & 0xff);
  pucMACArray[5] = ((ulUser1 >> 16) & 0xff);

  //////////////////////////////////////////////////////////
  // YOU MUST SET THESE TO VALID VALUES FOR YOUR LOCATION //
  //////////////////////////////////////////////////////////
  configureMACAddress(pucMACArray);

  //
  // Initialze the lwIP library
  //


  pl = findNextEmptyParameterBlock();
  p = (struct parm *) (pl + 1);

  if (useStaticIP)
    {
      OPENER_TRACE_INFO("using static IP address\n");

      ip = 0x8083BAC9; //128.130.200.201
      nm = 0xFFFFFF00;
      gw = 0x8083BA01;
      valid = 7;
      lwIPInit(pucMACArray, ip, nm, gw, IPADDR_USE_STATIC);
    }
  else
    {
      //
      // Initialze the lwIP library, using DHCP.
      //
      OPENER_TRACE_INFO("using DHCP\n");
      valid = 0; //0
      lwIPInit(pucMACArray, 0, 0, 0, IPADDR_USE_DHCP);
    }

    //change time-interval value for call of updateElMeasuringAndMeteringData
    //in SysTickIntHandler-method to show/provide correct values (see line 370)

  IntPrioritySet(INT_ETH, ETHERNET_INT_PRIORITY);
  IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

  /*for a real device the serial number should be unique per device */
  setDeviceSerialNumber(123456789);

  /* Setup the CIP Layer */
  CIP_Init(365);
  IntMasterDisable();

  IntMasterEnable();
  Start_NetworkHandler();

  // this is a simple command interpreter which reads from serial port 0
  // it is used to set a static IP address
  while (1)
    {

    }
}

/* implement missing functions rand and srand */
int _EXFUN(rand,(_VOID))
{
  return nextXorShiftUInt32();
}

_VOID   _EXFUN(srand,(unsigned __seed))
{
  setXorShiftSeed(__seed);
}



//*****************************************************************************
//
// The interrupt handler for the SysTick interrupt.
// Entry here directly from the interrupt vector
// This interrupt occurs once every 10 milliseconds
//*****************************************************************************
void SysTickIntHandler(void) {

  //
  // Update internal time and set PPS output, if needed.
  //
  g_ulSystemTimeNanoSeconds += SYSTICKNS;

  //
  // Call the lwIP timer handler.
  //
  lwIPTimer(SYSTICKMS);
}

