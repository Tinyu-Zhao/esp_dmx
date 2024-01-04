/**
 * @file dmx/struct.h
 * @author Mitch Weisbrod (mitch@theweisbrods.com)
 * @brief This file contains the definition for the DMX driver. This file is not
 * considered part of the API and should not be included by the user.
 */
#pragma once

#include <stdint.h>

#include "dmx/types.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "rdm/responder.h"
#include "rdm/types.h"
#include "rdm_utils.h"

#include "dmx/hal/gpio.h"
#include "dmx/hal/timer.h"
#include "dmx/hal/uart.h"

#include "esp_check.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief DMX port max. Used for error checking.*/
#define DMX_NUM_MAX SOC_UART_NUM

/** @brief Used for argument checking at the beginning of each function.*/
#define DMX_CHECK(a, err_code, format, ...) \
  ESP_RETURN_ON_FALSE(a, err_code, TAG, format, ##__VA_ARGS__)

/** @brief Logs a warning message on the terminal if the condition is not met.*/
#define DMX_ERR(format, ...)              \
  do {                                    \
    ESP_LOGE(TAG, format, ##__VA_ARGS__); \
  } while (0);

/** @brief Logs a warning message on the terminal if the condition is not met.*/
#define DMX_WARN(format, ...)             \
  do {                                    \
    ESP_LOGW(TAG, format, ##__VA_ARGS__); \
  } while (0);

/** @brief Macro used to convert milliseconds to FreeRTOS ticks. Evaluates to
 * the minimum number of ticks needed for the specified number of milliseconds
 * to elapse.*/
#define pdDMX_MS_TO_TICKS(ms)                               \
  (pdMS_TO_TICKS(ms) +                                      \
   (((TickType_t)(ms) * (TickType_t)(configTICK_RATE_HZ)) % \
        (TickType_t)1000U >                                 \
    0))

#ifdef CONFIG_RDM_DEVICE_UID_MAN_ID
/** @brief This is the RDM Manufacturer ID used with this library. It may be set
 * using the Kconfig file. The default value is 0x05e0.*/
#define RDM_UID_MANUFACTURER_ID (CONFIG_RDM_DEVICE_UID_MAN_ID)
#else
/** @brief This is the RDM Manufacturer ID that was registered with ESTA for use
 * with this software. Any device that uses this ID is associated with this
 * library. Users of this library are welcome to use this manufacturer ID (as
 * long as it is used responsibly) or may choose to register their own
 * manufacturer ID.*/
#define RDM_UID_MANUFACTURER_ID (0x05e0)
#endif

#ifdef CONFIG_RDM_DEVICE_UID_DEV_ID
/** @brief This is the RDM Device ID used with this library. It may be set
 * using the Kconfig file. The default value is a function of this device's MAC
 * address.*/
#define RDM_UID_DEVICE_ID (CONFIG_RDM_DEVICE_UID_DEV_ID)
#else
/** @brief This is the RDM Device ID used with this library. The default value
 * is a function of this device's MAC address.*/
#define RDM_UID_DEVICE_ID (0xffffffff)
#endif

#define RDM_RESPONDER_NUM_PIDS_REQUIRED 9

#ifdef CONFIG_RDM_RESPONDER_MAX_OPTIONAL_PARAMETERS
/** @brief The maximum number of optional parameters that the RDM responder can
 * support. This value is editable in the Kconfig.*/
#define RDM_RESPONDER_NUM_PIDS_OPTIONAL (CONFIG_RDM_RESPONDER_MAX_OPTIONAL_PARAMETERS)
#else
#define RDM_RESPONDER_NUM_PIDS_OPTIONAL 25
#endif

/** @brief The maximum number of parameters that the RDM responder can
 * support.*/
#define RDM_RESPONDER_PIDS_MAX (RDM_RESPONDER_NUM_PIDS_REQUIRED + RDM_RESPONDER_NUM_PIDS_OPTIONAL)

#ifdef CONFIG_RDM_RESPONDER_MAX_QUEUE_SIZE
/** @brief The maximum number of queued messages that ther RDM responder can 
 * support. It may be set using the Kconfig file.
 */
#define RDM_RESPONDER_QUEUE_SIZE_MAX CONFIG_RDM_RESPONDER_MAX_QUEUE_SIZE
#else
/** @brief The maximum number of queued messages that ther RDM responder can 
 * support.
 */
#define RDM_RESPONDER_QUEUE_SIZE_MAX 64
#endif

#if defined(CONFIG_DMX_ISR_IN_IRAM) || ESP_IDF_VERSION_MAJOR < 5
/** @brief This macro sets certain functions used within DMX interrupt handlers
 * to be placed within IRAM. The current hardware configuration of this device
 * places the DMX driver functions within IRAM. */
#define DMX_ISR_ATTR IRAM_ATTR
/** @brief This macro is used to conditionally compile certain parts of code
 * depending on whether or not the DMX driver is within IRAM.*/
#define DMX_ISR_IN_IRAM
#else
/** @brief This macro sets certain functions used within DMX interrupt handlers
 * to be placed within IRAM. Due to the current hardware configuration of this
 * device, the DMX driver is not currently placed within IRAM. */
#define DMX_ISR_ATTR
#endif

/** @brief Directs the DMX driver to use spinlocks in critical sections. This is
 * needed for devices which have multiple cores.*/
#define DMX_USE_SPINLOCK
#define DMX_SPINLOCK(n) (&dmx_driver[(n)]->spinlock)
typedef spinlock_t dmx_spinlock_t;
#define DMX_SPINLOCK_INIT portMUX_INITIALIZER_UNLOCKED

extern const char *TAG;  // The log tagline for the library.

enum dmx_flags_t {
  DMX_FLAGS_DRIVER_IS_ENABLED = BIT0,   // The driver is enabled.
  DMX_FLAGS_DRIVER_IS_IDLE = BIT1,      // The driver is not sending data.
  DMX_FLAGS_DRIVER_IS_SENDING = BIT2,   // The driver is sending.
  DMX_FLAGS_DRIVER_SENT_LAST = BIT3,    // The driver sent the last packet.
  DMX_FLAGS_DRIVER_IS_IN_BREAK = BIT4,  // The driver is in a DMX break.
  DMX_FLAGS_DRIVER_IS_IN_MAB = BIT5,    // The driver is in a DMX MAB.
  DMX_FLAGS_DRIVER_HAS_DATA = BIT6,     // The driver has an unhandled packet.
  DMX_FLAGS_DRIVER_BOOT_LOADER = BIT7,  // An error occurred with the driver.

  DMX_FLAGS_RDM_IS_VALID = BIT0,      // The RDM packet is valid.
  DMX_FLAGS_RDM_IS_REQUEST = BIT1,    // The RDM packet is a request.
  DMX_FLAGS_RDM_IS_BROADCAST = BIT2,  // The RDM packet is a broadcast.
  DMX_FLAGS_RDM_IS_RECIPIENT = BIT3,  // The RDM packet is addressed to this device.
  DMX_FLAGS_RDM_IS_DISC_UNIQUE_BRANCH = BIT4,  // The RDM packet is a DISC_UNIQUE_BRANCH.
};

typedef struct rdm_pid_info_t {
  rdm_pid_description_t desc;
  const char *param_str;
  bool is_persistent;
} rdm_pid_info_t;

/**
 * @brief Stores the DMX personality information of the DMX driver when RDM is
 * not enabled.*/
typedef struct dmx_driver_personality_t {
  uint16_t dmx_start_address;   // The driver's DMX start address.
  uint8_t current_personality;  // The current personality of the DMX driver.
  uint8_t personality_count;    // The number of personalities supported.
} dmx_driver_personality_t;

/** @brief The DMX driver object used to handle reading and writing DMX data on
 * the UART port. It storese all the information needed to run and analyze DMX
 * and RDM.*/
typedef struct dmx_driver_t {
  dmx_port_t dmx_num;  // The driver's DMX port number.

  dmx_uart_handle_t uart;    // The handle to the UART HAL.
  dmx_timer_handle_t timer;  // The handle to the hardware timer HAL.
  dmx_gpio_handle_t gpio;    // The handle to the GPIO HAL.

  // Synchronization state
  SemaphoreHandle_t mux;      // The handle to the driver mutex which allows multi-threaded driver function calls.
  TaskHandle_t task_waiting;  // The handle to a task that is waiting for data to be sent or received.

#ifdef DMX_USE_SPINLOCK
  dmx_spinlock_t spinlock;  // The spinlock used for critical sections.
#endif

  // Data buffer
  int16_t head;     // The index of the slot being transmitted or received.
  uint8_t *data;    // The buffer that stores the DMX packet.
  int16_t tx_size;  // The size of the outgoing packet.
  int16_t rx_size;  // The expected size of the incoming packet.

  // Driver state
  uint8_t flags;     // Flags which indicate the current state of the driver.
  uint8_t rdm_type;  // Flags which indicate the RDM type of the most recent packet.
  uint8_t tn;  // The current RDM transaction number. Is incremented with every RDM packet sent.
  int64_t last_slot_ts;  // The timestamp (in microseconds since boot) of the last slot of the previous data packet.

  // DMX configuration
  struct dmx_personality_t {
    uint16_t footprint;       // The DMX footprint of the personality.
    const char *description;  // A description of the personality.
  } personalities[DMX_PERSONALITY_COUNT_MAX];
  uint32_t break_len;  // Length in microseconds of the transmitted break.
  uint32_t mab_len;  // Length in microseconds of the transmitted mark-after-break.

  // Parameter data
  void *pd;        // Allocated memory for DMX/RDM parameter data.
  size_t pd_size;  // The size of the allocated memory.
  size_t pd_head;  // The amount of memory currently used for parameters.

  // RDM responder configuration
  size_t num_rdm_cbs;            // The number of RDM callbacks registered.
  struct rdm_cb_table_t {
    rdm_pid_description_t desc;  // The parameter description.
    const char *param_str;       // A parameter string describing the data.
    bool non_volatile;                    // True if the parameter is non-volatile.
    rdm_driver_cb_t driver_cb;   // The driver-side callback function.
    rdm_responder_cb_t user_cb;  // The user-side callback function.
    void *param;                 // A pointer to the parameter data.
    void *context;               // The contexted for the user-side callback.
  } rdm_cbs[RDM_RESPONDER_PIDS_MAX];  // A table containing information on RDM callbacks.

  uint16_t rdm_queue_last_sent;  // The PID of the last sent queued message.
  uint16_t rdm_queue_size;  // The index of the RDM message queue list.
  uint16_t rdm_queue[RDM_RESPONDER_QUEUE_SIZE_MAX];  // The RDM queued message list.

  // DMX sniffer configuration
  dmx_metadata_t metadata;  // The metadata received by the DMX sniffer.
  QueueHandle_t metadata_queue;  // The queue handle used to receive sniffer data.
  int64_t last_pos_edge_ts;  // Timestamp of the last positive edge on the sniffer pin.
  int64_t last_neg_edge_ts;  // Timestamp of the last negative edge on the sniffer pin.
} dmx_driver_t;

extern dmx_port_t rdm_binding_port;
extern rdm_uid_t rdm_device_uid;
extern dmx_driver_t *dmx_driver[DMX_NUM_MAX];

#ifdef __cplusplus
}
#endif
