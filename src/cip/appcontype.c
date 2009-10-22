/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 * Contributors:
 *     <date>: <author>, <author email> - changes
 ******************************************************************************/

#include "cipconnectionmanager.h"

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

bool
establishedMasterConnectionExists(S_CIP_ConnectionObject * pa_pstConnData);

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

          if (!establishedMasterConnectionExists(pa_pstConnData))
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

bool
establishedMasterConnectionExists(S_CIP_ConnectionObject * pa_pstConnData)
{
  S_CIP_ConnectionObject *pstRunner = g_pstActiveConnectionList;

  while (NULL != pstRunner)
    {
      if ((enConnTypeIOExclusiveOwner == pstRunner->m_eInstanceType)
          || (enConnTypeIOInputOnly == pstRunner->m_eInstanceType))
        {
          if (pa_pstConnData->ConnectionPath.ConnectionPoint[1]
              == pstRunner->ConnectionPath.ConnectionPoint[1])
            {
              return true;
            }
        }
      pstRunner = pstRunner->m_pstNext;
    }
  return false;
}
