#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

#define DEVICE_NAME "esp32测试"
#define APP_ID 0

// 广播
void adv()
{
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = false,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
        .p_manufacturer_data = 0,
        .p_service_data = 0,
        .p_service_uuid = 0,
    };
    esp_ble_gap_config_adv_data(&adv_data);
    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,
        .adv_int_max = 0x40,
        .adv_type = ADV_TYPE_IND,
        .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
        .channel_map = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
    };
    esp_ble_gap_start_advertising(&adv_params);
}

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
    // gap cb
    // 安全响应
    // 配对完成
    // 解绑时
    switch (event) {
    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI("debug", "安全请求响应");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        ESP_LOGI("debug", "配对状态: %d", param->ble_security.auth_cmpl.success);
        break;
    case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
        ESP_LOGI("debug", "解绑");
        break;
    default:
        break;
    }
}

void gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param)
{
    // gatts cb
    // 创建属性表
    // 安全连接
    // 连接与广播操作
    switch (event) {
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI("debug", "连接");
        esp_ble_gap_stop_advertising();
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI("debug", "断连");
        adv();
        break;

    default:
        break;
    }
}

void app_main(void)
{
    esp_err_t ret;
    // FALSH
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 控制器
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t controller_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&controller_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    // 协议栈
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    esp_bluedroid_enable();

    // gap
    esp_ble_gap_register_callback(gap_cb);
    // gatts
    esp_ble_gatts_register_callback(gatts_cb);
    // 广播
    esp_ble_gap_set_device_name(DEVICE_NAME);
    adv();
    // 注册应用
    esp_ble_gatts_app_register(APP_ID);

    // SPP配置
    // -配对方式 IO OBB
    uint8_t iocap = ESP_IO_CAP_OUT;
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(iocap));
    uint8_t oob = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob, sizeof(oob));
    uint32_t pin = 509509;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &pin, sizeof(uint32_t));
    // -认证选项
    uint8_t authen = ESP_LE_AUTH_BOND | ESP_LE_AUTH_REQ_MITM | ESP_LE_AUTH_REQ_SC_ONLY;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &authen, sizeof(authen));
    uint8_t only_accept = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &only_accept, sizeof(only_accept));

    // -密钥
    uint8_t key_size = 16;
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(key_size));
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK | ESP_BLE_CSR_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(init_key));
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK | ESP_BLE_CSR_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(rsp_key));
}