/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/capability.h>

#ifdef OPENER_RT
#include <pthread.h>
#include <sys/mman.h>
#include <sched.h>
#include <limits.h>
#endif

#include "generic_networkhandler.h"
#include "opener_api.h"
#include "cipcommon.h"
#include "ciptcpipinterface.h"
#include "trace.h"
#include "networkconfig.h"
#include "doublylinkedlist.h"
#include "cipconnectionobject.h"
#include "nvdata.h"

#define BringupNetwork(if_name, method, if_cfg, hostname)  (0)
#define ShutdownNetwork(if_name)  (0)

/******************************************************************************/
/** @brief Signal handler function for ending stack execution
 *
 * @param signal the signal we received
 */
void LeaveStack(int signal);

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

/*****************************************************************************/
/** @brief Flag indicating if the stack should end its execution
 */
volatile int g_end_stack = 0;

/******************************************************************************/
int main(int argc,
         char *arg[]) {
  int ret;

  cap_t capabilities;
  cap_value_t capabilies_list[1];

  capabilities = cap_get_proc();
  if (NULL == capabilities) {
    printf("Could not get capabilities\n");
    exit(EXIT_FAILURE);
  }

  capabilies_list[0] = CAP_NET_RAW;
  if (-1
      == cap_set_flag(capabilities, CAP_EFFECTIVE, 1, capabilies_list,
                      CAP_SET) ) {
    cap_free(capabilities);
    printf("Could not set CAP_NET_RAW capability\n");
    exit(EXIT_FAILURE);
  }

  if (-1 == cap_set_proc(capabilities) ) {
    cap_free(capabilities);
    printf("Could not push CAP_NET_RAW capability to process\n");
    exit(EXIT_FAILURE);
  }

  if (-1 == cap_free(capabilities) ) {
    printf("Could not free capabilites value\n");
    exit(EXIT_FAILURE);
  }

  if (argc != 2) {
    printf("Wrong number of command line parameters!\n");
    printf("The correct command line parameters are:\n");
    printf("./OpENer interfacename\n");
    printf("    e.g. ./OpENer eth1\n");
    exit(EXIT_FAILURE);
  }

  DoublyLinkedListInitialize(&connection_list,
                             CipConnectionObjectListArrayAllocator,
                             CipConnectionObjectListArrayFree);
  /* Fetch MAC address from the platform. This tests also if the interface
   *  is present. */
  if (kEipStatusError == ConfigureMacAddressByInterface(arg[1]) ) {
    printf("Network interface %s not found.\n", arg[1]);
    exit(EXIT_FAILURE);
  }

  /* for a real device the serial number should be unique per device */
  SetDeviceSerialNumber(123456789);

  /* unique_connection_id should be sufficiently random or incremented and stored
   *  in non-volatile memory each time the device boots.
   */
  EipUint16 unique_connection_id = rand();

  /* Setup the CIP Layer. All objects are initialized with the default
   * values for the attribute contents. */
  CipStackInit(unique_connection_id);

  /* The current host name is used as a default. This value is kept in the
   *  case NvdataLoad() needs to recreate the TCP/IP object's settings from
   *  the defaults on the first start without a valid TCP/IP configuration
   *  file.
   */
  ConfigureHostName();

  /* Now any NV data values are loaded to change the attribute contents to
   *  the stored configuration.
   */
  if (kEipStatusError == NvdataLoad()) {
    OPENER_TRACE_WARN("Loading of some NV data failed. Maybe the first start?\n");
  }

  /* Bring up network interface or start DHCP client ... */
  ret = BringupNetwork(arg[1],
                       g_tcpip.config_control,
                       &g_tcpip.interface_configuration,
                       &g_tcpip.hostname);
  if (ret < 0) {
    OPENER_TRACE_ERR("BringUpNetwork() failed\n");
  }

  /* register for closing signals so that we can trigger the stack to end */
  g_end_stack = 0;
  signal(SIGHUP, LeaveStack);
  signal(SIGINT, LeaveStack); /* needed to be able to abort with ^C */

  /* Next actions depend on the set network configuration method. */
  CipDword network_config_method = g_tcpip.config_control & kTcpipCfgCtrlMethodMask;
  if (kTcpipCfgCtrlStaticIp == network_config_method) {
    OPENER_TRACE_INFO("Static network configuration done\n");
  }
  if (kTcpipCfgCtrlDhcp == network_config_method) {
    OPENER_TRACE_INFO("DHCP network configuration started\n");
    /* Wait for DHCP done here ...*/
    /* TODO: implement this function to wait for IP present
        or abort through g_end_stack.
        rc = WaitInterfaceIsUp(arg[1], g_end_stack);
      */
    OPENER_TRACE_INFO("DHCP wait for interface: status %d\n", ret);

    /* Read IP configuration received via DHCP from interface and store in
     *  the TCP/IP object.*/
    ret = ConfigureNetworkInterface(arg[1]);
    if (ret < 0) {
      OPENER_TRACE_WARN("Problems getting interface configuration\n");
    }
    ConfigureDomainName();
  }


  /* The network initialization of the EIP stack for the NetworkHandler. */
  if (!g_end_stack && kEipStatusOk == NetworkHandlerInitialize()) {
#ifdef OPENER_RT
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
    (void)executeEventLoop(NULL);
#endif
    /* clean up network state */
    NetworkHandlerFinish();
  }
  /* close remaining sessions and connections, clean up used data */
  ShutdownCipStack();

  /* Shut down the network interface now. */
  (void) ShutdownNetwork(arg[1]);

  return EXIT_SUCCESS;
}

void LeaveStack(int signal) {
  (void) signal;       /* kill unused parameter warning */
  OPENER_TRACE_STATE("got signal %d\n",signal);
  g_end_stack = 1;
}

void *executeEventLoop(void *pthread_arg) {
  static int pthread_dummy_ret;
  (void) pthread_arg;

  /* The event loop. Put other processing you need done continually in here */
  while (!g_end_stack) {
    if (kEipStatusOk != NetworkHandlerProcessOnce() ) {
      OPENER_TRACE_ERR("Error in NetworkHandler loop! Exiting OpENer!\n");
      break;
    }
  }

  return &pthread_dummy_ret;
}
