/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#include "opener_api.h"
#include "cipcommon.h"
#include "cipmessagerouter.h"
#include "endianconv.h"
#include "ciperror.h"
#include "trace.h"

S_CIP_MR_Request gMRRequest;
S_CIP_MR_Response gMRResponse;

/*! A class registry list node
 * a linked list of this  object is the registry of classes known to the message router
 * for small devices with very limited memory it could make sense to change this list into an
 * array with a given max size for removing the need for having to dynamically allocate 
 * memory. The size of the array could be a parameter in the platform config file.
 */
typedef struct CIP_MR_Object
{
  struct CIP_MR_Object *next; /*< link */
  S_CIP_Class *pt2Class; /*< object */
} S_CIP_MR_Object;

/* pointer to first registered object in MessageRouter*/
S_CIP_MR_Object *g_pt2firstObject = 0;

/*! Register an Class to the message router
 *  @param pa_pt2Class     pointer to a class object to be registered.
 *  @return status      0 .. success
 *                     -1 .. error no memory available to register more objects
 */
EIP_STATUS
registerClass(S_CIP_Class * pa_pt2Class);

/*!  Create MRRequest structure out of the received data.
 * 
 * Parses the UCMM header consisting of: service, IOI size, IOI, data into a request structure
 * @param pa_pnData    pointer to the message data received
 * @param pa_nLength   number of bytes in the message
 * @param pa_pstMRReqdata   pointer to structure of MRRequest data item.
 * @return status  0 .. success
 *                 -1 .. error
 */EIP_BYTE
createMRRequeststructure(EIP_UINT8 * pa_pnData, EIP_INT16 pa_nLength,
    S_CIP_MR_Request * pa_pstMRReqdata);

EIP_STATUS
CIP_MessageRouter_Init()
{
  S_CIP_Class *pstMessageRouter;

  pstMessageRouter = createCIPClass(CIP_MESSAGE_ROUTER_CLASS_CODE, /* class ID*/
  0, /* # of class attributes */
  0xffffffff, /* class getAttributeAll mask*/
  0, /* # of class services*/
  0, /* # of instance attributes*/
  0xffffffff, /* instance getAttributeAll mask*/
  0, /* # of instance services*/
  1, /* # of instances*/
  "message router", /* class name*/
  1); /* revision */
  if (pstMessageRouter == 0)
    return EIP_ERROR;

  /* reserved for future use -> set to zero */
  gMRResponse.Reserved = 0;
  gMRResponse.Data = g_acMessageDataReplyBuffer; /* set reply buffer, using a fixed buffer (about 100 bytes) */

  return EIP_OK;
}

/*! get the registered MessageRouter object corresponding to ClassID.
 *  given a class ID, return a pointer to the registration node for that object
 *  @param pa_nClassID      ClassCode to be searched for.
 *  @return pointer to registered MR object
 *      0 .. Class not registered
 */
S_CIP_MR_Object *
getRegisteredObject(EIP_UINT32 pa_nClassID)
{
  S_CIP_MR_Object *p = g_pt2firstObject; /* get pointer to head of class registration list */

  while (p) /* for each entry in list*/
    {
      OPENER_ASSERT(p->pt2Class != NULL);
      if (p->pt2Class->nClassID == pa_nClassID)
        return p; /* return registration node if it matches class ID*/
      p = p->next;
    }
  return 0;
}

S_CIP_Class *
getCIPClass(EIP_UINT32 pa_nClassID)
{
  S_CIP_MR_Object *p = getRegisteredObject(pa_nClassID);

  if (p)
    return p->pt2Class;
  else
    return 0;
}

S_CIP_Instance *
getCIPInstance(S_CIP_Class * pa_pstClass, EIP_UINT32 pa_nInstanceNr)
{
  S_CIP_Instance *p; /* pointer to linked list of instances from the class object*/

  if (pa_nInstanceNr == 0)
    return (S_CIP_Instance *) pa_pstClass; /* if the instance number is zero, return the class object itself*/

  for (p = pa_pstClass->pstInstances; p; p = p->pstNext) /* follow the list*/
    {
      if (p->nInstanceNr == pa_nInstanceNr)
        return p; /* if the number matches, return the instance*/
    }

  return 0;
}

EIP_STATUS
registerClass(S_CIP_Class * pa_pt2Class)
{
  S_CIP_MR_Object **p = &g_pt2firstObject;

  while (*p)
    p = &(*p)->next; /* follow the list until p points to an empty link (list end)*/

  *p = (S_CIP_MR_Object *) IApp_CipCalloc(1, sizeof(S_CIP_MR_Object)); /* create a new node at the end of the list*/
  if (*p == 0)
    return EIP_ERROR; /* check for memory error*/

  (*p)->pt2Class = pa_pt2Class; /* fill in the new node*/
  (*p)->next = NULL;

  return EIP_OK;
}

EIP_STATUS
notifyMR(EIP_UINT8 * pa_pnData, int pa_nDataLength)
{
  EIP_STATUS nRetVal = EIP_OK_SEND;
  EIP_BYTE nStatus;

  gMRResponse.Data = g_acMessageDataReplyBuffer; /* set reply buffer, using a fixed buffer (about 100 bytes) */

  OPENER_TRACE_INFO("notifyMR: routing unconnected message\n");
  if (CIP_ERROR_SUCCESS
      != (nStatus = createMRRequeststructure(pa_pnData, pa_nDataLength,
          &gMRRequest)))
    { /* error from create MR structure*/
      OPENER_TRACE_ERR("notifyMR: error from createMRRequeststructure\n");
      gMRResponse.GeneralStatus = nStatus;
      gMRResponse.SizeofAdditionalStatus = 0;
      gMRResponse.Reserved = 0;
      gMRResponse.DataLength = 0;
      gMRResponse.ReplyService = (0x80 | gMRRequest.Service);
    }
  else
    {
      /* forward request to appropriate Object if it is registered*/
      S_CIP_MR_Object *pt2regObject;

      pt2regObject = getRegisteredObject(gMRRequest.RequestPath.ClassID);
      if (pt2regObject == 0)
        {
          OPENER_TRACE_ERR(
              "notifyMR: sending CIP_ERROR_OBJECT_DOES_NOT_EXIST reply, class id 0x%x is not registered\n",
              (unsigned) gMRRequest.RequestPath.ClassID);
          gMRResponse.GeneralStatus = CIP_ERROR_PATH_DESTINATION_UNKNOWN; /*according to the test tool this should be the correct error flag instead of CIP_ERROR_OBJECT_DOES_NOT_EXIST;*/
          gMRResponse.SizeofAdditionalStatus = 0;
          gMRResponse.Reserved = 0;
          gMRResponse.DataLength = 0;
          gMRResponse.ReplyService = (0x80 | gMRRequest.Service);
        }
      else
        {
          /* call notify function from Object with ClassID (gMRRequest.RequestPath.ClassID)
           object will or will not make an reply into gMRResponse*/
          gMRResponse.Reserved = 0;
          OPENER_ASSERT(NULL != pt2regObject->pt2Class);
          OPENER_TRACE_INFO(
              "notifyMR: calling notify function of class '%s'\n",
              pt2regObject->pt2Class->acName);
          nRetVal = notifyClass(pt2regObject->pt2Class, &gMRRequest,
              &gMRResponse);

#ifdef OPENER_TRACE_ENABLED
          if (nRetVal == EIP_ERROR)
            {
              OPENER_TRACE_ERR(
                  "notifyMR: notify function of class '%s' returned an error\n",
                  pt2regObject->pt2Class->acName);
            }
          else if (nRetVal == EIP_OK)
            {
              OPENER_TRACE_INFO(
                  "notifyMR: notify function of class '%s' returned no reply\n",
                  pt2regObject->pt2Class->acName);
            }
          else
            {
              OPENER_TRACE_INFO(
                  "notifyMR: notify function of class '%s' returned a reply\n",
                  pt2regObject->pt2Class->acName);
            }
#endif
        }
    }
  return nRetVal;
}

EIP_BYTE
createMRRequeststructure(EIP_UINT8 * pa_pnData, EIP_INT16 pa_nLength,
    S_CIP_MR_Request * pa_pstMRReqdata)
{
  int nRetVal;

  pa_pstMRReqdata->Service = *pa_pnData;
  pa_pnData++;
  pa_nLength--;


  nRetVal = decodePaddedEPath(&(pa_pstMRReqdata->RequestPath), &pa_pnData);
  if(nRetVal < 0)
    {
      return CIP_ERROR_PATH_SEGMENT_ERROR;
    }

  pa_pstMRReqdata->Data = pa_pnData;
  pa_pstMRReqdata->DataLength = pa_nLength - nRetVal;

  if (pa_pstMRReqdata->DataLength < 0)
    return CIP_ERROR_PATH_SIZE_INVALID;
  else
    return CIP_ERROR_SUCCESS;
}

void
deleteAllClasses(void)
{
  S_CIP_MR_Object *pstRunner = g_pt2firstObject; /* get pointer to head of class registration list */
  S_CIP_MR_Object *pstToDelete;
  S_CIP_Instance *pstInstRunner, *pstInstDel;

  while (NULL != pstRunner)
    {
      pstToDelete = pstRunner;
      pstRunner = pstRunner->next;

      pstInstRunner = pstToDelete->pt2Class->pstInstances;
      while (NULL != pstInstRunner)
        {
          pstInstDel = pstInstRunner;
          pstInstRunner = pstInstRunner->pstNext;
          if (pstToDelete->pt2Class->nNr_of_Attributes) /* if the class has instance attributes */
            { /* then free storage for the attribute array */
              IApp_CipFree(pstInstDel->pstAttributes);
            }
          IApp_CipFree(pstInstDel);
        }

      /*clear meta class data*/
      IApp_CipFree(pstToDelete->pt2Class->m_stSuper.pstClass->acName);
      IApp_CipFree(pstToDelete->pt2Class->m_stSuper.pstClass->pstServices);
      IApp_CipFree(pstToDelete->pt2Class->m_stSuper.pstClass);
      /*clear class data*/
      IApp_CipFree(pstToDelete->pt2Class->m_stSuper.pstAttributes);
      IApp_CipFree(pstToDelete->pt2Class->pstServices);
      IApp_CipFree(pstToDelete->pt2Class);
      IApp_CipFree(pstToDelete);
    }
  g_pt2firstObject = NULL;
}
