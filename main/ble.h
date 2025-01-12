#ifndef _BLE_H
#define _BLE_H

#define SERV_UUID        0xB00B
#define TOGGLE_CTRL_UUID 0xC0C0
#define READ_KP_UUID     0xAAAA
#define WRIT_KP_UUID     0xAAA1
#define READ_KD_UUID     0xBBBB
#define WRIT_KD_UUID     0xBBB1
#define READ_KI_UUID     0xCCCC
#define WRIT_KI_UUID     0xCCC1
#define PKT_DELIMETER    ';'
#define SIZEOF_RDATA     12

void ble_task(void);

#endif /* _BLE_H */