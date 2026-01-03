/*******************************************************************************
 * Copyright (c) 2021,Peter Christen
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_PORTS_STM32_OPENER_H_
#define SRC_PORTS_STM32_OPENER_H_

/**
 * @brief  Start OpENer Ethernet/IP stack
 * @param  none
 * @retval None
 */
void opener_init(struct netif* netif);

#endif  // SRC_PORTS_STM32_OPENER_H_
