// Scraps of this module were pulled from the following demo program:

//*****************************************************************************
//
// enet_lwip.c - Sample WebServer Application using lwIP.
//
// Copyright (c) 2007-2008 Luminary Micro, Inc.  All rights reserved.
// Software License Agreement
//
// Luminary Micro, Inc. (LMI) is supplying this software for use solely and
// exclusively on LMI's microcontroller products.
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

#include "local.h"
#include "flashmgr.h"
#include "networkhandler.h"
#include <opener_api.h>
#include "cipcommon.h"
#include "random.h"
#include <trace.h>
#include "../../cip_energy/ElEnergyData.h"
#include "eminterface.h"

int callocsize = CALLOCSIZE;
char callocmem[CALLOCSIZE];

//define here instead of ptpd.h
volatile unsigned long g_ulSystemTimeSeconds;
volatile unsigned long g_ulSystemTimeNanoSeconds;

// cast an int as a struct_inaddr (check the "inet_ntoa" man page -- it wants a struct_inaddr passed by value, not an int)
// static IP-Address?
//#define useStaticIP 1
#define useStaticIP 0

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

void DisplayIPAddress(unsigned long ipaddr, unsigned long ulCol,
                 unsigned long ulRow);
void DisplayOdo(EIP_INT64 pa_nValue, unsigned long pa_nRow);


void DisplayRealVal(double pa_fValue, const char *pcPrefix, const char *pcPostfix, unsigned long pa_nRow, unsigned long pa_nCol);


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
      DisplayIPAddress((ulIPAddress), 36, 8);
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
  // Initialize ADC
  //
  EMInterfaceInit();
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


  pl = parmFind();
  p = (struct parm *) (pl + 1);

  if (useStaticIP)
    {
      OPENER_TRACE_INFO("using static IP address\n");

      ip = 0x8083BAC9; //128.130.200.201
//      ip = 0x8083BACB; //128.130.200.203
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


  // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    //
    //
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);
    RIT128x96x4StringDraw("OpENer w/ CIP-Energy", 4, 0, 15);
    RIT128x96x4StringDraw("-----------------------", 0, 14, 15);
    RIT128x96x4StringDraw("IP:   ", 0, 8, 15);
    DisplayIPAddress(htonl(ip), 36, 8);

    RIT128x96x4StringDraw("  TWh GWh MWh kWh  Wh  ", 0, 20, 15);

    RIT128x96x4StringDraw("factor 1000 applies    ", 0, 54, 10);
    RIT128x96x4StringDraw("to metered values     ", 0, 62, 10);
    //change time-interval value for call of updateElMeasuringAndMeteringData
    //in SysTickIntHandler-method to show/provide correct values (see line 370)

  IntPrioritySet(INT_ETH, ETHERNET_INT_PRIORITY);
  IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

  /*for a real device the serial number should be unique per device */
  setDeviceSerialNumber(123456789);

  /* Setup the CIP Layer */
  CIP_Init(365);
  IntMasterDisable();
  CIP_BaseEnergy_Init();
  CIP_ElEnergy_Init();

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
  return RandomNumber();
}

_VOID   _EXFUN(srand,(unsigned __seed))
{
  RandomAddEntropy(__seed);
  RandomSeed();
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
  if(g_ulSystemTimeNanoSeconds >= 1000000000) //1 second interval
      {
          //GPIOPinWrite(PPS_GPIO_BASE, PPS_GPIO_PIN, PPS_GPIO_PIN); ???
          g_ulSystemTimeNanoSeconds -= 1000000000;
          g_ulSystemTimeSeconds += 1;

          //TODO: set interval back to 1 second
          updateElMeasuringAndMeteringData(1000.0, EMInterfaceGetVoltage(), EMInterfaceGetCurrent());

          RIT128x96x4StringDraw("N", 0, 28, 15);
          DisplayOdo(g_nBE_TotalEnergyValue, 28);

          RIT128x96x4StringDraw("C", 0, 36, 15);
          DisplayOdo(g_nBE_ConsumedEnergyValue, 36);

          RIT128x96x4StringDraw("P", 0, 44, 15);
          DisplayOdo(g_nBE_ProducedEnergyValue, 44);

          DisplayRealVal(g_astEE_ObjInstanceAttribs[eELEL1Current].m_nAttribValue.m_fReal, "I:", "A", 74, 0);
          DisplayRealVal(g_astEE_ObjInstanceAttribs[eELEL1toNVoltage].m_nAttribValue.m_fReal, "U:", "V", 74, 66);

          DisplayRealVal(g_astEE_ObjInstanceAttribs[eELELineFrequency].m_nAttribValue.m_fReal, "f:", "Hz", 82, 0);
          DisplayRealVal(g_astEE_ObjInstanceAttribs[eELEL1RealPower].m_nAttribValue.m_fReal, "P:", "W", 82, 66);

      }


  //
  // Call the lwIP timer handler.
  //
  lwIPTimer(SYSTICKMS);
}


//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(unsigned long ipaddr, unsigned long ulCol,
                 unsigned long ulRow)
{
    char pucBuf[16];
    unsigned char *pucTemp = (unsigned char *)&ipaddr;

    //
    // Convert the IP Address into a string.
    //
    sprintf(pucBuf, "%d.%d.%d.%d", pucTemp[0], pucTemp[1], pucTemp[2],
             pucTemp[3]);

    //
    // Display the string.
    //
    RIT128x96x4StringDraw(pucBuf, ulCol, ulRow, 15);
}


//*****************************************************************************
//
// Display a 5 digit odometer
//
//*****************************************************************************
void
DisplayOdo(EIP_INT64 pa_nValue, unsigned long pa_nRow) {
  char pucBuf[23];
  char pucSign[2];
  UINT16 odoMeter[5] = {0,0,0,0,0};
  int i = 0;


  if (0 > pa_nValue) {
    pa_nValue = pa_nValue * (-1);
    sprintf(pucSign,"-");
  } else {
    sprintf(pucSign," ");
  }
  RIT128x96x4StringDraw(pucSign, 6, pa_nRow, 15);


  for (i=0; i<5;++i) {
      odoMeter[i] = pa_nValue % 1000;
      pa_nValue /= 1000;
  }

  sprintf(pucBuf, "%03d,%03d,%03d,%03d,%03d", odoMeter[4], odoMeter[3],
      odoMeter[2], odoMeter[1], odoMeter[0]);

  RIT128x96x4StringDraw(pucBuf, 12, pa_nRow, 15);

}


//*****************************************************************************
//
// Split double value at decimal-point and provide integer values
// for both values. the value after the decimal point is limited to 3 digits
//
//*****************************************************************************
void
splitDecimal(double pa_fValue, char* pa_pSign, int* pa_pFull, int* pa_pMilli){
  if (0 <= pa_fValue) {
      *pa_pSign = ' ';
      *pa_pFull = floor(pa_fValue);
      *pa_pMilli = floor(fmod(pa_fValue, 1.0)*1000);
  } else {
      *pa_pSign = '-';
      *pa_pFull = floor(-1.0 * pa_fValue);
      *pa_pMilli = floor(fmod(pa_fValue * (-1.0), 1.0)*1000);
  }
}



//*****************************************************************************
//
// Print the given double-value, preceeded by pre-fix and followed by post-fix
// at given position of OLED-display
//
//*****************************************************************************
void
DisplayRealVal(double pa_fValue, const char *pcPrefix, const char *pcPostfix, unsigned long pa_nRow, unsigned long pa_nCol) {
  char pucBuf[23];

  int Val;
  int milliVal;
  char sign;

  splitDecimal(pa_fValue, &sign, &Val, &milliVal);
//TODO: minor bug: values between -1 and 0 have incorrect sign at output (there is no integer value with -0)
  sprintf(pucBuf, "%s%c%2d.%03d%s", pcPrefix, sign, Val, milliVal, pcPostfix);
  RIT128x96x4StringDraw(pucBuf, pa_nCol, pa_nRow, 15);
}

