/*
 * This file is part of the jirachi repository, https://github.com/gluonsandquarks/jirachi
 * ble.c - configuration of BLE functionality through the NimBLE stack, custom callback
 * functions to handle data read/write, data parser, setup of the GATT service and
 * characteristics, bluetooth event handler, etc. This shit is not secure in any way
 * other than the security provided by the NimBLE stack, everyone can connect and send
 * data to the device, please don't use in safety-critical environments lol
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

#include <stdio.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "globals.h"
#include "ble.h"

/* TODO: the read and write operations of the PID constants variables aren't technically thread safe */
/*       they need a mutex but i'm lazy and since this thread only read/writes while the other just  */
/*       reads (and multiple times), it is ok so whatever :PPP also im too lazy rn to do that        */

static uint8_t ble_addr_type;

void ble_app_advertise(void); /* forward declare this function cause api is shit >:) */

static uint8_t parse_rx_data(char *raw_data, char *parsed_data)
{
    uint8_t pkt_delim = 0U;

    /* check for packet delimeter */
    for (int i = 0U; i < SIZEOF_RDATA; i++)
    {
        if (raw_data[i] == PKT_DELIMETER)
        {
            pkt_delim = 1U;
            break;
        }
        else
        {
            parsed_data[i] = raw_data[i];
        }
    }

    ESP_LOGD("parse_rx_data", "parsed_data = %s, pkt_delim = %d", parsed_data, pkt_delim);

    if (!pkt_delim)
    {
        ESP_LOGD("parse_rx_data", "pkt_delim = 0");
        return 0U;
    }

    /* check for empty string and non digits */
    /* TODO: need to implement decimal point handling */
    for (uint8_t i = 0U; i < SIZEOF_RDATA; i++)
    {
        if (i == 0U && parsed_data[i] == '\0') { return 0U; } /* return if string is empty */
        if (parsed_data[i] == '\0') { break; }
        if (!(parsed_data[i] >= '0' && parsed_data[i] <= '9'))
        {
            ESP_LOGI("parse_rx_data", "parsed_data contains non-digit chars");
            return 0U;
        }
    }

    return 1U;
}

/* callback functions for BLE characteristics  */
static int toggle_control(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = (char *)ctxt->om->om_data;
    char *on = "1";
    char *off = "0";

    /* just ignore other messages lmao */
    if (strncmp(data, on, 1) == 0) { control_active = 1; return 0; }
    if (strncmp(data, off, 1) == 0) { control_active = 0; return 0; }

    return 0;
}

static int read_kp(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char buffer[SIZEOF_RDATA] = { 0 };
    snprintf(buffer, SIZEOF_RDATA, "%.2f", pid_kp);
    os_mbuf_append(ctxt->om, buffer, strlen(buffer));
    return 0;
}

static int update_kp(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = (char *)ctxt->om->om_data;
    char parsed_data[SIZEOF_RDATA] = { 0 };

    if (!parse_rx_data(data, parsed_data)) { return 0; } /* don't update value if we don't recieve sensible data */
    pid_kp = strtof((char *)parsed_data, NULL); /* update variable */

    return 0;
}

static int read_kd(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char buffer[SIZEOF_RDATA] = { 0 };
    snprintf(buffer, SIZEOF_RDATA, "%.2f", pid_kd);
    os_mbuf_append(ctxt->om, buffer, strlen(buffer));
    return 0;
}

static int update_kd(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = (char *)ctxt->om->om_data;
    char parsed_data[SIZEOF_RDATA] = { 0 };

    if (!parse_rx_data(data, parsed_data)) { return 0; } /* don't update value if we don't recieve sensible data */
    pid_kd = strtof((char *)parsed_data, NULL); /* update variable */

    return 0;
}

static int read_ki(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char buffer[SIZEOF_RDATA] = { 0 };
    snprintf(buffer, SIZEOF_RDATA, "%.2f", pid_ki);
    os_mbuf_append(ctxt->om, buffer, strlen(buffer));
    return 0;
}

static int update_ki(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *data = (char *)ctxt->om->om_data;
    char parsed_data[SIZEOF_RDATA] = { 0 };

    if (!parse_rx_data(data, parsed_data)) { return 0; } /* don't update value if we don't recieve sensible data */
    pid_ki = strtof((char *)parsed_data, NULL); /* update variable */

    return 0;
}

/* array of pointers to other service definitions */
/* UUID - Universal Unique Identifier */
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(SERV_UUID),
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(TOGGLE_CTRL_UUID),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = toggle_control},
         {.uuid = BLE_UUID16_DECLARE(READ_KP_UUID),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = read_kp},
         {.uuid = BLE_UUID16_DECLARE(WRIT_KP_UUID),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = update_kp},
         {.uuid = BLE_UUID16_DECLARE(READ_KD_UUID),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = read_kd},
         {.uuid = BLE_UUID16_DECLARE(WRIT_KD_UUID),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = update_kd},
         {.uuid = BLE_UUID16_DECLARE(READ_KI_UUID),
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = read_ki},
         {.uuid = BLE_UUID16_DECLARE(WRIT_KI_UUID),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = update_ki},
         {0}}},
    {0}};

/* BLE event handling */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    /* advertise if connected */
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGD("ble_gap_event", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    /* advertise again after completion of the event */
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGD("ble_gap_event", "BLE GAP EVENT DISCONNECTED");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGD("ble_gap_event", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

/* define the BLE connection */
void ble_app_advertise(void)
{
    /* GAP - device name definition */
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); /* Read the BLE device name */
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    /* GAP - device connectivity definition */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; /* connectable or non-connectable */
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; /* discoverable or non-discoverable */
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

/* the application */
static void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); /* determines the best address type automatically, privacy mode 0 */
    ble_app_advertise();                     /* define the BLE connection */
}

/* the infinite task */
static void host_task(void *param)
{
    nimble_port_run(); /* this function will return only when nimble_port_stop() is executed */
}

void ble_task(void)
{
    nvs_flash_init();                          /* init non volatile memory*/
    nimble_port_init();                        /* init nimble stack in server mode */
    ble_svc_gap_device_name_set("BLE-Server"); /* config server name */
    ble_svc_gap_init();                        /* config gap service */
    ble_svc_gatt_init();                       /* config gatt service */
    ble_gatts_count_cfg(gatt_svcs);            /* config gatt services */
    ble_gatts_add_svcs(gatt_svcs);             /* queues gatt services */
    ble_hs_cfg.sync_cb = ble_app_on_sync;      /* point to init function */
    nimble_port_freertos_init(host_task);      /* run the host_task */
    vTaskDelete(NULL);
}