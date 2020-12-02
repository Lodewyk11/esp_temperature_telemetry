
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_server.h"
#include "../wifi/wifi.h"
#include "esp_log.h"

#define DEGUG_LOG "******* DEBUG ******"

/**
 * The vendor specific security test service consists of two characteristics:
 *     o random-number-generator: generates a random 32-bit number each time
 *       it is read.  This characteristic can only be read over an encrypted
 *       connection.
 *     o static-value: a single-byte characteristic that can always be read,
 *       but can only be written over an encrypted connection.
 */

//TODO: Double check UUIDs
/* 59462f12-9543-9999-12c8-58b459a2712d */
static const ble_uuid128_t svc_uuid =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                     0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t get_wifi_aps_chr_uuid =
    BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                     0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

static int get_wifi_aps(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt,
                           void *arg);

static wifi_ap_scan_t wifi_ap_scan;

static char response[1024];

const struct ble_gatt_svc_def services[] = {
    {
        /*** Service: Wifi service. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {/*** Characteristic: Get Temperature. */
             .uuid = &get_wifi_aps_chr_uuid.u,
             .access_cb = get_wifi_aps,
             .flags = BLE_GATT_CHR_F_READ},
            {
                0, /* No more characteristics in this service. */
            }},
    },

    {
        0, /* No more services. */
    },
};

// static int process_write(struct os_mbuf *om, uint16_t max_len,
//                          void *dst, uint16_t *len)
// {
//     uint16_t om_len;
//     int rc;
//     om_len = OS_MBUF_PKTLEN(om);
//     if (om_len > max_len)
//     {
//         return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//     }

//     rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
//     if (rc != 0)
//     {
//         return BLE_ATT_ERR_UNLIKELY;
//     }
//     printf(dst);

//     return 0;
// }


int get_wifi_aps(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt,
                    void *arg)

{
    ESP_LOGE(DEGUG_LOG, "Getting wifi access points");
    const ble_uuid_t *uuid;
    uuid = ctxt->chr->uuid;
    int rc;
    if (ble_uuid_cmp(uuid, &get_wifi_aps_chr_uuid.u) == 0)
    {
        switch (ctxt->op)
        {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            scan_for_wifi();
            wifi_ap_scan = get_aps();
            if (wifi_ap_scan.success == ESP_OK)
            {
                sprintf(response,"%s", wifi_ap_scan.value);
                ESP_LOGE(DEGUG_LOG, "Writing value  %s", response);
                rc = os_mbuf_append(ctxt->om, response, strlen(response) * sizeof(char));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            else
            {
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }
        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }
    assert(0);
    return 0;
}

// static int process_read_write(uint16_t conn_handle, uint16_t attr_handle,
//                               struct ble_gatt_access_ctxt *ctxt,
//                               void *arg)
// {
//     const ble_uuid_t *uuid;
//     uuid = ctxt->chr->uuid;
//     int rc;
//     if (ble_uuid_cmp(uuid, &chr_uuid.u) == 0)
//     {
//         switch (ctxt->op)
//         {
//         case BLE_GATT_ACCESS_OP_READ_CHR:
//             printf("BLE_GATT_ACCESS_OP_READ_CHR: Returning a static value\n");
//             char *message = "It's working";
//             rc = os_mbuf_append(ctxt->om, message,
//                                 strlen(message) * sizeof(char));
//             return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

//         case BLE_GATT_ACCESS_OP_WRITE_CHR:
//             printf("BLE_GATT_ACCESS_OP_WRITE_CHR: Figuring out what to do here\n");
//             rc = process_write(ctxt->om,
//                                sizeof command,
//                                &command, NULL);
//             return rc;

//         default:
//             assert(0);
//             return BLE_ATT_ERR_UNLIKELY;
//         }
//     }

//     /* Unknown characteristic; the nimble stack should not have called this
//      * function.
//      */
//     assert(0);
//     return BLE_ATT_ERR_UNLIKELY;
// }

void gatt_server_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op)
    {
    case BLE_GATT_REGISTER_OP_SVC:
        MODLOG_DFLT(INFO, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        MODLOG_DFLT(INFO, "registering characteristic %s with "
                          "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        MODLOG_DFLT(INFO, "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int gatt_server_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(services);
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(services);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}