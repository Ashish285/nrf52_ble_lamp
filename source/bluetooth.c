#include "ble.h"
#include "bluetooth.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_state.h"
#include "nrf_log.h"
#include "ble_nus.h"
#include "app_timer.h"
#include "pwm.h"
#include "app_fds.h"
#include "gpio.h"
#include "rainbow.h"

#define DEVICE_NAME                         "Lamp"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

#define MIN_CONN_INTERVAL                   MSEC_TO_UNITS(10, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.32 seconds). */
#define MAX_CONN_INTERVAL                   MSEC_TO_UNITS(15, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                       0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                    MSEC_TO_UNITS(20000, UNIT_10_MS)         /**< Connection supervisory timeout (20 seconds). */
#define APP_ADV_INTERVAL                160                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 2s). */
#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define SECURITY_REQUEST_DELAY          APP_TIMER_TICKS(400)                        /**< Delay after connection until Security Request is sent, if necessary (ticks). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                      1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                      0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                      0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS                  0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES           BLE_GAP_IO_CAPS_DISPLAY_ONLY            /**< Display I/O capabilities. */
#define SEC_PARAM_OOB                       0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE              7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE              16                                      /**< Maximum encryption key size. */

#define PASSKEY_TXT                     "Passkey:"                                  /**< Message to be displayed together with the pass-key. */
#define PASSKEY_TXT_LENGTH              8                                           /**< Length of message to be displayed together with the pass-key. */
#define PASSKEY_LENGTH                  6                                           /**< Length of pass-key received by the stack for display. */

#define APP_BLE_CONN_CFG_TAG                1                                       /**< A tag identifying the SoftDevice BLE configuration. */
#define APP_BLE_OBSERVER_PRIO               3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */

NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */
BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
APP_TIMER_DEF(m_sec_req_timer_id);                                                  /**< Security Request timer. */

//bt bt_status = status_uninit;
static pm_peer_id_t m_peer_to_be_deleted = PM_PEER_ID_INVALID;
uint8_t passw[6] = {0};
char ble_data[25]={0};
static uint16_t m_conn_handle         = BLE_CONN_HANDLE_INVALID;                   /**< Handle of the current connection. */
static uint16_t m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;             /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[] =                                                  /**< Universally unique service identifiers. */
{
  {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

int red_duty = 0;
int blue_duty = 0;
int green_duty = 0;
int alpha_duty = 0;

/**************************Private Function Definitions**************************/


/**
* @brief             Send a string over to the connected BLE device
* @param[in]         data      The string to send
* @param[in]         length    Length of the string to send
* @retval            NONE
*/
void send_data_ble(unsigned char *data,uint16_t length)
{
  ble_nus_data_send(&m_nus,data,&length,m_conn_handle);
}


/**
* @brief             Start the advertisement
* @param[in]         NONE
* @retval            NONE
*/
void advertising_start(void)
{
  
  uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
  APP_ERROR_CHECK(err_code);
  
  // green_led_on();
}


/**@brief Function for handling BLE events.
*
* @param[in]   p_ble_evt   Bluetooth stack event.
* @param[in]   p_context   Unused.
*/
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
  ret_code_t err_code;
  
  switch (p_ble_evt->header.evt_id)
  {
  case BLE_GAP_EVT_CONNECTED:
    NRF_LOG_INFO("Connected");
    m_peer_to_be_deleted = PM_PEER_ID_INVALID;
    //is_advertising = false;
    //Use LED to indicate connection status
    led1_on();
    m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
    APP_ERROR_CHECK(err_code);
    // Start Security Request timer.
    /*err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
    APP_ERROR_CHECK(err_code);*/
    break;
    
  case BLE_GAP_EVT_DISCONNECTED:
    NRF_LOG_INFO("Disconnected, reason %d.",
                 p_ble_evt->evt.gap_evt.params.disconnected.reason);
    m_conn_handle = BLE_CONN_HANDLE_INVALID;
    if (m_peer_to_be_deleted != PM_PEER_ID_INVALID)
    {
      err_code = pm_peer_delete(m_peer_to_be_deleted);
      APP_ERROR_CHECK(err_code);
      NRF_LOG_DEBUG("Collector's bond deleted");
      m_peer_to_be_deleted = PM_PEER_ID_INVALID;
    }
    led1_off();
   // advertising_start();
    break;
    
  case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
      NRF_LOG_DEBUG("PHY update request.");
      ble_gap_phys_t const phys =
      {
        .rx_phys = BLE_GAP_PHY_AUTO,
        .tx_phys = BLE_GAP_PHY_AUTO,
      };
      err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
      APP_ERROR_CHECK(err_code);
    } break;
    
  case BLE_GATTC_EVT_TIMEOUT:
    // Disconnect on GATT Client timeout event.
    NRF_LOG_DEBUG("GATT Client Timeout.");
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                     BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break;
    
  case BLE_GATTS_EVT_TIMEOUT:
    // Disconnect on GATT Server timeout event.
    NRF_LOG_DEBUG("GATT Server Timeout.");
    err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                     BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
    break;
  case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
    NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
    break;
    
  case BLE_GAP_EVT_PASSKEY_DISPLAY:
    {
      char passkey[PASSKEY_LENGTH + 1];
      memcpy(passkey, p_ble_evt->evt.gap_evt.params.passkey_display.passkey, PASSKEY_LENGTH);
      passkey[PASSKEY_LENGTH] = 0;
      // Don't send delayed Security Request if security procedure is already in progress.
      err_code = app_timer_stop(m_sec_req_timer_id);
      APP_ERROR_CHECK(err_code);
      
      NRF_LOG_INFO("Passkey: %s", nrf_log_push(passkey));
    } break;
    
  default:
    // No implementation needed.
    break;
  }
}


/**
* @brief             Initialise the BLE stack
* @param[in]         NONE
* @retval            NONE
*/
void init_ble_stack()
{
  ret_code_t err_code;
  
  err_code = nrf_sdh_enable_request();
  APP_ERROR_CHECK(err_code);
  
  // Configure the BLE stack using the default settings.
  // Fetch the start address of the application RAM.
  uint32_t ram_start = 0;
  err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
  APP_ERROR_CHECK(err_code);
  
  // Enable BLE stack.
  err_code = nrf_sdh_ble_enable(&ram_start);
  APP_ERROR_CHECK(err_code);
  
  // Register a handler for BLE events.
  NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**
* @brief             Initialise the GAP parameters
* @param[in]         NONE
* @retval            NONE
*/
void gap_params_init()
{
  ret_code_t              err_code;
  ble_gap_conn_params_t   gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;
  
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
  
  err_code = sd_ble_gap_device_name_set(&sec_mode,
                                        (const uint8_t *)DEVICE_NAME,
                                        strlen(DEVICE_NAME));
  APP_ERROR_CHECK(err_code);
  
  /* 
  err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_THERMOMETER);
  APP_ERROR_CHECK(err_code);
  */
  memset(&gap_conn_params, 0, sizeof(gap_conn_params));
  
  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency     = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
  
  err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
  APP_ERROR_CHECK(err_code);
  
  static ble_opt_t optS;
  
  optS.gap_opt.passkey.p_passkey=passw;
  
  err_code =sd_ble_opt_set(BLE_GAP_OPT_PASSKEY, &optS);
}


/**
* @brief             Event handler for the GATT module
* @param[in]         NONE
* @retval            NONE
*/
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
  if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
  {
    m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
    NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
  }
  NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                p_gatt->att_mtu_desired_central,
                p_gatt->att_mtu_desired_periph);
}


/**
* @brief             Initialise the GATT server
* @param[in]         NONE
* @retval            NONE
*/
void gatt_init(void)
{
  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
  APP_ERROR_CHECK(err_code);
  
  err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
  APP_ERROR_CHECK(err_code);
}


/**
* @brief             Event handler for the advertisement
* @param[in]         NONE
* @retval            NONE
*/
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
  
  switch (ble_adv_evt)
  {
  case BLE_ADV_EVT_FAST:
    led2_on();
    // err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    //  APP_ERROR_CHECK(err_code);
    break;
  case BLE_ADV_EVT_IDLE:
    led2_off();
    //   sleep_mode_enter();
    break;
  default:
    break;
  }
}


/**
* @brief             Initialise advertisement
* @param[in]         NONE
* @retval            NONE
*/
void init_advertising()
{
  ret_code_t             err_code;
  ble_advertising_init_t init;
  
  memset(&init, 0, sizeof(init));
  
  init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
  init.advdata.include_appearance      = false;
  init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
  //init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  init.advdata.uuids_complete.p_uuids  = m_adv_uuids;
  
  init.config.ble_adv_fast_enabled  = true;
  init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
  init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
  
  init.evt_handler = on_adv_evt;
  
  err_code = ble_advertising_init(&m_advertising, &init);
  APP_ERROR_CHECK(err_code);
  
  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
  
  
  int8_t tx_power = -16;
  err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV,m_advertising.adv_handle,tx_power);
  APP_ERROR_CHECK(err_code);
}


/**
* @brief             QWR(Queued Write Request) error handler
* @param[in]         NONE
* @retval            NONE
*/
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
  APP_ERROR_HANDLER(nrf_error);
}

/**
* @brief             Checks the bluetooth command and responds appropriately
* @param[in]         data               String with the comand
* @retval            NONE
*/
void parse_ble_data(char *data)
{
  if(!strncmp(data,"rainbow",7))
  {
    do_rainbow();
    return;
  }
  if(strlen(data) != 11)
  {
    //Complete string not received, no need to do anything
    return;
  }
  
  red_duty = ((data[0] - '0')*10) + (data[1] - '0');
  green_duty = ((data[3] - '0')*10) + (data[4] - '0');
  blue_duty = ((data[6] - '0')*10) + (data[7] - '0');
  alpha_duty = ((data[9] - '0')*10) + (data[10] - '0');
  
  pwm_set_duty_cycle(red_duty, green_duty, blue_duty, alpha_duty);
  
  update_record_in_flash(data);
}

/**
* @brief             NUS(Nordic UART  Service) data handler
* @param[in]         NONE
* @retval            NONE
*/
static void nus_data_handler(ble_nus_evt_t * p_evt)
{
  if(p_evt->type == BLE_NUS_EVT_RX_DATA)
  {
    memcpy(ble_data,p_evt->params.rx_data.p_data,(p_evt->params.rx_data.length));
    // ble_data[(p_evt->params.rx_data.length)] = '\r';
    // ble_data[(p_evt->params.rx_data.length)+1] = '\n';
    parse_ble_data(ble_data);
    NRF_LOG_INFO("Received data is %s",ble_data);
    memset(ble_data,'\0',sizeof(ble_data));
  }
  
  else if(p_evt->type == BLE_NUS_EVT_COMM_STARTED)
  {
    NRF_LOG_INFO("Notification enabled");
  }
  
  else if(p_evt->type == BLE_NUS_EVT_COMM_STOPPED)
  {
    //Status.bt_status = status_notification_disable;
    NRF_LOG_INFO("Notification disabled");
  }
}


/**
* @brief             Initialise the BLE services
* @param[in]         NONE
* @retval            NONE
*/
void services_init()
{
  uint32_t           err_code;
  ble_nus_init_t     nus_init;
  nrf_ble_qwr_init_t qwr_init = {0};
  
  // Initialize Queued Write Module.
  qwr_init.error_handler = nrf_qwr_error_handler;
  
  err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
  APP_ERROR_CHECK(err_code);
  
  // Initialize NUS.
  memset(&nus_init, 0, sizeof(nus_init));
  
  nus_init.data_handler = nus_data_handler;
  
  err_code = ble_nus_init(&m_nus, &nus_init);
  APP_ERROR_CHECK(err_code);
}

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
  uint32_t err_code;
  
  if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
  {
    err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
    APP_ERROR_CHECK(err_code);
  }
}

/**
* @brief             Error handler for the connection parameters
* @param[in]         NONE
* @retval            NONE
*/
static void conn_params_error_handler(uint32_t nrf_error)
{
  //APP_ERROR_HANDLER(nrf_error);
}


/**
* @brief             Connection parameters Initialisation
* @param[in]         NONE
* @retval            NONE
*/
void conn_params_init()
{
  uint32_t               err_code;
  ble_conn_params_init_t cp_init;
  
  memset(&cp_init, 0, sizeof(cp_init));
  
  cp_init.p_conn_params                  = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail             = false;
  cp_init.evt_handler                    = on_conn_params_evt;
  cp_init.error_handler                  = conn_params_error_handler;
  
  err_code = ble_conn_params_init(&cp_init);
  NRF_LOG_INFO("Conn params error %d",err_code);
  APP_ERROR_CHECK(err_code);
}



/**
* @brief             Stops the advertisement
* @param[in]         NONE
* @retval            NONE
*/
void stop_advertise(void *p_context)
{
  UNUSED_PARAMETER(p_context);
  
  sd_ble_gap_adv_stop(m_advertising.adv_handle);
  NRF_LOG_INFO("Stop Advertisement");
  
  
}

/**
* @brief             Disconnect from a connected device
* @param[in]         NONE
* @retval            NONE
*/
void disconnect()
{
  uint32_t err_code;
  err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
  NRF_LOG_INFO("err %d",err_code);
  APP_ERROR_CHECK(err_code);
}


/**
* @brief             Peer manager event handler
* @param[in]         NONE
* @retval            NONE
*/
static void pm_evt_handler(pm_evt_t const * p_evt)
{
  ret_code_t err_code;
  
  switch (p_evt->evt_id)
  {
  case PM_EVT_BONDED_PEER_CONNECTED:
    {
      NRF_LOG_INFO("Connected to a previously bonded device.");
      // Start Security Request timer.
      //err_code = app_timer_start(m_sec_req_timer_id, SECURITY_REQUEST_DELAY, NULL);
      //APP_ERROR_CHECK(err_code);
    } break;
    
  case PM_EVT_CONN_SEC_SUCCEEDED:
    {
      pm_conn_sec_status_t conn_sec_status;
      
      // Check if the link is authenticated (meaning at least MITM).
      err_code = pm_conn_sec_status_get(p_evt->conn_handle, &conn_sec_status);
      APP_ERROR_CHECK(err_code);
      
      if (conn_sec_status.mitm_protected)
      {
        NRF_LOG_INFO("Link secured. Role: %d. conn_handle: %d, Procedure: %d",
                     ble_conn_state_role(p_evt->conn_handle),
                     p_evt->conn_handle,
                     p_evt->params.conn_sec_succeeded.procedure);
      }
      else
      {
        // The peer did not use MITM, disconnect.
        NRF_LOG_INFO("Collector did not use MITM, disconnecting");
        err_code = pm_peer_id_get(m_conn_handle, &m_peer_to_be_deleted);
        APP_ERROR_CHECK(err_code);
        err_code = sd_ble_gap_disconnect(m_conn_handle,
                                         BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
      }
    } break;
    
  case PM_EVT_CONN_SEC_FAILED:
    {
      /* Often, when securing fails, it shouldn't be restarted, for security reasons.
      * Other times, it can be restarted directly.
      * Sometimes it can be restarted, but only after changing some Security parameters.
      * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
      * Sometimes it is impossible, to secure the link, or the peer device does not support it.
      * How to handle this error is highly application dependent. */
      NRF_LOG_INFO("Failed to secure connection. Disconnecting.");
      err_code = sd_ble_gap_disconnect(m_conn_handle,
                                       BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
      if (err_code != NRF_ERROR_INVALID_STATE)
      {
        APP_ERROR_CHECK(err_code);
      }
      m_conn_handle = BLE_CONN_HANDLE_INVALID;
    } break;
    
  case PM_EVT_CONN_SEC_CONFIG_REQ:
    {
      // Reject pairing request from an already bonded peer.
      pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
      pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
    } break;
    
  case PM_EVT_STORAGE_FULL:
    {
      // Run garbage collection on the flash.
      err_code = fds_gc();
      if (err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
      {
        // Retry.
      }
      else
      {
        APP_ERROR_CHECK(err_code);
      }
    } break;
    
  case PM_EVT_PEERS_DELETE_SUCCEEDED:
    {
      NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED");
      advertising_start();
    } break;
    
  case PM_EVT_PEER_DATA_UPDATE_FAILED:
    {
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
    } break;
    
  case PM_EVT_PEER_DELETE_FAILED:
    {
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
    } break;
    
  case PM_EVT_PEERS_DELETE_FAILED:
    {
      // Assert.
      APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
    } break;
    
  case PM_EVT_ERROR_UNEXPECTED:
    {
      // Assert.
      APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
    } break;
    
  case PM_EVT_CONN_SEC_START:
  case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
  case PM_EVT_PEER_DELETE_SUCCEEDED:
  case PM_EVT_LOCAL_DB_CACHE_APPLIED:
  case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
    // This can happen when the local DB has changed.
  case PM_EVT_SERVICE_CHANGED_IND_SENT:
  case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
  default:
    break;
  }
}

/**@brief Function for handling the Security Request timer timeout.
*
* @details This function will be called each time the Security Request timer expires.
*
* @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
*                       app_start_timer() call to the timeout handler.
*/
static void sec_req_timeout_handler(void * p_context)
{
  ret_code_t err_code;
  
  if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
  {
    // Initiate bonding.
    NRF_LOG_DEBUG("Start encryption");
    err_code = pm_conn_secure(m_conn_handle, false);
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
      APP_ERROR_CHECK(err_code);
    }
  }
}

void ble_timer_init()
{
  uint32_t err_code = app_timer_create(&m_sec_req_timer_id,
                                       APP_TIMER_MODE_SINGLE_SHOT,
                                       sec_req_timeout_handler);
  APP_ERROR_CHECK(err_code);
}

/**
* @brief             FDS(Flash Data Storage) event handler
* @param[in]         NONE
* @retval            NONE
*/
static void fds_evt_handler(fds_evt_t const * const p_evt)
{
  if (p_evt->id == FDS_EVT_GC)
  {
    NRF_LOG_DEBUG("GC completed\n");
  }
}


/**
* @brief             Peer manager initialisation 
* @param[in]         NONE
* @retval            NONE
*/
void peer_manager_init(void)
{
  ble_gap_sec_params_t sec_param;
  ret_code_t           err_code;
  
  err_code = pm_init();
  APP_ERROR_CHECK(err_code);
  
  memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));
  
  // Security parameters to be used for all security procedures.
  sec_param.bond           = SEC_PARAM_BOND;
  sec_param.mitm           = SEC_PARAM_MITM;
  sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
  sec_param.oob            = SEC_PARAM_OOB;
  sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
  sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
  sec_param.kdist_own.enc  = 1;
  sec_param.kdist_own.id   = 1;
  sec_param.kdist_peer.enc = 1;
  sec_param.kdist_peer.id  = 1;
  /*
  err_code = pm_sec_params_set(&sec_param);
  APP_ERROR_CHECK(err_code);
  */
  err_code = pm_register(pm_evt_handler);
  APP_ERROR_CHECK(err_code);
  
  err_code = fds_register(fds_evt_handler);
  APP_ERROR_CHECK(err_code);
}


