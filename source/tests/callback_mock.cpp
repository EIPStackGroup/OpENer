/*******************************************************************************
 * This module provides a set of Cpputest mock functions for the OpENer
 * callback API.
 ******************************************************************************/
#include "CppUTestExt/MockSupport.h"
#include <stdio.h>
extern "C" {
  #include "opener_api.h"
}


EipStatus ApplicationInitialization(void) {
  return (EipStatus)mock()
         .actualCall(__func__)
         .returnIntValueOrDefault(kEipStatusOk);
}


void HandleApplication(void) {
  mock().actualCall(__func__);
}


void CheckIoConnectionEvent(unsigned int output_assembly_id,
                            unsigned int input_assembly_id,
                            IoConnectionEvent io_connection_event) {
  mock().actualCall(__func__);
}


EipStatus AfterAssemblyDataReceived(CipInstance *instance) {
  return (EipStatus)mock()
         .actualCall(__func__)
         .returnIntValueOrDefault(kEipStatusOk);
}


EipBool8 BeforeAssemblyDataSend(CipInstance *instance) {
  return mock()
         .actualCall(__func__)
         .returnIntValueOrDefault(false);
}


EipStatus ResetDevice(void) {
  return (EipStatus)mock()
         .actualCall(__func__)
         .returnIntValueOrDefault(kEipStatusError);
}


EipStatus ResetDeviceToInitialConfiguration(void) {
  return (EipStatus)mock()
         .actualCall(__func__)
         .returnIntValueOrDefault(kEipStatusError);
}


void *CipCalloc(size_t number_of_elements,
                size_t size_of_element) {
  MockActualCall& call = mock().actualCall(__func__);

  /*
   * The return value is dependent upon if a predetermined result has been
   * configured via andReturnValue(), which should only be used to
   * simulate an allocation failure by returning NULL. Otherwise, the
   * default is to return a normally-allocated pointer.
   */
  return call.hasReturnValue() ? NULL : calloc(number_of_elements,
                                               size_of_element);
}


void CipFree(void *data) {
  mock().actualCall(__func__);
  free(data);
}


void RunIdleChanged(EipUint32 run_idle_value) {
  mock().actualCall(__func__);
}
