/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "networkhandler.h"
#include "cipcommon.h"
#include "cipconnectionmanager.h"

#include "encap.h"
#include <trace.h>

#define EIP_VERBOSE	2
#define EIP_VVERBOSE	3
#define EIP_TERSE	1

#define MAX_RECEIVE_SIZE 512
#define MAX_SEND_SIZE 512

static EIP_UINT8 rxbuf[MAX_RECEIVE_SIZE]; // this appears to be the EIP command buffer
EIP_UINT8 eip_reply_buf[MAX_SEND_SIZE]; // this appears to be the EthernetIP reply buffer

extern void
dump(unsigned char *p, int size);

static struct tcp_pcb *TCPlistener;
static struct udp_pcb *UDPlistener;

#define MAX_NO_OF_TCP_SOCKETS 10

struct sockaddr_in my_addr;

struct tcp_pcb *g_pstCurrentTCP_PCB = NULL;

// UDP UNSOLICITED DATA RECEIVE CALLBACK
// The callback function is responsible for deallocating the pbuf.

void
udp_unsolicited_receive_callback(void * arg, // arg specified when the callback was registered
    struct udp_pcb * pcb, // pcb handling the receive
    struct pbuf * p, // the packet
    struct ip_addr * addr, // source IP address
    u16_t port) // source UDP port number
{
  EIP_UINT8 *rxp; // pointer into the receive buf
  int rxlen; // size of the received message
  int bytesleft; // bytes left (allows for multiple messages per packet)
  int replylen;
  err_t status;
  struct pbuf * r; // reply buffer
  struct sockaddr_in stFrom;

  assert(p->tot_len <= sizeof(rxbuf)); // TODO this needs to be hardened
  pbuf_copy_partial(p, rxbuf, sizeof(rxbuf), 0); // copy the packet into a contiguous receive buffer
  rxlen = p->tot_len; // size of the received message
  pbuf_free(p); // we no longer need the packet buffer

  if (EIP_DEBUG > EIP_VVERBOSE)
    {
      OPENER_TRACE_INFO("Data received on UDP:\n");
      dump(rxbuf, rxlen);
    }

  rxp = &rxbuf[0]; // point to the start of the message
  do
    {
      stFrom.sin_family = AF_INET;
      stFrom.sin_port = port;
      stFrom.sin_addr.s_addr = addr->addr;
      memset(&stFrom.sin_zero, 0, sizeof(stFrom.sin_zero));

      replylen = handleReceivedExplictUDPData((unsigned) pcb, // bogus a socket fd (use the pcb address cast to an int)
          &stFrom, rxp, rxlen, &bytesleft);

      rxp += rxlen - bytesleft; // bump the buffer pointer by the amount of data that was eaten
      rxlen = bytesleft; // dec the data size by the same amount

      if (replylen > 0)
        {
          if (EIP_DEBUG >= EIP_VVERBOSE)
            {
              OPENER_TRACE_INFO("reply sent:\n");
              dump(eip_reply_buf, replylen);
            }

          r = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
          assert(r != 0); // TODO harden
          //r->payload = &eip_reply_buf[0];
          r->payload = &rxbuf[0];
          r->len = r->tot_len = replylen;
          status = udp_sendto(pcb, r, (struct ip_addr *) &addr->addr, port);
          assert(status == 0); // TODO check for non-fatal status response?
          pbuf_free(r); // reference counting makes sure the header does not get freed prematurely, but what about the replybuf?
        }
    }
  while (rxlen > 0);
}

// UDP CONNECTED DATA CALLBACK

void
udp_registered_receive_callback(void * arg, // arg specified when the callback was registered
    struct udp_pcb * pcb, // pcb handling the receive
    struct pbuf * pbuf, // the packet
    struct ip_addr * addr, // source IP address
    u16_t port) // source UDP port number
{
  EIP_UINT8 *rxp; // pointer into the receive buf
  int rxlen; // size of the received message
  struct sockaddr_in stFrom;

  //assert(pbuf->len == pbuf->tot_len);
  assert(pbuf->tot_len <= MAX_RECEIVE_SIZE); // TODO this needs to be hardened
  rxlen = pbuf_copy_partial(pbuf, rxbuf, MAX_RECEIVE_SIZE, 0); // copy the packet into a contiguous receive buffer
  //assert(rxlen == pbuf->tot_len);
  rxp = &rxbuf[0]; // point to the start of the message
  pbuf_free(pbuf); // we no longer need the packet buffer

  if (rxlen == 0)
    {
      OPENER_TRACE_ERR("connection closed by client\n");
      udp_disconnect(pcb); /* close socket */
      udp_remove(pcb);
      return;
    }
  if (rxlen <= 0)
    {
      OPENER_TRACE_ERR("networkhandler: error on recv");
      udp_disconnect(pcb); /* close socket */
      udp_remove(pcb);
      return;
    }

  stFrom.sin_family = AF_INET;
  stFrom.sin_port = port;
  stFrom.sin_addr.s_addr = addr->addr;
  memset(&stFrom.sin_zero, 0, sizeof(stFrom.sin_zero));

  handleReceivedConnectedData(rxbuf, rxlen, &stFrom);
}

// TCP DATA SENT CALLBACK

err_t
tcp_sent_callback(void * arg, struct tcp_pcb * tpcb, u16_t len)
{
  // TODO unlock the reply buf?

  return ERR_OK;
}



// TCP DATA RECEIVE CALLBACK

err_t
tcp_receive_callback(void * arg, // arg specified earlier
    struct tcp_pcb *pcb, // pcb that is delivering the data
    struct pbuf *pbuf, // the packet
    err_t err) // TCP uses this to tell us what's happening to the connection perhaps?
{
  EIP_UINT8 *rxp;
  int rxlen;
  int bytesread;
  int bytesleft;
  int replylen;
  int txspace;
  int rxoff = 0;
  err_t status;

  if (err != ERR_OK)
    return err; // don't try to receive if error

  if (pbuf == 0) // check if connection is closing
    {
      tcp_close(pcb); // close out end
      return ERR_OK;
    }

  while (rxoff < pbuf->tot_len)
    {
      rxlen = pbuf_copy_partial(pbuf, rxbuf, 4, rxoff); // copy the first four words into the contiguous receive buffer
      assert(rxlen == 4); //need at least four bytes of the header at this point
      rxp = &rxbuf[2]; // at this place EIP stores the data length
      rxlen = ltohs(&rxp) + ENCAPSULATION_HEADER_LENGTH - 4; // -4 is for the 4 bytes we have already read
      // (NOTE this advances the buffer pointer)

      if (rxlen + 4 > MAX_RECEIVE_SIZE)
        { //TODO can this be handled in a better way?
          OPENER_TRACE_ERR("too large packet received will be ignored\n"); // this may corrupt the connection ???
          return EIP_ERROR;
        }

      bytesread = pbuf_copy_partial(pbuf, &rxbuf[4], rxlen, rxoff + 4); // copy the rest of the message into the contiguous receive buffer
      assert(bytesread == rxlen);
      rxlen += 4;
      rxoff += rxlen;
      tcp_recved(pcb, rxlen); // tell TCP we have received the data

      if (EIP_DEBUG >= EIP_VVERBOSE)
        {
          OPENER_TRACE_INFO("Data received on tcp:\n");
          dump(rxbuf, rxlen);
        }

      g_pstCurrentTCP_PCB = pcb;
      replylen = handleReceivedExplictTCPData((unsigned) pcb, // bogus a socket fd (use the pcb address cast to an int) -- I do not think this is used anywhere
          rxbuf, rxlen, &bytesleft);
      g_pstCurrentTCP_PCB = NULL;
      assert(bytesleft == 0);

      if (replylen > 0)
        {
          if (EIP_DEBUG >= EIP_VVERBOSE)
            {
              OPENER_TRACE_INFO("reply sent:\n");
              dump(eip_reply_buf, replylen);
            }

          txspace = tcp_sndbuf(pcb); // see how much data can be sent
          assert(txspace >= replylen); // TODO harden this

          status = tcp_write(pcb, rxbuf, replylen, TCP_WRITE_FLAG_COPY); // TODO need to examine serial reuse of the reply buf
          if (status != ERR_OK)
            {
              OPENER_TRACE_ERR("TCP response was not sent OK: %d\n", status);
            }
          tcp_output(pcb); // push the data out
        }
    }

  pbuf_free(pbuf); // we no longer need the packet buffer

  return ERR_OK;
}

// TCP CONNECTION ACCEPT CALLBACK

err_t
tcp_connection_accept_callback(void *arg, // arg registered for this listener (ignored in this case)
    struct tcp_pcb *newpcb, // pcb for new tcp connection
    err_t err)
{
  OPENER_TRACE_INFO("networkhandler: new TCP connection\n");

  tcp_accepted(TCPlistener); // tell the listener that the call was accepted

  tcp_recv(newpcb, tcp_receive_callback); // set the callback for received data on the new connection
  tcp_sent(newpcb, tcp_sent_callback); // set the callback for transmitted data on the new connection

  return ERR_OK;
}

/* INT8 Start_NetworkHandler()
 * 	start a TCP listening socket, accept connections, receive data in select loop, call manageConnections periodicaly.
 * 	return status
 * 			-1 .. error
 */

EIP_STATUS
Start_NetworkHandler()
{
  err_t status;

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(OPENER_ETHERNET_PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&my_addr.sin_zero, 0, sizeof(my_addr.sin_zero));

  /* create a new TCP listener socket */
  TCPlistener = tcp_new();
  assert(TCPlistener != 0);
  status = tcp_bind(TCPlistener, INADDR_ANY, OPENER_ETHERNET_PORT);
  assert(status == 0);
  TCPlistener = tcp_listen(TCPlistener);
  assert(TCPlistener != 0);
  tcp_accept(TCPlistener, tcp_connection_accept_callback);

  /* create a new UDP listener socket */
  UDPlistener = udp_new();
  assert(UDPlistener != 0);
  udp_bind(UDPlistener, INADDR_ANY, OPENER_ETHERNET_PORT);
  udp_recv(UDPlistener, udp_unsolicited_receive_callback, 0); // set the callback

  return EIP_OK;
}

static elapsedtime = 0;

// this gets called every 10 ms from the lwiptick handler, which is called from an Ethernet controller interrupt (SYSTICKHZ = 100)
void
CIPtick(int delta) // time since last tick in ms, probably 10
{
  /* call manage_connections() in connection manager every TIMERTICK ms */
  elapsedtime += delta;
  if (elapsedtime >= TIMERTICK)
    {
      manageConnections();
      elapsedtime = 0;
    }
}

/* INT8 registerCallbackFunc(int sockfd, struct sockaddr_in pa_addr, S_CIP_Class *p_stObject, INT8 (*pt2func)(S_CIP_Class *p_stObject, INT8 *data, UINT16 datalength))
 *  register a callbackfuntion with the corresponding CIP object and socket handle.
 * 	sockfd		sockethandle
 * 	pa_addr		remote address for sending UDP packets
 * 	p_stObject	pointer to CIP object which is to be registered
 * 	(*pt2func)()	pointer to callbackfunktion which will be called if data arrive on sockfd
 *
 *	return status	0 .. success
 * 			-1 .. error
 */

EIP_STATUS
registerCallbackFunc(int sockfd, struct sockaddr_in *pa_addr,
    S_CIP_Instance *pa_pstInstance, TCIPServiceFunc pa_ptfuncReceiveData)
{
  return EIP_OK;
}

EIP_STATUS
unregisterCallbackFunc(S_CIP_Instance * pa_pstInstance)
{
  return EIP_OK;
}

/* INT8 sendUDPData(struct sockaddr_in pa_addr, int sockfd, INT8 *pa_data, UINT16 pa_datalength)
 * send udp datagram to pa_addr on socket sockfd.
 * 	 pa_addr	remote address
 * 	 sockfd		sockethandle
 * 	 pa_data	pointer to data which will be sent
 * 	 pa_datalength	length of data ind pa_data
 *
 * return status	0 .. success
 * 			-1 .. error
 */

EIP_STATUS
IApp_SendUDPData(struct sockaddr_in *pa_addr, int sockfd, EIP_UINT8 *pa_data,
    EIP_UINT16 pa_datalength)
{
  struct pbuf *r;
  err_t status;
  struct udp_pcb *pcb;

  pcb = (struct udp_pcb *) sockfd;
  r = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
  assert(r != 0); // TODO harden
  r->payload = pa_data;
  r->len = r->tot_len = pa_datalength;
  status = udp_sendto(pcb, r, (struct ip_addr *) &pa_addr->sin_addr,
      htons(pa_addr->sin_port));
  assert(status == 0); // TODO check for non-fatal status response?
  pbuf_free(r); // reference counting make sure the header does not get freed prematurely, but what about the replybuf?

  return EIP_OK;
}

// create a new UDP socket for the connection manager
// returns the fd if successful, else -1

int
IApp_CreateUDPSocket(int pa_nDirection, // direction: CONSUMING or PRODUCING
    struct sockaddr_in *pa_pstAddr) // bind address, used for producing only
{
  struct udp_pcb *pcb;

  /* create a new UDP socket */
  pcb = udp_new();
  if (pcb == 0)
    {
      OPENER_TRACE_ERR("networkhandler: cannot create UDP socket\n");
      return -1;
    } OPENER_TRACE_INFO("networkhandler: created UDP socket %x\n", pcb);

  /* check if it is sending or receiving */
  if (pa_nDirection == CONSUMING)
    { /* bind is only for consuming necessary */
      struct ip_addr addr;

      addr.addr = htonl(pa_pstAddr->sin_addr.s_addr);
      if (udp_bind(pcb, &addr, htons(pa_pstAddr->sin_port)) != ERR_OK)
        {
          OPENER_TRACE_INFO("networkhandler: error on bind udp\n");
          return -1;
        }

      OPENER_TRACE_INFO("networkhandler: bind UDP socket %08x port %d\n", pcb,
          pa_pstAddr->sin_port);
      udp_recv(pcb, udp_registered_receive_callback, 0); // set the callback
    }


  if ((pa_nDirection == CONSUMING) || (0 == pa_pstAddr->sin_addr.s_addr))
    {
      /* store the originators address */
      pa_pstAddr->sin_addr.s_addr =  g_pstCurrentTCP_PCB->remote_ip.addr;
    }

  return (int) pcb;
}

void IApp_CloseSocket_udp(int fd)
{
  struct udp_pcb *pcb;

  OPENER_TRACE_INFO("networkhandler: shutdown UDP socket %x\n", pcb);
  pcb = (struct udp_pcb *) fd;
  udp_remove(pcb);
}

void IApp_CloseSocket_tcp(int fd)
{
  struct tcp_pcb *pcb;

  OPENER_TRACE_INFO("networkhandler: shutdown TCP socket %x\n", pcb);
  pcb = (struct tcp_pcb *) fd;
  tcp_close(pcb);
}

