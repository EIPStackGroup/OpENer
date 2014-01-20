/*******************************************************************************
 * Copyright (c) 2011, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "cipclass3connection.h"
#include "cipconnectionmanager.h"
#include <string.h>

S_CIP_ConnectionObject *
getFreeExplicitConnection(void);

/**** Global variables ****/

/*! Array of the available explicit connections */
S_CIP_ConnectionObject g_astExplicitConnections[OPENER_CIP_NUM_EXPLICIT_CONNS];

/**** Implementation ****/
int
establishClass3Connection(struct CIP_ConnectionObject *pa_pstConnObj,
    EIP_UINT16 *pa_pnExtendedError)
{
  int nRetVal = EIP_OK;
  EIP_UINT32 nTmp;

  //TODO add check for transport type trigger
  //if (0x03 == (g_stDummyConnectionObject.TransportTypeClassTrigger & 0x03))

  S_CIP_ConnectionObject *pstExplConn = getFreeExplicitConnection();

  if (NULL == pstExplConn)
    {
      nRetVal = CIP_ERROR_CONNECTION_FAILURE;
      *pa_pnExtendedError = CIP_CON_MGR_ERROR_NO_MORE_CONNECTIONS_AVAILABLE;
    }
  else
    {
      copyConnectionData(pstExplConn, pa_pstConnObj);

      nTmp = pstExplConn->CIPProducedConnectionID;
      generalConnectionConfiguration(pstExplConn);
      pstExplConn->CIPProducedConnectionID = nTmp;
      pstExplConn->m_eInstanceType = enConnTypeExplicit;
      pstExplConn->sockfd[0] = pstExplConn->sockfd[1] = EIP_INVALID_SOCKET;
      /* set the connection call backs */
      pstExplConn->m_pfCloseFunc = removeFromActiveConnections;
      /* explicit connection have to be closed on time out*/
      pstExplConn->m_pfTimeOutFunc = removeFromActiveConnections;

      addNewActiveConnection(pstExplConn);
    }
  return nRetVal;
}

S_CIP_ConnectionObject *
getFreeExplicitConnection(void)
{
  int i;
  for (i = 0; i < OPENER_CIP_NUM_EXPLICIT_CONNS; i++)
    {
      if (g_astExplicitConnections[i].State == CONN_STATE_NONEXISTENT)
        return &(g_astExplicitConnections[i]);
    }
  return NULL;
}

void initializeClass3ConnectionData(){
  memset(g_astExplicitConnections, 0, OPENER_CIP_NUM_EXPLICIT_CONNS * sizeof(S_CIP_ConnectionObject));
}
