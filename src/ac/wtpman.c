/*
    This file is part of actube.

    actube is free software: you can redistribute it and/or modify
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h> 


#include "cw/capwap.h"
#include "cw/sock.h"
#include "cw/log.h"
#include "cw/timer.h"
#include "cw/cw.h"

#include "cw/dtls.h"
#include "cw/dbg.h"
#include "cw/conn.h"
#include "cw/format.h"

#include "ac.h"
#include "conf.h"
#include "db.h"
#include "socklist.h"
#include "wtpman.h"
#include "wtplist.h"
#include "cw/ktv.h"

#include "actube.h"

static void reset_echointerval_timer(struct wtpman *wtpman)
{
/*	char sock_buf[SOCK_ADDR_BUFSIZE];*/
/*	uint16_t ct = mbag_get_word(wtpman->conn->local, CW_ITEM_CAPWAP_TIMERS,
				    CW_MAX_DISCOVERY_INTERVAL << 8 |
				    CAPWAP_ECHO_INTERVAL);
*/
	/* start echinterval timer and put 2 seconds for "safety" on it */

/*
//	wtpman->echointerval_timer = cw_timer_start(2+ (ct & 0xff));
//	db_ping_wtp(sock_addr2str_p(&wtpman->conn->addr,sock_buf), conf_acname);
//	cw_dbg(DBG_X, "Starting capwap timer: %d", wtpman->echointerval_timer);
*/

}


static int msg_start_handler(struct conn *conn, struct cw_action_in *a, uint8_t * data,
			     int len, struct sockaddr *from)
{
	struct wtpman *wtpman = conn->data;
	reset_echointerval_timer(wtpman);
	return 0;
}




static void wtpman_remove(struct wtpman *wtpman)
{
	wtplist_lock();
	wtplist_remove(wtpman);
	wtplist_unlock();
	wtpman_destroy(wtpman);
}



static void wtpman_run_discovery(void *arg)
{
	struct wtpman *wtpman = (struct wtpman *) arg;

	time_t timer = cw_timer_start(10);

	wtpman->conn->capwap_state = CAPWAP_STATE_DISCOVERY;


	while (!cw_timer_timeout(timer)
	       && wtpman->conn->capwap_state == CAPWAP_STATE_DISCOVERY) {
		int rc;
		rc = cw_read_messages(wtpman->conn);
		if (cw_result_is_ok(rc)){
			wtpman->conn->capwap_state=CAPWAP_STATE_JOIN;
			
			cw_dbg(DBG_INFO,"Discovery has detected mods: %s %s", 
				wtpman->conn->cmod->name,wtpman->conn->bmod->name);

			wtplist_lock();
			discovery_cache_add(discovery_cache,(struct sockaddr*)&wtpman->conn->addr,
				wtpman->conn->cmod,wtpman->conn->bmod);
			wtplist_unlock();
			
		}
	}

	return;
}

/*
int xprocess_message(struct conn *conn, uint8_t * rawmsg, int rawlen,
		     struct sockaddr *from)
{
	uint8_t *msgptr = rawmsg + cw_get_hdr_msg_offset(rawmsg);
	uint32_t type = cw_get_msg_type(msgptr);
	cw_log(LOG_ERR, "Hey: %d", type);
	if (type == CAPWAP_MSG_DISCOVERY_REQUEST)
		conn->capwap_state = CAPWAP_STATE_DISCOVERY;


	return process_message(conn, rawmsg, rawlen, from);
}

*/


static int wtpman_dtls_setup(void *arg)
{
	char cipherstr[512];

	char sock_buf[SOCK_ADDR_BUFSIZE];
	struct wtpman *wtpman = (struct wtpman *) arg;

	/* try to accept the connection */
	if (!dtls_accept(wtpman->conn)) {
		cw_dbg(DBG_DTLS, "Error establishing DTLS session with %s",
		       sock_addr2str_p(&wtpman->conn->addr,sock_buf));
		return 0;
	}

	cw_dbg(DBG_DTLS, "DTLS session established with %s, %s",
	       sock_addr2str_p(&wtpman->conn->addr,sock_buf), dtls_get_cipher(wtpman->conn,cipherstr));

	return 1;
}

static int wtpman_join(void *arg)
{
	int rc;
	char sock_buf[SOCK_ADDR_BUFSIZE];
	struct wtpman *wtpman = (struct wtpman *) arg;
	struct conn *conn = wtpman->conn;
	time_t timer, wait_join;

	cw_dbg(DBG_INFO, "Join State - %s", sock_addr2str(&conn->addr,sock_buf));
	
	wait_join = cw_ktv_get_word(conn->global_cfg,"wait-join",CAPWAP_WAIT_JOIN);

	timer = cw_timer_start(wait_join);


	while (!cw_timer_timeout(timer) && wtpman->conn->capwap_state == CAPWAP_STATE_JOIN) {
		rc = cw_read_messages(wtpman->conn);
		if (rc < 0) {
			if (errno == EAGAIN)
				continue;
				
			break;
		}
		cw_dbg_ktv_dump(conn->remote_cfg,DBG_INFO,
		"-------------dump------------",
		"DMP","---------end dump --------");
	}

	if (rc != 0) {
		cw_log(LOG_ERR, "Error joining WTP %s", cw_strerror(rc));
		return 0;
	}


	if (wtpman->conn->capwap_state != CAPWAP_STATE_JOIN_COMPLETE) {
		cw_dbg(DBG_MSG_ERR, "No join request from %s after %d seconds, WTP died.",
		       sock_addr2str(&wtpman->conn->addr,sock_buf), wait_join);

		return 0;
	}


	return 1;


}

static void wtpman_image_data(struct wtpman *wtpman)
{
/*	char sock_buf[SOCK_ADDR_BUFSIZE];
	struct conn *conn = wtpman->conn;

	// Image upload 
	const char *filename = mbag_get_str(conn->outgoing, CW_ITEM_IMAGE_FILENAME, NULL);
	if (!filename) {
		cw_log(LOG_ERR,
		       "Can't send image to %s. No Image Filename Item found.",
		       sock_addr2str(&conn->addr,sock_buf));
		return;
	}
	cw_dbg(DBG_INFO, "Sending image file '%s' to '%s'.", filename,
	       sock_addr2str(&conn->addr,sock_buf));
	FILE *infile = fopen(filename, "rb");
	if (infile == NULL) {
		cw_log(LOG_ERR, "Can't open image %s: %s",
		       sock_addr2str(&conn->addr,sock_buf), strerror(errno));
		return;
	}



	CW_CLOCK_DEFINE(clk);
	cw_clock_start(&clk);

	mbag_item_t *eof = mbag_set_const_ptr(conn->outgoing, CW_ITEM_IMAGE_FILEHANDLE,
					      infile);

	int rc = 0;
	while (conn->capwap_state == CW_STATE_IMAGE_DATA && rc == 0 && eof != NULL) {
		rc = cw_send_request(conn, CAPWAP_MSG_IMAGE_DATA_REQUEST);
		eof = mbag_get(conn->outgoing, CW_ITEM_IMAGE_FILEHANDLE);
	}


	if (rc) {
		cw_log(LOG_ERR, "Error sending image to %s: %s",
		       sock_addr2str(&conn->addr,sock_buf), cw_strrc(rc));
	} else {
		cw_dbg(DBG_INFO, "Image '%s' sucessful sent to %s in %0.1f seconds.",
		       filename, sock_addr2str(&conn->addr,sock_buf), cw_clock_stop(&clk));
		conn->capwap_state = CW_STATE_NONE;
	}

	fclose(infile);
	wtpman_remove(wtpman);
*/

}








void * wtpman_run_data(void *wtpman_arg)
{

	return NULL;
/*
	struct wtpman *wtpman = (struct wtpman *) wtpman_arg;
	struct conn *conn = wtpman->conn;


	uint8_t data[1001];
	memset(data, 0, 1000);

	cw_log(LOG_ERR, "I am the data thread**********************************************************************\n");
	while (1) {
		sleep(5);
/ //		conn->write_data(conn, data, 100); /
		cw_log(LOG_ERR, "O was the data thread***********************************************************\n");
	}
*/

}

int cw_run_state_machine(struct conn * conn, time_t *timer)
{

	int timerval;
	cw_StateMachineState_t search, *result;

	
	while(1){
		search.state = conn->capwap_state;
		search.prevstate = conn->capwap_prevstate;
		result = mavl_get(conn->msgset->state_machine,&search);
		
		cw_dbg(DBG_STATE,"State transition: [%s -> %s]",
			cw_strstate(conn->capwap_prevstate),
			cw_strstate(conn->capwap_state)
			);
		if (result == NULL){
			cw_log(LOG_ERR,"State not found");
			return 0;
		}
		if (result->jump_state){
			conn->capwap_state = result->jump_state;
			conn->capwap_prevstate = result->jump_prevstate;
			
			cw_dbg(DBG_STATE,"Jump to state: [%s->%s]",
				cw_strstate(conn->capwap_prevstate),
				cw_strstate(conn->capwap_state));
			continue;
		}
		
		if (result->dbgmsg){
			cw_dbg(DBG_STATE,"%s",result->dbgmsg);
		}
		
		if (result->timer_key){
			timerval = cw_ktv_get_word(conn->local_cfg,result->timer_key,result->timer_default);
			*timer = cw_timer_start(timerval);
			cw_dbg(DBG_STATE,"Starting timer: [%s] - %d seconds.",result->timer_key,timerval);
		}
		return result->retval;
	}
}


/*#define CW_TRANSITION(prestate,state) (prestate<<16|state)*/

static void * wtpman_main(void *arg)
{
	mavl_t r;
	int rc ;
	time_t timer;
	char sock_buf[SOCK_ADDR_BUFSIZE];
	struct conn *conn;
	int last_state;

	struct wtpman *wtpman = (struct wtpman *) arg;

	wtpman->conn->seqnum = 0;
	conn = wtpman->conn;

	wtpman->conn->remote_cfg = cw_ktv_create(); 
	

	/* We were invoked with an unencrypted packet, 
	 * so assume, it is a discovery request */
	if (!wtpman->dtlsmode){
		wtpman_run_discovery(arg);
		wtpman_remove(wtpman);
		return NULL;
	}


	/* reject connections to our multi- or broadcast sockets */
	if (socklist[wtpman->socklistindex].type != SOCKLIST_UNICAST_SOCKET) {
		cw_dbg(DBG_DTLS,"Reject multi");
		wtpman_remove(wtpman);
		return NULL;
	}
	
	conn->capwap_state = CAPWAP_STATE_DTLS_SETUP;
	/* establish dtls session */
	if (!wtpman_dtls_setup(wtpman)) {
		wtpman_remove(wtpman);
		return NULL;
	}
	
	/*last_state = conn->capwap_state;
	conn->capwap_state = CAPWAP_STATE_JOIN;
*/
	conn->capwap_prevstate = CAPWAP_STATE_DTLS_SETUP;
	conn->capwap_state = CAPWAP_STATE_JOIN;
	rc = 0;
	
	while (1){
	
		int wait_join;
		int wait_change_state;
		
		
		
		if (!cw_run_state_machine(conn, &timer)){
			cw_dbg(DBG_INFO,"WTP died");
					wtpman_remove(wtpman);
				return NULL;		
		};
		




	/*	
		switch (conn->capwap_transition){
			case CW_TRANSITION(CAPWAP_STATE_DTLS_SETUP, CAPWAP_STATE_JOIN):
			{

				wait_join = cw_ktv_get_word(conn->global_cfg,"wait-join",CAPWAP_WAIT_JOIN);
				timer = cw_timer_start(wait_join);
				break;
			}
			case CW_TRANSITION(CAPWAP_STATE_JOIN, CAPWAP_STATE_JOIN):
			{	
				char wtpname[CAPWAP_MAX_WTP_NAME_LEN];
				cw_KTV_t * result;
				result = cw_ktv_get(conn->remote_cfg,"wtp-name",NULL);
				result->type->to_str(result,wtpname,CAPWAP_MAX_WTP_NAME_LEN);
				cw_dbg(DBG_INFO, "WTP joined: '%s', IP %s.",
					wtpname,
					sock_addr2str(&conn->addr,sock_buf)
					);
				break;
			}
			case CW_TRANSITION(CAPWAP_STATE_JOIN,CAPWAP_STATE_TIMEOUT):
			{
				cw_dbg(DBG_MSG_ERR, "No join request from %s after %d seconds, WTP died.",
				sock_addr2str(&wtpman->conn->addr,sock_buf), wait_join);
				wtpman_remove(wtpman);
				return NULL;
				break;
			}
			case CW_TRANSITION(CAPWAP_STATE_JOIN, CAPWAP_STATE_CONFIGURE):
			{
				
				wait_change_state = cw_ktv_get_word(conn->global_cfg,
					"capwap-timers/change-state-pending-timer",
					CAPWAP_TIMER_CHANGE_STATE_PENDING_TIMER);
				break;
			}
			case CW_TRANSITION(CAPWAP_STATE_CONFIGURE,CAPWAP_STATE_TIMEOUT):
			{
				cw_dbg(DBG_MSG_ERR, "No Change State Event Request %s after %d seconds, WTP died.",
				sock_addr2str(&wtpman->conn->addr,sock_buf), wait_change_state);
				wtpman_remove(wtpman);
				return NULL;
				break;
			}

		}
		
*/



		
		while (!cw_timer_timeout(timer)) {
			if (conn->update_cfg != NULL){
				mavl_t tmp;
				tmp = conn->local_cfg;

				conn->local_cfg=conn->update_cfg;
								
				cw_dbg(DBG_INFO, "Updating WTP %s",sock_addr2str(&conn->addr,sock_buf));

				rc = cw_send_request(conn, CAPWAP_MSG_CONFIGURATION_UPDATE_REQUEST);
				conn->update_cfg=NULL;
				conn->local_cfg=tmp;
			}

			
			rc = cw_read_messages(wtpman->conn);
			if (rc < 0) {
				if (errno == EAGAIN)
					continue;
			}
			break;
		}
		
		if(rc<0){
			conn->capwap_prevstate = conn->capwap_state;
			conn->capwap_state = CAPWAP_STATE_TIMEOUT;
		}

	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	




	/* dtls is established, goto join state */

	conn->capwap_state = CAPWAP_STATE_JOIN;
	if (!wtpman_join(wtpman)) {
		wtpman_remove(wtpman);
		return NULL;
	}



	cw_dbg(DBG_INFO, "WTP from %s has joined with session id: %s",
			sock_addr2str_p(&conn->addr,sock_buf),
			format_bin2hex(conn->session_id,16));


exit(0);

/*
//	cw_dbg(DBG_INFO, "Creating data thread");
//	pthread_t thread;
//	pthread_create(&thread, NULL, (void *) wtpman_run_data, (void *) wtpman);
*/

	/* here the WTP has joined, now we assume an image data request  
	   or a configuration status request. Nothing else. 
	 */

	rc = 0;
	while (!cw_timer_timeout(timer)
	       && wtpman->conn->capwap_state == CAPWAP_STATE_CONFIGURE) {
		rc = cw_read_messages(wtpman->conn);
		if (rc < 0) {
			if (errno != EAGAIN)
				break;
		}
	}

cw_dbg_ktv_dump(conn->remote_cfg,DBG_INFO,"-------------dump------------","DMP","---------end dump --------");

	if (!cw_result_is_ok(rc)) {
		cw_dbg(DBG_INFO, "WTP Problem: %s", cw_strrc(rc));
		wtpman_remove(wtpman);
		return NULL;

	}


	if (conn->capwap_state == CW_STATE_IMAGE_DATA) {
		wtpman_image_data(wtpman);
		return NULL;
	}



	conn->capwap_state = CAPWAP_STATE_RUN;
/*
	// XXX testing ...
//	DBGX("Cofig to sql", "");
//	props_to_sql(conn,conn->incomming,0);
//	radios_to_sql(conn);
*/

	/*conn->msg_end=msg_end_handler;*/
	/* The main run loop */
	reset_echointerval_timer(wtpman);

	rc = 0;
	while (wtpman->conn->capwap_state == CAPWAP_STATE_RUN) {
		rc = cw_read_messages(wtpman->conn);
		if (rc < 0) {
			if (errno != EAGAIN)
				break;
		}

/*//		cw_dbg(DBG_X, "Time left: %d",
//*/
	       /*cw_timer_timeleft(wtpman->echointerval_timer);*/
	       
		if (cw_timer_timeout(wtpman->echointerval_timer)) {

			cw_dbg(DBG_INFO, "Lost connection to WTP:%s",
			       sock_addr2str_p(&conn->addr,sock_buf));
			break;
		}
/*
//		mavl_del_all(conn->outgoing);
//		conn_clear_upd(conn,1);

//	props_to_sql(conn,conn->incomming,0);
//	radios_to_sql(conn);
*/


	
		r = db_get_update_tasks(conn, sock_addr2str(&conn->addr,sock_buf));
		if (r) {

	/*		
//			if (!conn->outgoing->count)
//				continue;
*/
			cw_dbg(DBG_INFO, "Updating WTP %s",sock_addr2str(&conn->addr,sock_buf));

			rc = cw_send_request(conn, CAPWAP_MSG_CONFIGURATION_UPDATE_REQUEST);

/*
//			mavl_merge(conn->config, conn->outgoing);
//			mavl_destroy(conn->outgoing);
//			conn->outgoing = mbag_create();
//			props_to_sql(conn,conn->incomming,0);
//			radios_to_sql(conn);
//			mavl_destroy(r);
*/
		}

		r = db_get_radio_tasks(conn, sock_addr2str(&conn->addr,sock_buf));
		if (r) {

/*
			//			if (!conn->radios_upd->count)
//				continue;
*/
			cw_dbg(DBG_INFO, "Updating Radios for %s",sock_addr2str(&conn->addr,sock_buf));
			rc = cw_send_request(conn, CAPWAP_MSG_CONFIGURATION_UPDATE_REQUEST);

/*
//			conn_clear_upd(conn,1);

//			mavl_destroy(conn->radios_upd);
//			conn->radios_upd=mbag_i_create();


//			radios_to_sql(conn);
*/
			/*
			rc = cw_send_request(conn, CW_MSG_CONFIGURATION_UPDATE_REQUEST);
			mavl_merge(conn->config, conn->outgoing);
			mavl_destroy(conn->outgoing);
			conn->outgoing = mbag_create();
			config_to_sql(conn);
			radios_to_sql(conn);
			mavl_destroy(r);
			*/
		}



	}

	db_ping_wtp(sock_addr2str_p(&conn->addr,sock_buf), "");
	wtpman_remove(wtpman);
	return NULL;
}


static void wtpman_run_dtls(void *arg)
{
	char sock_buf[SOCK_ADDR_BUFSIZE];
	struct wtpman *wtpman = (struct wtpman *) arg;



	/* reject connections to our multi- or broadcast sockets */
	if (socklist[wtpman->socklistindex].type != SOCKLIST_UNICAST_SOCKET) {
		cw_dbg(DBG_DTLS, "Dropping connection from %s to non-unicast socket.",
		       sock_addr2str_p(&wtpman->conn->addr,sock_buf));
		wtpman_remove(wtpman);
		return;
	}
/*//      time_t timer = cw_timer_start(wtpman->conn->wait_dtls);*/

	/* establish dtls session */
	if (!wtpman_dtls_setup(wtpman)) {
		wtpman_remove(wtpman);
		return;
	}

	wtpman_main(arg);
}



void wtpman_destroy(struct wtpman *wtpman)
{
	if (wtpman->conn)
		conn_destroy(wtpman->conn);
	free(wtpman);
}



struct wtpman *wtpman_create(int socklistindex, struct sockaddr *srcaddr, int dtlsmode)
{
	struct sockaddr dbgaddr;
	socklen_t dbgaddrl;
	int sockfd, replyfd;
	char sock_buf[SOCK_ADDR_BUFSIZE];
	
	struct wtpman *wtpman;
	wtpman = malloc(sizeof(struct wtpman));
	if (!wtpman)
		return 0;
	memset(wtpman, 0, sizeof(struct wtpman));

	if (socklist[socklistindex].type != SOCKLIST_UNICAST_SOCKET) {

		int port = sock_getport(&socklist[socklistindex].addr);
		replyfd = socklist_find_reply_socket(srcaddr, port);

		if (replyfd == -1) {
			cw_log(LOG_ERR, "Can't find reply socket for request from %s",
			       sock_addr2str(srcaddr,sock_buf));
			free(wtpman);
			return NULL;
		}
	} else {
		replyfd = socklist[socklistindex].sockfd;
	}

	sockfd = replyfd;	/*//socklist[socklistindex].reply_sockfd;*/
	
	
	
	
	


	dbgaddrl = sizeof(dbgaddr);
	getsockname(sockfd, &dbgaddr, &dbgaddrl);

	cw_dbg(DBG_INFO, "Creating wtpman on socket %d, %s:%d", sockfd,
	       sock_addr2str(&dbgaddr,sock_buf), sock_getport(&dbgaddr));



	wtpman->conn = conn_create(sockfd, srcaddr, 100);
	wtpman->conn->role = CW_ROLE_AC;

	wtpman->conn->data_sock = socklist[socklistindex].data_sockfd;
	sock_copyaddr(&wtpman->conn->data_addr, (struct sockaddr *) &wtpman->conn->addr);

	if (!wtpman->conn) {
		wtpman_destroy(wtpman);
		return NULL;
	}







	wtpman->conn->mods = conf_mods;

	wtpman->conn->strict_capwap = conf_strict_capwap;
	wtpman->conn->strict_hdr = conf_strict_headers;
/*
//	wtpman->conn->radios = mbag_i_create();
//	wtpman->conn->radios_upd = mbag_i_create();
//	wtpman->conn->local = ac_config;
//wtpman->conn->capwap_mode=0; //CW_MODE_STD; //CISCO;
//	wtpman->conn->capwap_mode = CW_MODE_CISCO;
//wtpman->conn->strict_capwap_hdr=0;
*/


	wtpman->conn->local_cfg = cw_ktv_create();
	wtpman->conn->global_cfg = actube_global_cfg;
	wtpman->conn->local_cfg = actube_global_cfg;

	/* when created caused by a packet in DTLS mode, we try
	 * to find out the modules to load, for detected connection 
	 * from discovery request */
	if (dtlsmode){
		int rc;
		struct cw_Mod *cmod, *bmod;
		
		rc = discovery_cache_get(discovery_cache,srcaddr,&cmod,&bmod);
		if (rc){
			cw_dbg(DBG_INFO, "Initializing with mod %s %s",cmod->name,bmod->name);
			wtpman->conn->msgset = cw_mod_get_msg_set(wtpman->conn,cmod,bmod);
			wtpman->conn->detected=1;
			cmod->setup_cfg(wtpman->conn);
		}
	}

	return wtpman;
}


void wtpman_addpacket(struct wtpman *wtpman, uint8_t * packet, int len)
{
	conn_q_add_packet(wtpman->conn, packet, len);
}

void wtpman_start(struct wtpman *wtpman, int dtlsmode)
{
	cw_dbg(DBG_INFO, "Starting wtpman, DTLS mode = %d",dtlsmode);
	wtpman->dtlsmode=dtlsmode;
	pthread_create(&wtpman->thread, NULL, wtpman_main,
		       (void *) wtpman);
	return;
}





