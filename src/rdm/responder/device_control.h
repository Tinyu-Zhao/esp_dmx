/**
 * @file rdm/responder/device_control.h
 * @author Mitch Weisbrod
 * @brief // TODO
 */
#pragma once

#include "dmx/types.h"
#include "rdm/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the default response to RDM_PID_IDENTIFY_DEVICE requests.
 * This response is required by all RDM-capable devices. It is called when the
 * DMX driver is initially installed.
 *
 * @param dmx_num The DMX port number.
 * @param cb A callback which is called upon receiving a request for this PID.
 * @param[inout] context A pointer to context which is used in the user
 * callback.
 * @return true if the PID response was registered.
 * @return false if there is not enough memory to register additional responses.
 */
bool rdm_register_identify_device(dmx_port_t dmx_num, rdm_callback_t cb,
                                  void *context);

/**
 * @brief Gets a copy of the RDM identify device state of this device.
 *
 * @param dmx_num The DMX port number.
 * @param identify A pointer which stores a copy of the identify device state of
 * this device.
 * @return true on success.
 * @return false on failure.
 */ // TODO: update docs
size_t rdm_get_identify_device(dmx_port_t dmx_num, bool *identify);

/**
 * @brief Sets the RDM identify device state of this device.
 *
 * @param dmx_num The DMX port number.
 * @param identify The identify device state to which to set this device.
 * @return true on success.
 * @return false on failure.
 */
bool rdm_set_identify_device(dmx_port_t dmx_num, const bool identify);

#ifdef __cplusplus
}
#endif
