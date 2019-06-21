#include "fds.h"
#include "bluetooth.h"
#include "app_fds.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

/**************************Private Function Definitions**************************/

/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
  "FDS_EVT_INIT",
  "FDS_EVT_WRITE",
  "FDS_EVT_UPDATE",
  "FDS_EVT_DEL_RECORD",
  "FDS_EVT_DEL_FILE",
  "FDS_EVT_GC",
};


/* Array to map FDS return values to strings. */
char const * fds_err_str[] =
{
  "FDS_SUCCESS",
  "FDS_ERR_OPERATION_TIMEOUT",
  "FDS_ERR_NOT_INITIALIZED",
  "FDS_ERR_UNALIGNED_ADDR",
  "FDS_ERR_INVALID_ARG",
  "FDS_ERR_NULL_ARG",
  "FDS_ERR_NO_OPEN_RECORDS",
  "FDS_ERR_NO_SPACE_IN_FLASH",
  "FDS_ERR_NO_SPACE_IN_QUEUES",
  "FDS_ERR_RECORD_TOO_LARGE",
  "FDS_ERR_NOT_FOUND",
  "FDS_ERR_NO_PAGES",
  "FDS_ERR_USER_LIMIT_REACHED",
  "FDS_ERR_CRC_CHECK_FAILED",
  "FDS_ERR_BUSY",
  "FDS_ERR_INTERNAL",
};

/* Keep track of the progress of a delete_all operation. */
static struct
{
  bool delete_next;   //!< Delete next record.
  bool pending;       //!< Waiting for an fds FDS_EVT_DEL_RECORD event, to delete the next record.
} m_delete_all;

/* Data to write in flash. */
struct 
{
  char pwm_string[20];
}fds_data;

/* A record containing dummy configuration data. */
static fds_record_t const pwm_record =
{
  .file_id           = PWM_FILE,
  .key               = PWM_REC_KEY,
  .data.p_data       = &fds_data,
  /* The length of a record is always expressed in 4-byte units (words). */
  .data.length_words = (sizeof(fds_data) + 3) / sizeof(char),
};

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;
static bool volatile m_gc_complete = false;


/**
* @brief             Event handler for FDS events
* @param[in]         p_evt              Structure with FDS data
* @retval            NONE
*/
static void fds_evt_handler(fds_evt_t const * p_evt)
{
  /*NRF_LOG_INFO("Event: %s received (%s)",
               fds_evt_str[p_evt->id],
               fds_err_str[p_evt->result]);
  */
  switch (p_evt->id)
  {
  case FDS_EVT_INIT:
    if (p_evt->result == FDS_SUCCESS)
    {
      m_fds_initialized = true;
    }
    break;
    
  case FDS_EVT_WRITE:
    {
      if (p_evt->result == FDS_SUCCESS)
      {
        NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->write.record_id);
        NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->write.file_id);
        NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
      }
    } break;
    
  case FDS_EVT_DEL_RECORD:
    {
      if (p_evt->result == FDS_SUCCESS)
      {
        NRF_LOG_INFO("Record ID:\t0x%04x",  p_evt->del.record_id);
        NRF_LOG_INFO("File ID:\t0x%04x",    p_evt->del.file_id);
        NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
      }
      m_delete_all.pending = false;
    } break;
    
  case FDS_EVT_GC:
    {
      m_gc_complete = true;
    }break;
    
  default:
    break;
  }
}
/**
* @brief             Function to go to low power when waiting for FDS
* @param[in]         NONE
* @retval            NONE
*/
static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
  (void) sd_app_evt_wait();
#else
  __WFE();
#endif
}

/**
* @brief             The FDS takes some time for init. Waiting till it gets initialised properly.
* @param[in]         NONE
* @retval            NONE
*/
static void wait_for_fds_ready(void)
{
  while (!m_fds_initialized)
  {
    power_manage();
  }
}

/**
* @brief             The FDS takes some time for init. Waiting till it gets initialised properly.
* @param[in]         NONE
* @retval            NONE
*/
static void wait_for_gc_complete(void)
{
  while (!m_gc_complete)
  {
    power_manage();
  }
}

/**
* @brief             Checks if the file is present in flash memory. If present, sets the PWM accordingly, else create a new file.
* @param[in]         NONE
* @retval            NONE
*/
void check_for_file()
{
  ret_code_t rc;
  
  fds_record_desc_t desc = {0};
  fds_find_token_t  tok  = {0};
  
  rc = fds_record_find(PWM_FILE, PWM_REC_KEY, &desc, &tok);
  
  if (rc == FDS_SUCCESS)
  {
    /* A config file is in flash. Let's update it. */
    fds_flash_record_t config = {0};
    
    /* Open the record and read its contents. */
    rc = fds_record_open(&desc, &config);
    APP_ERROR_CHECK(rc);
    
    /* Read the content from the flash. */
    char pwm_string_flash[20];
    
    memcpy(&pwm_string_flash, config.p_data, sizeof(pwm_string_flash));
    
    NRF_LOG_INFO("File Found. %s",pwm_string_flash);
    
    /* Close the record when done reading. */
    rc = fds_record_close(&desc);
    APP_ERROR_CHECK(rc);
    
    /* Parse the string and set pwm accordingly. */
    parse_ble_data(pwm_string_flash);
  }
  
  else
  {
    rc = fds_record_write(&desc, &pwm_record);
    APP_ERROR_CHECK(rc);
  }
}

/**
* @brief             Updates the record in flash memory
* @param[in]         ble_data           Data received from bluetooth that has to be updated
* @retval            NONE
*/
void update_record_in_flash(char *ble_data)
{
  ret_code_t rc;
  
  fds_record_desc_t desc = {0};
  fds_find_token_t  tok  = {0};
  
  rc = fds_record_find(PWM_FILE, PWM_REC_KEY, &desc, &tok);
  
  if (rc == FDS_SUCCESS)
  {
    /* A config file is in flash. Let's update it. */
    fds_flash_record_t config = {0};
    
    /* Open the record and read its contents. */
    rc = fds_record_open(&desc, &config);
    APP_ERROR_CHECK(rc);
    
    memcpy(fds_data.pwm_string, ble_data, strlen(ble_data));
    
    /* Close the record when done reading. */
    rc = fds_record_close(&desc);
    APP_ERROR_CHECK(rc);
    
    NRF_LOG_INFO("Updating Record %s",pwm_record.data.p_data);
    /* Write the updated record to flash. */
    rc = fds_record_update(&desc, &pwm_record);
    
    if(rc == FDS_ERR_NO_SPACE_IN_FLASH)
    {
      rc = fds_gc();
      APP_ERROR_CHECK(rc);
      
      wait_for_gc_complete();
      
      rc = fds_record_update(&desc, &pwm_record);
    }
  }
}

/**
* @brief             Initialise FDS
* @param[in]         NONE
* @retval            NONE
*/
void init_fds()
{
  ret_code_t rc;
  
  /* Register first to receive an event when initialization is complete. */
  (void) fds_register(fds_evt_handler);
  
  NRF_LOG_INFO("Initializing fds...");
  
  rc = fds_init();
  APP_ERROR_CHECK(rc);
  
  /* Wait for fds to initialize. */
  wait_for_fds_ready();
  
  /* Check for configuration file. If present, set the PWM according to the saved data, else create a new file. */
  check_for_file();
}