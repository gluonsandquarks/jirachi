/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * ble.h - exposes ble main task for bluetooh configuration/control
 * 
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 gluons.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

/*
 * @brief Initializes the needed peripherals, configures NimBLE stack,
 *        sets the and GATT services and characteristics, event handling,
 *        data parsing, and update the exposed variables.
 *        sorry no customization cause this is embedded device and i don't need
 *        an overengineered api :P
 *        feel free to mess around with the actual implementation tho
 */
void ble_task(void);

#endif /* _BLE_H */