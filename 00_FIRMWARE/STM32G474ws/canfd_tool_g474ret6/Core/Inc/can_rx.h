/*
 * can_rx.h
 *
 *  Created on: Feb 16, 2025
 *      Author: Pep
 */

#ifndef INC_CAN_RX_H_
#define INC_CAN_RX_H_


// CAN RX Screen functions //
void init_canRxTable();
void can_bus_load();

// CAN RX Work Functions //
void save_rx_buf_static(uint32_t id, uint8_t dlc, uint8_t * data);
void save_rx_buf_dinamic(uint32_t id, uint8_t dlc, uint8_t * data);
void update_rx_screen_static();
void update_rx_screen_dinamic();
void update_dinamic_buf();
void clear_rxTable();


#endif /* INC_CAN_RX_H_ */
