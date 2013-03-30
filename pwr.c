/*
 * This file is part of Samsung-RIL.
 *
 * Copyright (C) 2010-2011 Joerie de Gram <j.de.gram@gmail.com>
 * Copyright (C) 2011-2012 Paul Kocialkowski <contact@paulk.fr>
 *
 * Samsung-RIL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Samsung-RIL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Samsung-RIL.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define LOG_TAG "RIL-PWR"
#include <utils/Log.h>

#include "samsung-ril.h"
#include "util.h"

/*
 * Out: RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED
 *   Modem lets us know it's powered on. Though, it's still in LPM and should
 *   be considered as OFF. Send this to update RILJ radio state (OFF)
 */
void ipc_pwr_phone_pwr_up(void)
{
	ril_data.state.radio_state = RADIO_STATE_OFF;
	ril_data.state.power_state = IPC_PWR_PHONE_STATE_LPM;

	RIL_START_UNLOCK();
}

void ipc_pwr_phone_reset(void)
{
	ril_data.state.radio_state = RADIO_STATE_OFF;
	ril_request_unsolicited(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, NULL, 0);
}

/*
 * In: IPC_PWR_PHONE_STATE
 *   Noti from the modem giving current power mode (LPM or NORMAL)
 *   LPM = Low Power Mode (airplane mode for instance)
 *
 * Out: RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED
 *   Update radio state according to modem power state
 */
void ipc_pwr_phone_state(struct ipc_message_info *info)
{
	uint8_t state = *((uint8_t *) info->data);

	switch(state)
	{
		case IPC_PWR_R(IPC_PWR_PHONE_STATE_LPM):
			LOGD("Got power to LPM");

			if (ril_data.tokens.radio_power != (RIL_Token) 0x00) {
				ril_request_complete(ril_data.tokens.radio_power, RIL_E_SUCCESS, NULL, 0);
				ril_data.tokens.radio_power = (RIL_Token) 0x00;
			}

			ril_data.state.radio_state = RADIO_STATE_OFF;
			ril_data.state.power_state = IPC_PWR_PHONE_STATE_LPM;
			ril_request_unsolicited(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, NULL, 0);
		break;
		case IPC_PWR_R(IPC_PWR_PHONE_STATE_NORMAL):
			LOGD("Got power to NORMAL");

			if (ril_data.tokens.radio_power != (RIL_Token) 0x00) {
				ril_request_complete(ril_data.tokens.radio_power, RIL_E_SUCCESS, NULL, 0);
				ril_data.tokens.radio_power = (RIL_Token) 0x00;
			}

			ril_data.state.radio_state = RADIO_STATE_SIM_NOT_READY;
			ril_data.state.power_state = IPC_PWR_PHONE_STATE_NORMAL;
			ril_request_unsolicited(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, NULL, 0);
		break;
	}

	ril_tokens_check();
}

/*
 * In: RIL_REQUEST_RADIO_POWER
 *   Request ON or OFF radio power mode
 *
 * Out: IPC_PWR_PHONE_STATE
 *   Order the modem to get in required power mode
 */
void ril_request_radio_power(RIL_Token t, void *data, size_t datalen)
{
	int power_state = *((int *)data);
	unsigned short power_data;

	LOGD("requested power_state is %d", power_state);

	if (power_state > 0) {
		LOGD("Request power to NORMAL");
		power_data = IPC_PWR_PHONE_STATE_NORMAL;

		ipc_gen_phone_res_expect_to_abort(ril_request_get_id(t), IPC_PWR_PHONE_STATE);
		ipc_fmt_send(IPC_PWR_PHONE_STATE, IPC_TYPE_EXEC, (void *) &power_data, sizeof(power_data), ril_request_get_id(t));

		ril_data.tokens.radio_power = t;
	} else {
		LOGD("Request power to LPM");
		power_data = IPC_PWR_PHONE_STATE_LPM;

		ipc_gen_phone_res_expect_to_abort(ril_request_get_id(t), IPC_PWR_PHONE_STATE);
		ipc_fmt_send(IPC_PWR_PHONE_STATE, IPC_TYPE_EXEC, (void *) &power_data, sizeof(power_data), ril_request_get_id(t));

		ril_data.tokens.radio_power = t;
	}
}
