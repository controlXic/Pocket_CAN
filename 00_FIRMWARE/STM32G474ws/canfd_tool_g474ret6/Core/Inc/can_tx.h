/*
 * can_tx.h
 *
 *  Created on: Mar 3, 2025
 *      Author: Pep
 */

#ifndef INC_CAN_TX_H_
#define INC_CAN_TX_H_


// CAN TX Screen functions //
void init_canTxTable();
void can_tx_keyboard();
void update_text_areas();
void error_msgbox(const char * error);

// CAN TX Work Functions //
void save_tx_buf(const char * id, const char * dlc, const char * b0, const char * b1, const char * b2,
				 const char * b3, const char * b4, const char * b5, const char * b6, const char * b7);
void update_tx_screen();
void send_tx_frame();
void remove_tx_frame(uint32_t nFrame);
void clear_txTable();



#endif /* INC_CAN_TX_H_ */

