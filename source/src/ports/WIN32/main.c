/*******************************************************************************
* Copyright (c) 2009, Rockwell Automation, Inc.
* All rights reserved.
*
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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
* @param   thread_arg  dummy argument
* @returns             NO_ERROR at the moment
*
*  The call signature is chosen to be able to pass this function directly as
*  parameter for CreateThread().
*/
static DWORD executeEventLoop(LPVOID thread_arg);


/*****************************************************************************/
/** @brief Flag indicating if the stack should end its execution
*/
volatile int g_end_stack = 0;

/******************************************************************************/
int main(int argc, char *arg[]) {

	if (argc != 2) {
		fprintf(stderr, "Wrong number of command line parameters!\n");
		fprintf(stderr, "Usage: %s [interface index | interface name]\n", arg[0]);
		fprintf(stderr, "\te.g. ./OpENer \"Ethernet 2\"\n");
		exit(EXIT_FAILURE);
	}

	DoublyLinkedListInitialize(&connection_list,
		CipConnectionObjectListArrayAllocator,
		CipConnectionObjectListArrayFree);
	/* Fetch MAC address from the platform. This tests also if the interface
	*  is present. */
	uint8_t iface_mac[6];
	if (kEipStatusError == IfaceGetMacAddress(arg[1], iface_mac)) {
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

	CipEthernetLinkSetMac(iface_mac);

	/* The current host name is used as a default. This value is kept in the
	*  case NvdataLoad() needs to recreate the TCP/IP object's settings from
	*  the defaults on the first start without a valid TCP/IP configuration
	*  file.
	*/
	GetHostName(&g_tcpip.hostname);

	/* The CIP objects are now created and initialized with their default values.
	*  After that any NV data values are loaded to change the attribute contents
	*  to the stored configuration.
	*/
	if (kEipStatusError == NvdataLoad()) {
		OPENER_TRACE_WARN("Loading of some NV data failed. Maybe the first start?\n");
	}

	/* Bring up network interface or start DHCP client ... */
	EipStatus status = BringupNetwork(arg[1],
		g_tcpip.config_control,
		&g_tcpip.interface_configuration,
		&g_tcpip.hostname);
	if (status < 0) {
		OPENER_TRACE_ERR("BringUpNetwork() failed\n");
	}

	/* Register signal handler for SIGINT and SIGTERM that are "supported"
	*  under Windows. */
	g_end_stack = 0;
	signal(SIGINT, LeaveStack);
	signal(SIGTERM, LeaveStack);

	/* Next actions depend on the set network configuration method. */
	CipDword network_config_method = g_tcpip.config_control & kTcpipCfgCtrlMethodMask;
	if (kTcpipCfgCtrlStaticIp == network_config_method) {
		OPENER_TRACE_INFO("Static network configuration done\n");
	}
	if (kTcpipCfgCtrlDhcp == network_config_method) {
		OPENER_TRACE_INFO("DHCP network configuration started\n");
		/* DHCP should already have been started with BringupNetwork(). Wait
		* here for IP present (DHCP done) or abort through g_end_stack. */
		status = IfaceWaitForIp(arg[1], -1, &g_end_stack);
		OPENER_TRACE_INFO("DHCP wait for interface: status %d, g_end_stack=%d\n",
			status, g_end_stack);
		if (kEipStatusOk == status && 0 == g_end_stack) {
			/* Read IP configuration received via DHCP from interface and store in
			*  the TCP/IP object.*/
			status = IfaceGetConfiguration(arg[1], &g_tcpip.interface_configuration);
			if (status < 0) {
				OPENER_TRACE_WARN("Problems getting interface configuration\n");
			}
		}
	}


	/* The network initialization of the EIP stack for the NetworkHandler. */
	if (!g_end_stack && kEipStatusOk == NetworkHandlerInitialize()) {

		(void)executeEventLoop(NULL);

		/* clean up network state */
		NetworkHandlerFinish();
	}

	/* close remaining sessions and connections, clean up used data */
	ShutdownCipStack();

	/* Shut down the network interface now. */
	(void)ShutdownNetwork(arg[1]);

	if (0 != g_end_stack) {
		printf("OpENer aborted by signal %d.\n", g_end_stack);
		return RET_SHOW_SIGNAL + g_end_stack;
	}

	return EXIT_SUCCESS;
}

static void LeaveStack(int signal) {
	if (SIGINT == signal || SIGTERM == signal) {
		g_end_stack = signal;
	}
	OPENER_TRACE_STATE("got signal %d\n", signal);
}

static DWORD executeEventLoop(LPVOID thread_arg) {
	/* The event loop. Put other processing you need done continually in here */
	while (0 == g_end_stack) {
		if (kEipStatusOk != NetworkHandlerProcessOnce()) {
			break;
		}
	}
	return NO_ERROR;
}