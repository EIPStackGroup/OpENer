/*******************************************************************************
 * Copyright (c) 2023, Peter Christen
 * All rights reserved.
 *
 ******************************************************************************/
#include "generic_networkhandler.h"
#include "opener_api.h"
#include "cipethernetlink.h"
#include "ciptcpipinterface.h"
#include "trace.h"
#include "networkconfig.h"
#include "doublylinkedlist.h"
#include "cipconnectionobject.h"

#define OPENER_THREAD_PRIO			osPriorityAboveNormal
#define OPENER_STACK_SIZE			  2000

static void opener_thread(void const *argument);
osThreadId opener_ThreadId;
volatile int g_end_stack = 0;

/**
 * @brief   Initializes the OpENer Ethernet/IP stack
 *          The network interface has to be configured and the link established
 * @param   netif      address specifying the network interface
 * @retval  None
 */
void opener_init(struct netif *netif) {

  EipStatus eip_status = 0;

  if (IfaceLinkIsUp(netif)) {
    DoublyLinkedListInitialize(&connection_list,
                               CipConnectionObjectListArrayAllocator,
                               CipConnectionObjectListArrayFree);

    /* Fetch MAC address from the platform */
    uint8_t iface_mac[6];
    IfaceGetMacAddress(netif, iface_mac);

    /* for a real device the serial number should be unique per device */
    SetDeviceSerialNumber(123456789);

    /* unique_connection_id should be sufficiently random or incremented and stored
     *  in non-volatile memory each time the device boots.
     */
    EipUint16 unique_connection_id = rand();

    /* Setup the CIP Layer. All objects are initialized with the default
     * values for the attribute contents. */
    EipStatus eip_status = CipStackInit(unique_connection_id);

    CipEthernetLinkSetMac(iface_mac);

    /* The current host name is used as a default. */
    GetHostName(netif, &g_tcpip.hostname);

    /* register for closing signals so that we can trigger the stack to end */
    g_end_stack = 0;


    eip_status = IfaceGetConfiguration(netif, &g_tcpip.interface_configuration);
    if (eip_status < 0) {
      OPENER_TRACE_WARN("Problems getting interface configuration\n");
    }

    eip_status = NetworkHandlerInitialize();
  }
  else {
    OPENER_TRACE_WARN("Network link is down, OpENer not started\n");
    g_end_stack = 1;  // end in case of network link is down
  }
  if ((g_end_stack == 0) && (eip_status == kEipStatusOk)) {
    osThreadDef(OpENer, opener_thread, OPENER_THREAD_PRIO, 0,
                OPENER_STACK_SIZE);
    osThreadCreate(osThread(OpENer), netif);
    OPENER_TRACE_INFO("OpENer: opener_thread started, free heap size: %d\n",
           xPortGetFreeHeapSize());
  } else {
    OPENER_TRACE_ERR("NetworkHandlerInitialize error %d\n", eip_status);
  }
}

static void opener_thread(void const *argument) {
  struct netif *netif = (struct netif*) argument;
  /* The event loop. Put other processing you need done continually in here */
  while (!g_end_stack) {
    if (kEipStatusOk != NetworkHandlerProcessCyclic()) {
      OPENER_TRACE_ERR("Error in NetworkHandler loop! Exiting OpENer!\n");
      g_end_stack = 1;	// end loop in case of error
    }
    if (!IfaceLinkIsUp(netif)) {
      OPENER_TRACE_INFO("Network link is down, exiting OpENer\n");
      g_end_stack = 1;	// end loop in case of network link is down
    }
  }		// loop ended
  /* clean up network state */
  NetworkHandlerFinish();
  /* close remaining sessions and connections, clean up used data */
  ShutdownCipStack();
  /* Delete the OpENer Thread */
  osThreadTerminate(NULL);
}
