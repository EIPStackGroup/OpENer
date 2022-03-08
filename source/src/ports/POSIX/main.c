/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#ifdef OPENER_RT
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <limits.h>
#endif

#include "generic_networkhandler.h"
#include "opener_api.h"
#include "cipethernetlink.h"
#include "ciptcpipinterface.h"
#include "trace.h"
#include "networkconfig.h"
#include "doublylinkedlist.h"
#include "cipconnectionobject.h"
#include "nvdata.h"

#define BringupNetwork(if_name, method, if_cfg, hostname)  (0)
#define ShutdownNetwork(if_name)  (0)

/** If OpENer is aborted by a signal it returns the sum of the signal number
 *  and this define. */
#define RET_SHOW_SIGNAL 200

/******************************************************************************/
/** @brief Signal handler function for ending stack execution
 *
 * @param signal  the signal we received
 */
static void LeaveStack(int signal);

/******************************************************************************/
/** @brief Execute OpENer stack loop function
 *
 * @param   pthread_arg dummy argument
 * @returns             pointer to internal dummy return value
 *
 *  The call signature is chosen to be able to pass this function directly as
 *  parameter for pthread_create().
 */
static void *executeEventLoop(void *pthread_arg);

/******************************************************************************/
/** @brief Fuzz TCP packets handling flow with AFL.
 *
 */
#ifdef FUZZING_AFL
static void fuzzHandlePacketFlow(void);
#endif /* FUZZING_AFL */

/*****************************************************************************/
/** @brief Flag indicating if the stack should end its execution
 */
volatile int g_end_stack = 0;

/******************************************************************************/
int main(int argc,
         char *arg[]) {

  if(argc != 2) {
    fprintf(stderr, "Wrong number of command line parameters!\n");
    fprintf(stderr, "Usage: %s [interface name]\n", arg[0]);
    fprintf(stderr, "\te.g. ./OpENer eth1\n");
    exit(EXIT_FAILURE);
  }

  DoublyLinkedListInitialize(&connection_list,
                             CipConnectionObjectListArrayAllocator,
                             CipConnectionObjectListArrayFree);
  /* Fetch MAC address from the platform. This tests also if the interface
   *  is present. */
  uint8_t iface_mac[6];
  if(kEipStatusError == IfaceGetMacAddress(arg[1], iface_mac) ) {
    printf("Network interface %s not found.\n", arg[1]);
    exit(EXIT_FAILURE);
  }

  /* for a real device the serial number should be unique per device */
  SetDeviceSerialNumber(123456789);

  /* unique_connection_id should be sufficiently random or incremented and stored
   *  in non-volatile memory each time the device boots. This is used as the upper
   *  16 bits of the connection id. Here we use random number approach, first seed
   *  the PRNG to ensure we don't get the same value on every startup.
   */
  srand(time(NULL));
  EipUint16 unique_connection_id = (EipUint16)rand();

  /* Setup the CIP Layer. All objects are initialized with the default
   * values for the attribute contents. */
  EipStatus eip_status = CipStackInit(unique_connection_id);

  CipEthernetLinkSetMac(iface_mac);

  /* The current host name is used as a default. This value is kept in the
   *  case NvdataLoad() needs to recreate the TCP/IP object's settings from
   *  the defaults on the first start without a valid TCP/IP configuration
   *  file.
   */
  GetHostName(&g_tcpip.hostname);

  /* Fuzzing UDP/TCP handle packet flow */
#ifdef FUZZING_AFL
  fuzzHandlePacketFlow();
  return EXIT_SUCCESS;
#endif

  /* The CIP objects are now created and initialized with their default values.
   *  After that any NV data values are loaded to change the attribute contents
   *  to the stored configuration.
   */
  if(kEipStatusError == NvdataLoad() ) {
    OPENER_TRACE_WARN("Loading of some NV data failed. Maybe the first start?\n");
  }

  /* Bring up network interface or start DHCP client ... */
  eip_status = BringupNetwork(arg[1],
                              g_tcpip.config_control,
                              &g_tcpip.interface_configuration,
                              &g_tcpip.hostname);
  if(eip_status < 0) {
    OPENER_TRACE_ERR("BringUpNetwork() failed\n");
  }

  /* register for closing signals so that we can trigger the stack to end */
  g_end_stack = 0;
  signal(SIGHUP, LeaveStack);
  signal(SIGINT, LeaveStack); /* needed to be able to abort with ^C */

  /* Next actions depend on the set network configuration method. */
  CipDword network_config_method = g_tcpip.config_control &
                                   kTcpipCfgCtrlMethodMask;
  if(kTcpipCfgCtrlStaticIp == network_config_method) {
    OPENER_TRACE_INFO("Static network configuration done\n");
  }
  if(kTcpipCfgCtrlDhcp == network_config_method) {
    OPENER_TRACE_INFO("DHCP network configuration started\n");
    /* DHCP should already have been started with BringupNetwork(). Wait
     * here for IP present (DHCP done) or abort through g_end_stack. */
    eip_status = IfaceWaitForIp(arg[1], -1, &g_end_stack);
    OPENER_TRACE_INFO(
      "DHCP wait for interface: eip_status %d, g_end_stack=%d\n",
      eip_status,
      g_end_stack);
    if(kEipStatusOk == eip_status && 0 == g_end_stack) {
      /* Read IP configuration received via DHCP from interface and store in
       *  the TCP/IP object.*/
      eip_status = IfaceGetConfiguration(arg[1],
                                         &g_tcpip.interface_configuration);
      if(eip_status < 0) {
        OPENER_TRACE_WARN("Problems getting interface configuration\n");
      }
    }
  }

  /* The network initialization of the EIP stack for the NetworkHandler. */
  if(!g_end_stack && kEipStatusOk == NetworkHandlerInitialize() ) {
#ifdef OPENER_RT
    int ret;

    /* Memory lock all*/
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
      OPENER_TRACE_ERR("mlockall failed: %m\n");
      exit(-2);
    }

    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    ret = pthread_attr_init(&attr);
    if (ret) {
      OPENER_TRACE_ERR("init pthread attributes failed\n");
      exit(-2);
    }

    /* Set stack size  */
    ret = pthread_attr_setstacksize(&attr,
                                    PTHREAD_STACK_MIN + OPENER_RT_THREAD_SIZE);
    if (ret) {
      OPENER_TRACE_ERR("setstacksize failed\n");
      exit(-2);
    }

    /* Set policy and priority of the thread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (ret) {
      OPENER_TRACE_ERR("setschedpolicy failed\n");
      exit(-2);
    }
    param.sched_priority = 25;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
      OPENER_TRACE_ERR("pthread setschedparam failed\n");
      exit(-2);
    }
    /* scheduling parameters */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
      OPENER_TRACE_ERR("setinheritsched failed\n");
      exit(-2);
    }

    /* Create a thread with the specified attributes */
    ret = pthread_create(&thread, &attr, executeEventLoop, NULL);
    if (ret) {
      OPENER_TRACE_ERR("create pthread failed\n");
      exit(-2);
    }

    /* Join the thread */
    ret = pthread_join(thread, NULL);
    if (ret) {
      OPENER_TRACE_ERR("join pthread failed: %m\n");
    }
    /* Unlock memory */
    munlockall();
#else
    (void) executeEventLoop(NULL);
#endif
    /* clean up network state */
    NetworkHandlerFinish();
  }

  /* close remaining sessions and connections, clean up used data */
  ShutdownCipStack();

  /* Shut down the network interface now. */
  (void) ShutdownNetwork(arg[1]);

  if(0 != g_end_stack) {
    printf("OpENer aborted by signal %d.\n", g_end_stack);
    return RET_SHOW_SIGNAL + g_end_stack;
  }

  return EXIT_SUCCESS;
}

static void LeaveStack(int signal) {
  if(SIGHUP == signal || SIGINT == signal) {
    g_end_stack = signal;
  } OPENER_TRACE_STATE("got signal %d\n", signal);
}

static void *executeEventLoop(void *pthread_arg) {
  static int pthread_dummy_ret;
  (void) pthread_arg;

  /* The event loop. Put other processing you need done continually in here */
  while(!g_end_stack) {
    if(kEipStatusOk != NetworkHandlerProcessCyclic() ) {
      OPENER_TRACE_ERR("Error in NetworkHandler loop! Exiting OpENer!\n");
      break;
    }
  }

  return &pthread_dummy_ret;
}

#ifdef FUZZING_AFL
static void fuzzHandlePacketFlow(void) {
  int socket_fd = 0;   // Fake socket fd
  uint8_t buff[512];   // Input buffer
  struct sockaddr_in from_address = { 0 }; // Fake socket address
  int remaining_bytes = 0; // Fake reamining bytes
  ENIPMessage outgoing_message;

  /* AFL persistent mode */
  while(__AFL_LOOP(100000) ) {
    /* Read input from STDIN and enter the handle receive flow */
    memset(buff, 0, 512);
    ssize_t received_size = read(STDIN_FILENO, buff, 512);
    EipUint8 *receive_buffer = &buff[0];

    InitializeENIPMessage(&outgoing_message);

    // Fuzz UDP
    //EipStatus need_to_send = HandleReceivedExplictUdpData(socket_fd, &from_address, receive_buffer, received_size, &remaining_bytes, true, &outgoing_message);

    // Fuzz TCP
    EipStatus need_to_send = HandleReceivedExplictTcpData(socket_fd,
                                                          receive_buffer,
                                                          received_size,
                                                          &remaining_bytes,
                                                          &from_address,
                                                          &outgoing_message);
  }
}
#endif /* FUZZING_AFL */
