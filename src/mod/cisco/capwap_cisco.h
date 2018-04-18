/*
    This file is part of libcapwap.

    libcapwap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libcapwap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

*/

/**
 * @file
 * @brief Cisco specific CAPWAP definitions 
 */

#ifndef __CAPWAP_CISCO_H
#define __CAPWAP_CISCO_H

#include <string.h>
#include <time.h>

#include "cw/lwapp.h"
#include "cw/cw.h"
#include "cw/vendors.h"

#define CISCO_ELEM_MWAR_ADDR			LWAPP_ELEM_AC_ADDRESS			/* 2 */
#define CW_CISCO_RAD				3
#define CW_CISCO_RAD_SLOT			4
#define CW_CISCO_RAD_NAME			LWAPP_ELEM_WTP_NAME			/* 5 */
#define	CW_CISCO_MWAR				LW_ELEM_AC_DESCRIPTOR			/* 6 */
#define CW_CISCO_ADD_WLAN			LW_ELEM_80211_ADD_WLAN			/* 7 */
#define CISCO_ELEM_WTP_RADIO_CONFIGURATION	8

#define CISCO_ELEM_MULTI_DOMAIN_CAPABILITY	LW_ELEM_80211_MULTI_DOMAIN_CAPABILITY	/* 10 */
#define CISCO_ELEM_MAC_OPERATION		LW_ELEM_80211_MAC_OPERATION		/* 11 */

#define CISCO_ELEM_TX_POWER			LW_ELEM_80211_TX_POWER			/* 12 */
#define CW_CISCO_TX_POWER_LEVELS		LW_ELEM_80211_TX_POWER_LEVELS		/* 13 */
#define CW_CISCO_DIRECT_SEQUENCE_CONTROL	LW_ELEM_DIRECT_SEQUENCE_CONTROL		/* 14 */
#define CW_CISCO_SUPPORTED_RATES		LW_ELEM_80211_RATE_SET			/* 16 */

#define CW_CISCO_80211_DELETE_WLAN		LW_ELEM_80211_DELETE_WLAN		/* 28 */

#define CW_CISCO_MWAR_NAME			LW_ELEM_AC_NAME				/* 31 */

#define CW_CISCO_LOCATION_DATA			LW_ELEM_LOCATION_DATA			/* 35 */
#define CW_CISCO_STATISTICS_TIMER		LW_ELEM_STATISTICS_TIMER		/* 37 */

#define CW_CISCO_ANTENNA_PAYLOAD		41

#define CW_CISCO_CERTIFICATE			LW_ELEM_CERTIFICATE			/* 44 */
#define CISCO_ELEM_WTP_BOARD_DATA		LW_ELEM_WTP_BOARD_DATA			/* 50 */
#define CW_CISCO_BCAST_SSID_MODE		LW_BCAST_SSID_MODE
#define CISCO_ELEM_AP_MODE_AND_TYPE		LW_ELEM_80211_WTP_MODE_AND_TYPE		/* 54 */
#define CISCO_ELEM_AP_QOS			LW_ELEM_QOS				/* 57 */

#define CW_CISCO_AC_IPV4_LIST			LW_ELEM_AC_IPV4_LIST			/* 59 */

#define CW_CISCO_CAPWAP_TIMERS			LW_ELEM_LWAPP_TIMERS			/* 68 */
#define CISCO_ELEM_AP_STATIC_IP_ADDR		83
#define CW_CISCO_SIG_PAYLOAD			84
#define CW_CISCO_SIG_TOGGLE			87
#define CISCO_ELEM_AIRSPACE_CAPABILITY		88

#define CW_CISCO_AC_NAME_WITH_INDEX		91
#define CW_CISCO_SPAM_DOMAIN_SECRET		96

#define CISCO_ELEM_SPAM_VENDOR_SPECIFIC		104

#define CISCO_ELEM_AP_UPTIME			108

#define CISCO_ELEM_AP_GROUP_NAME		123
#define CISCO_ELEM_AP_LED_STATE_CONFIG		125
#define CISCO_ELEM_AP_REGULATORY_DOMAIN		126
#define CISCO_ELEM_AP_MODEL			127
#define CISCO_ELEM_RESET_BUTTON_STATE		128



#define CW_CISCO_LWAPP_CHANNEL_POWER		134
#define CW_CISCO_AP_CORE_DUMP			135
#define CISCO_ELEM_AP_PRE_STD_SWITCH_CONFIG	137
#define CISCO_ELEM_AP_POWER_INJECTOR_CONFIG	138

#define CISCO_ELEM_AP_MIN_IOS_VERSION		149
#define CISCO_ELEM_AP_TIMESYNC			151
#define CW_CISCO_AP_DOMAIN			169
#define CW_CISCO_AP_DNS				170

#define CISCO_ELEM_AP_BACKUP_SOFTWARE_VERSION	183
#define CW_CISCO_BOARD_DATA_OPTIONS		207
#define CISCO_ELEM_MWAR_TYPE			208
#define CW_CISCO_80211_ASSOC_LIMIT		213
#define CW_CISCO_TLV_PAYLOAD			215
#define CISCO_ELEM_AP_LOG_FACILITY		224

#define CW_CISCO_OPER_STATE_DETAIL_CAUSE	235


#define CW_CISCO_AP_RETRANSMIT_PARAM		240

#define CW_CISCO_DOT11_CONFIG_CHECKER		242

#define CW_CISCO_AP_VENUE_SETTINGS		249
#define CISCO_ELEM_AP_LED_FLASH_CONFIG		254


/*
int cw_put_cisco_ap_timesync(uint8_t * dst, time_t time, uint8_t type);
*/

/*
int cw_out_cisco_ap_timesync(struct conn *conn,struct cw_action_out * a,uint8_t *dst);
*/


/*
int cw_addelem_cisco_ap_regulatory_domain(uint8_t *dst, struct radioinfo * ri);
*/

/**
 * Add a Cisco MWAR message element.
 * @param dst destinnation buffer
 * @param acinfo AC data
 * @return number of bytes put
 * 
 * This message elemet is basically an LWAPP AC Descriptor 
 * encapsulated in a CAPWAP vendor specific payload message
 */

/*
static inline int cw_addelem_cisco_mwar(uint8_t *dst, struct ac_info *acinfo){
	int l = lw_put_ac_descriptor(dst+10,acinfo);
	return l+cw_put_elem_vendor_hdr(dst,CW_VENDOR_ID_CISCO,CW_CISCO_MWAR,l);
}
*/

/*
int cw_readelem_cisco_station_cfg(uint8_t *src,int len);
int cw_addelem_cisco_certificate(uint8_t*dst,uint8_t*src,int len);


extern const char * cw_cisco_id_to_str(int elem_id);

int cw_readelem_cisco_wtp_radio_cfg(int elem_id,uint8_t *elem, int len,struct radioinfo *ri);
int cw_addelem_cisco_wtp_radio_cfg(uint8_t*dst,struct radioinfo * ri);
*/

/*
extern int cw_out_cisco_ac_descriptor(struct conn *conn,struct cw_action_out * a,uint8_t *dst) ;
*/

/*
int cw_in_cisco_image_identifier(struct conn *conn,struct cw_action_in * a,uint8_t *data,int len,struct sockaddr *from);

int cw_in_cipwap_wtp_descriptor(struct conn *conn, struct cw_action_in *a, uint8_t * data,
			 int len,struct sockaddr *from);
int cw_in_check_cipwap_join_req(struct conn *conn, struct cw_action_in *a, uint8_t * data,
			 int len,struct sockaddr *from);



int cw_out_cisco_wtp_radio_cfg(struct conn *conn, struct cw_action_out *a, uint8_t * dst);

int cw_in_cisco_radio_administrative_state(struct conn *conn, struct cw_action_in *a, uint8_t * data, int len,
		  struct sockaddr *from);

int cw_in_cisco_radio_administrative_state_wtp(struct conn *conn, struct cw_action_in *a, uint8_t * data, int len,
		  struct sockaddr *from);


int cw_in_cisco_radio_cfg(struct conn *conn, struct cw_action_in *a, uint8_t * data, int len,
		  struct sockaddr *from);


int cw_in_cisco_add_wlan(struct conn *conn, struct cw_action_in *a, uint8_t * data, int len,
		  struct sockaddr *from);
*/
#endif
