/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#include "appcontype.h"
#include "cipconnectionmanager.h"
#include "opener_api.h"

#define CIP_CON_MGR_ERROR_OWNERSHIP_CONFLICT 0x0106
#define CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH 0x0117
#define CIP_CON_MGR_INVALID_OR_INCONSISTENT_CONFIGURATION_APPLICATION_PATH 0x0117
#define CIP_CON_MGR_NON_LISTEN_ONLY_CONNECTION_NOT_OPENED 0x0119
#define CIP_CON_MGR_TARGET_OBJECT_OUT_OF_CONNECTIONS 0x011A

/* external globals neeeded from connectionmanager.c */
extern S_CIP_ConnectionObject *g_pstActiveConnectionList;

typedef struct
{
  unsigned int m_unOutputAssembly; /*< the O-to-T point for the connection */
  unsigned int m_unInputAssembly; /*< the T-to-O point for the connection */
  unsigned int m_unConfigAssembly; /*< the config point for the connection */
  S_CIP_ConnectionObject m_stConnectionData; /*< the connection data, only one connection is allowed per O-to-T point*/
} S_ExclusiveOwnerConnection;

typedef struct
{
  unsigned int m_unOutputAssembly; /*< the O-to-T point for the connection */
  unsigned int m_unInputAssembly; /*< the T-to-O point for the connection */
  unsigned int m_unConfigAssembly; /*< the config point for the connection */
  S_CIP_ConnectionObject
      m_astConnectionData[OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH]; /*< the connection data */
} S_InputOnlyConnection;

typedef struct
{
  unsigned int m_unOutputAssembly; /*< the O-to-T point for the connection */
  unsigned int m_unInputAssembly; /*< the T-to-O point for the connection */
  unsigned int m_unConfigAssembly; /*< the config point for the connection */
  S_CIP_ConnectionObject
      m_astConnectionData[OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH]; /*< the connection data */
} S_ListenOnlyConnection;

S_ExclusiveOwnerConnection
    g_astExlusiveOwnerConnections[OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS];

S_InputOnlyConnection
    g_astInputOnlyConnections[OPENER_CIP_NUM_INPUT_ONLY_CONNS];

S_ListenOnlyConnection
    g_astListenOnlyConnections[OPENER_CIP_NUM_LISTEN_ONLY_CONNS];

S_CIP_ConnectionObject *
getExclusiveOwnerConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError);
S_CIP_ConnectionObject *
getInputOnlyConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError);
S_CIP_ConnectionObject *
getListenOnlyConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError);

void
configureExclusiveOwnerConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly)
{
  if (OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS > pa_unConnNum)
    {
      g_astExlusiveOwnerConnections[pa_unConnNum].m_unOutputAssembly
          = pa_unOutputAssembly;
      g_astExlusiveOwnerConnections[pa_unConnNum].m_unInputAssembly
          = pa_unInputAssembly;
      g_astExlusiveOwnerConnections[pa_unConnNum].m_unConfigAssembly
          = pa_unConfigAssembly;
    }
}

void
configureInputOnlyConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly)
{
  if (OPENER_CIP_NUM_INPUT_ONLY_CONNS > pa_unConnNum)
    {
      g_astInputOnlyConnections[pa_unConnNum].m_unOutputAssembly
          = pa_unOutputAssembly;
      g_astInputOnlyConnections[pa_unConnNum].m_unInputAssembly
          = pa_unInputAssembly;
      g_astInputOnlyConnections[pa_unConnNum].m_unConfigAssembly
          = pa_unConfigAssembly;
    }
}

void
configureListenOnlyConnectionPoint(unsigned int pa_unConnNum,
    unsigned int pa_unOutputAssembly, unsigned int pa_unInputAssembly,
    unsigned int pa_unConfigAssembly)
{
  if (OPENER_CIP_NUM_LISTEN_ONLY_CONNS > pa_unConnNum)
    {
      g_astListenOnlyConnections[pa_unConnNum].m_unOutputAssembly
          = pa_unOutputAssembly;
      g_astListenOnlyConnections[pa_unConnNum].m_unInputAssembly
          = pa_unInputAssembly;
      g_astListenOnlyConnections[pa_unConnNum].m_unConfigAssembly
          = pa_unConfigAssembly;
    }
}

S_CIP_ConnectionObject *
getIOConnectionForConnectionData(S_CIP_ConnectionObject *pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError)
{
  S_CIP_ConnectionObject *pstRetVal = NULL;
  *pa_pnExtendedError = 0;

  pstRetVal = getExclusiveOwnerConnection(pa_pstConnData, pa_pnExtendedError);
  if (NULL == pstRetVal)
    {
      if (0 == *pa_pnExtendedError)
        {
          /* we found no connection and don't have an error so try input only next */
          pstRetVal
              = getInputOnlyConnection(pa_pstConnData, pa_pnExtendedError);
          if (NULL == pstRetVal)
            {
              if (0 == *pa_pnExtendedError)
                {
                  /* we found no connection and don't have an error so try listen only next */
                  pstRetVal = getListenOnlyConnection(pa_pstConnData,
                      pa_pnExtendedError);
                  if ((NULL == pstRetVal) && (0 == *pa_pnExtendedError))
                    {
                      /* no application connection type was found that suits the given data */
                      /* TODO check error code VS */
                      *pa_pnExtendedError
                          = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
                    }
                }
            }
          else
            {
              pa_pstConnData->m_eInstanceType = enConnTypeIOInputOnly;
            }
        }
    }
  else
    {
      pa_pstConnData->m_eInstanceType = enConnTypeIOExclusiveOwner;
    }

  if (NULL != pstRetVal)
    {
      copyConnectionData(pstRetVal, pa_pstConnData);
    }

  return pstRetVal;
}

S_CIP_ConnectionObject *
getExclusiveOwnerConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError)
{
  S_CIP_ConnectionObject *pstRetVal = NULL;
  int i;

  for (i = 0; i < OPENER_CIP_NUM_EXLUSIVE_OWNER_CONNS; i++)
    {
      if (g_astExlusiveOwnerConnections[i].m_unOutputAssembly
          == pa_pstConnData->ConnectionPath.ConnectionPoint[0])
        { /* we have the same output assembly */
          if (g_astExlusiveOwnerConnections[i].m_unInputAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[1])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }
          if (g_astExlusiveOwnerConnections[i].m_unConfigAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[2])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }
          if (g_astExlusiveOwnerConnections[i].m_stConnectionData.State
              != CONN_STATE_NONEXISTENT)
            {
              *pa_pnExtendedError = CIP_CON_MGR_ERROR_OWNERSHIP_CONFLICT;
              break;
            }
          pstRetVal = &(g_astExlusiveOwnerConnections[i].m_stConnectionData);
          break;
        }
    }
  return pstRetVal;
}

S_CIP_ConnectionObject *
getInputOnlyConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError)
{
  S_CIP_ConnectionObject *pstRetVal = NULL;
  int i, j;

  for (i = 0; i < OPENER_CIP_NUM_INPUT_ONLY_CONNS; i++)
    {
      if (g_astInputOnlyConnections[i].m_unOutputAssembly
          == pa_pstConnData->ConnectionPath.ConnectionPoint[0])
        { /* we have the same output assembly */
          if (g_astInputOnlyConnections[i].m_unInputAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[1])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }
          if (g_astInputOnlyConnections[i].m_unConfigAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[2])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }

          for (j = 0; j < OPENER_CIP_NUM_INPUT_ONLY_CONNS_PER_CON_PATH; j++)
            {
              if (CONN_STATE_NONEXISTENT
                  == g_astInputOnlyConnections[i].m_astConnectionData[j].State)
                {
                  return &(g_astInputOnlyConnections[i].m_astConnectionData[j]);
                }
            }
          *pa_pnExtendedError = CIP_CON_MGR_TARGET_OBJECT_OUT_OF_CONNECTIONS;
          break;
        }
    }
  return pstRetVal;
}

S_CIP_ConnectionObject *
getListenOnlyConnection(S_CIP_ConnectionObject * pa_pstConnData,
    EIP_UINT16 *pa_pnExtendedError)
{
  S_CIP_ConnectionObject *pstRetVal = NULL;
  int i, j;

  if (CIP_MULTICAST_CONNECTION
      != (pa_pstConnData->T_to_O_NetworkConnectionParameter
          & CIP_MULTICAST_CONNECTION))
    {
      /* a listen only connection has to be a multicast connection. */
      *pa_pnExtendedError = CIP_CON_MGR_NON_LISTEN_ONLY_CONNECTION_NOT_OPENED; /* maybe not the best error message however there is no suitable definition in the cip spec */
      return NULL;
    }

  for (i = 0; i < OPENER_CIP_NUM_LISTEN_ONLY_CONNS; i++)
    {
      if (g_astListenOnlyConnections[i].m_unOutputAssembly
          == pa_pstConnData->ConnectionPath.ConnectionPoint[0])
        { /* we have the same output assembly */
          if (g_astListenOnlyConnections[i].m_unInputAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[1])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }
          if (g_astListenOnlyConnections[i].m_unConfigAssembly
              != pa_pstConnData->ConnectionPath.ConnectionPoint[2])
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_INVALID_PRODUCED_OR_CONSUMED_APPLICATION_PATH;
              break;
            }

          if (NULL != getExistingProdMulticastConnection(
              pa_pstConnData->ConnectionPath.ConnectionPoint[1]))
            {
              *pa_pnExtendedError
                  = CIP_CON_MGR_NON_LISTEN_ONLY_CONNECTION_NOT_OPENED;
              break;
            }

          for (j = 0; j < OPENER_CIP_NUM_LISTEN_ONLY_CONNS_PER_CON_PATH; j++)
            {
              if (CONN_STATE_NONEXISTENT
                  == g_astListenOnlyConnections[i].m_astConnectionData[j].State)
                {
                  return &(g_astListenOnlyConnections[i].m_astConnectionData[j]);
                }
            }
          *pa_pnExtendedError = CIP_CON_MGR_TARGET_OBJECT_OUT_OF_CONNECTIONS;
          break;
        }
    }
  return pstRetVal;
}

S_CIP_ConnectionObject *
getExistingProdMulticastConnection(EIP_UINT32 pa_unInputPoint)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if ((enConnTypeIOExclusiveOwner == pstRunner->m_eInstanceType)
          || (enConnTypeIOInputOnly == pstRunner->m_eInstanceType))
        {
          if ((pa_unInputPoint == pstRunner->ConnectionPath.ConnectionPoint[1])
              && (CIP_MULTICAST_CONNECTION
                  == (pstRunner->T_to_O_NetworkConnectionParameter
                      & CIP_MULTICAST_CONNECTION)) && (EIP_INVALID_SOCKET
              != pstRunner->sockfd[PRODUCING]))
            {
              /* we have a connection that produces the same input assembly,
               * is a multicast producer and manages the connection.
               */
              break;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return pstRunner;
}

S_CIP_ConnectionObject *
getNextNonCtrlMasterCon(EIP_UINT32 pa_unInputPoint)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if ((enConnTypeIOExclusiveOwner == pstRunner->m_eInstanceType)
          || (enConnTypeIOInputOnly == pstRunner->m_eInstanceType))
        {
          if ((pa_unInputPoint == pstRunner->ConnectionPath.ConnectionPoint[1])
              && (CIP_MULTICAST_CONNECTION
                  == (pstRunner->T_to_O_NetworkConnectionParameter
                      & CIP_MULTICAST_CONNECTION)) && (EIP_INVALID_SOCKET
              == pstRunner->sockfd[PRODUCING]))
            {
              /* we have a connection that produces the same input assembly,
               * is a multicast producer and does not manages the connection.
               */
              break;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return pstRunner;
}

void
closeAllConnsForInputWithSameType(EIP_UINT32 pa_unInputPoint,
    EConnType pa_eInstanceType)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;
  S_CIP_ConnectionObject *pstToDelete;

  while (NULL != pstRunner)
    {
      if ((pa_eInstanceType == pstRunner->m_eInstanceType) && (pa_unInputPoint
          == pstRunner->ConnectionPath.ConnectionPoint[1]))
        {
          pstToDelete = pstRunner;
          pstRunner = pstRunner->m_pstNext;
          IApp_IOConnectionEvent(
              pstToDelete->ConnectionPath.ConnectionPoint[0],
              pstToDelete->ConnectionPath.ConnectionPoint[1], enClosed);

          closeConnection(pstToDelete); /* will remove the connection from the active connection list */
        }
      else
        {
          pstRunner = pstRunner->m_pstNext;
        }
    }
}

void closeAllConnections(void){
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;
  while (NULL != pstRunner){
    closeConnection(pstRunner);
    /* Close connection will remove the connection from the list therefore we
     * need to get again the start until there is no connection left
     */
    pstRunner = g_pstActiveConnectionList;
  }

}

bool
connectionWithSameConfigPointExists(EIP_UINT32 pa_unConfigPoint)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if (pa_unConfigPoint == pstRunner->ConnectionPath.ConnectionPoint[2])
        {
          break;
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return (NULL != pstRunner);
}
