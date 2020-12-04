#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "bluetooth.h"
#include "console.h"
#include "gatt_server.h"

#define DEVICE_NAME "Vacation Hydration"
#define LOG_TAG "bluetooth"

#define PASSKEY 1234

static uint8_t own_addr_type;
static int on_gap_event(struct ble_gap_event *event, void *arg);
void ble_store_config_init(void);

void print_addr(const uint8_t *addr)
{
    MODLOG_DFLT(INFO, "Device address: ");
    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

static void print_conn_desc(struct ble_gap_conn_desc *desc)
{
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                      "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
    MODLOG_DFLT(INFO, "\n");
}

static void on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;
    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]){
        BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)};

    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != ESP_OK)
    {
        MODLOG_DFLT(ERROR, "Error setting advertisement fields. rc=%d\n", rc);
        return;
    }

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, on_gap_event, NULL);
    if (rc != ESP_OK)
    {
        MODLOG_DFLT(ERROR, "Error enabling advertisement. rc=%d\n", rc);
        return;
    }
}


static int on_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        MODLOG_DFLT(INFO, "Connection %s. Status=%d",
                    event->connect.status == 0 ? "Established" : "Failed",
                    event->connect.status);

        if (event->connect.status == 0)
        {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == ESP_OK);
            print_conn_desc(&desc);
        }
        else
        {
            advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "Disconnected. reason=%d", event->disc_complete.reason);
        print_conn_desc(&event->disconnect.conn);

        advertise();
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        MODLOG_DFLT(INFO, "Connection updated. status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == ESP_OK);
        print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "Advertise complete. reason=%d",
                    event->adv_complete.reason);
        advertise();
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        MODLOG_DFLT(INFO, "Encryption changed. status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == ESP_OK);
        print_conn_desc(&desc);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        MODLOG_DFLT(INFO, "Subscribe event. conn_handle=%d attr_handle=%d "
                          "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "MTU update event. conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == ESP_OK);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        ESP_LOGI(LOG_TAG, "PASS_KEY_ACTION_EVENT started\n");
        struct ble_sm_io pkey = {0};
        int key = 0;

        if (event->passkey.params.action == BLE_SM_IOACT_DISP)
        {   
            pkey.action = event->passkey.params.action;
            pkey.passkey = PASSKEY;
            ESP_LOGI(LOG_TAG, "Enter passkey %d on the peer side", pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(LOG_TAG, "ble_sm_inject_io result: %d\n", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP)
        {
            ESP_LOGI(LOG_TAG, "Passkey on device's display: %d", event->passkey.params.numcmp);
            ESP_LOGI(LOG_TAG, "Accept or reject the passkey through console in this format -> key Y or key N");
            pkey.action = event->passkey.params.action;
            if (console_receive_key(&key))
            {
                pkey.numcmp_accept = key;
            }
            else
            {
                pkey.numcmp_accept = 0;
                ESP_LOGE(LOG_TAG, "Timeout! Rejecting the key");
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(LOG_TAG, "ble_sm_inject_io result: %d\n", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_OOB)
        {
            static uint8_t tem_oob[16] = {0};
            pkey.action = event->passkey.params.action;
            for (int i = 0; i < 16; i++)
            {
                pkey.oob[i] = tem_oob[i];
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(LOG_TAG, "ble_sm_inject_io result: %d\n", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_INPUT)
        {
            ESP_LOGI(LOG_TAG, "Enter the passkey through console in this format-> key 123456");
            pkey.action = event->passkey.params.action;
            if (console_receive_key(&key))
            {
                pkey.passkey = key;
            }
            else
            {
                pkey.passkey = 0;
                ESP_LOGE(LOG_TAG, "Timeout! Passing 0 as the key");
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(LOG_TAG, "ble_sm_inject_io result: %d\n", rc);
        }
        return 0;
    }
    return 0;
}

static void on_sync(void)
{
    int rc;
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == ESP_OK);

    //Figure out address to use while advertising. No privacy for now.
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != ESP_OK)
    {
        MODLOG_DFLT(ERROR, "Error determining the address type. rc=%d\n", rc);
        return;
    }

    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");
    /* Begin advertising. */
    advertise();
}

void host_task(void *param)
{
    ESP_LOGI(LOG_TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}


void init_ble()
{    
    int rc;
    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());
    nimble_port_init();


    ESP_LOGE("***** DEBUG *****", "Initializing BLE");
    //Initialize NimBLE host configuration.
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_server_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_svc_gap_init();
    
    rc = gatt_server_init();
    assert(rc == ESP_OK);
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_store_config_init();
    nimble_port_freertos_init(host_task);
    rc= console_init();
    if(rc != ESP_OK) {
        ESP_LOGI(LOG_TAG, "Console intialization failed");
    }
}