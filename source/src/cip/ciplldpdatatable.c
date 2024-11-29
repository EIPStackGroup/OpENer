/*******************************************************************************
 * Copyright (c) 2024, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "ciplldpdatatable.h"

EipStatus kCipLldpDataTableInit() {
  CipClass *lldp_data_table_class = NULL;

  if ( ( lldp_data_table_class = CreateCipClass(
           kCipLldpDataTableCode, 7, /* # class attributes */
           7,                    /* # highest class attribute number */
           3,                    /* # class services */
           11,                    /* # instance attributes */
           11,                    /* # highest instance attribute number */
           3,                    /* # instance services */
           0,                    /* # instances - 0 to supress creation */
           "LLDP Data Table", 1, /* # class revision */
           NULL                  /* # function pointer for initialization */
           ) ) == 0 ) {
    return kEipStatusError;
  }
}