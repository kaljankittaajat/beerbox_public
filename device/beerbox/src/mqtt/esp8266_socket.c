/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ring_buffer.h"

#include "esp8266_socket.h"

#include "serial_port.h"

typedef int EspSocket_t;

uint32_t get_ticks(void); // defined externally


#define I_DONT_USE(x) (void) x
#define DEBUGP( ... ) printf( __VA_ARGS__ )

// macro for changing state. Correct operation requires that ctx is a pointer to state machine instance (see state template)
#define TRAN(st) ctx->next_state = st

typedef enum eventTypes { eEnter, eExit, eTick, eReceive, eConnect, eDisconnect, eSend } EventType;

typedef struct event_ {
	EventType ev; // event type (= what happened)
	// we could add additional data
} event;

static const event evEnter = { eEnter };
static const event evExit = { eExit };


typedef struct smi_ smi;

typedef void (*smf)(smi *, const event *);  // prototype of state handler function pointer

#define EVQ_SIZE 32

#define SMI_BUFSIZE 80
#define RC_NOT_AVAILABLE  -1
#define RC_OK     0
#define RC_ERROR  1

struct smi_ {
	smf state;  // current state (function pointer)
	smf next_state; // next state (function pointer)
	RINGBUFF_T EspEventQ;
	event evq_buf[EVQ_SIZE];
    int timer;
    int count;
    int pos;
    char buffer[SMI_BUFSIZE];
    char ssid[32]; // SSID
    char pwd[32]; // password
    char sa_data[32]; // ip address
    char sa_port[14]; // port number (string)
};

static void stInit(smi *ctx, const event *e);
static void stEchoOff(smi *ctx, const event *e);
static void stStationModeCheck(smi *ctx, const event *e);
static void stStationModeSet(smi *ctx, const event *e);
static void stConnectAP(smi *ctx, const event *e);
static void stReady(smi *ctx, const event *e);
static void stConnectTCP(smi *ctx, const event *e);
static void stConnected(smi *ctx, const event *e);
static void stCloseTCP(smi *ctx, const event *e);
static void stPassthrough(smi *ctx, const event *e);
static void stPassthroughOK(smi *ctx, const event *e);
static void stAT(smi *ctx, const event *e);
static void stCommandMode(smi *ctx, const event *e);

static void EspSocketRun(smi *ctx);



smi EspSocketInstance;

static void port2str(int i, char *str)
{
    int m = 100000;
    i %= m; // limit integer size to max 5 digits.
    while(i / m == 0) m/=10;
    while(m > 0) {
        *str++ = '0' + i / m;
        i %= m;
        m /= 10;
    }
    *str='\0';
}

void smi_init(smi *ctx) 
{
    serial_init(ctx);
    memset(ctx, 0, sizeof(smi));
    ctx->state = stInit;
    ctx->next_state = stInit;
    RingBuffer_Init(&ctx->EspEventQ, ctx->evq_buf, sizeof(event), EVQ_SIZE);

    ctx->state(ctx, &evEnter); // enter initial state
}

    
int esp_socket(const char *ssid, const char *password)
{
    smi_init(&EspSocketInstance);
    
    
    strncpy(EspSocketInstance.ssid, ssid, 32);
    strncpy(EspSocketInstance.pwd, password, 32);
    
    
    while(EspSocketInstance.state != stReady) {
    	// run esp task and run ticks
    	EspSocketRun(&EspSocketInstance);
    }
   
    return 0;    
}



int esp_connect(int sockfd, const char *addr, int port)
{
    I_DONT_USE(sockfd);

    strncpy(EspSocketInstance.sa_data,addr,sizeof(EspSocketInstance.sa_data)-1);
    EspSocketInstance.sa_data[sizeof(EspSocketInstance.sa_data)-1] = '\0';
    port2str(port, EspSocketInstance.sa_port);

    const event e = { eConnect };

    RingBuffer_Insert(&EspSocketInstance.EspEventQ,&e);

    int rc = 0;

    while(EspSocketInstance.state != stConnected) {
    	// run esp task and run ticks
    	EspSocketRun(&EspSocketInstance);
    }

    return rc;
}

int esp_read(int sockfd, void *data, int length) 
{
    I_DONT_USE(sockfd);
    int count = 0;
    
    if(EspSocketInstance.state == stConnected) {
        // read data
        while(count < length && serial_get_char(&EspSocketInstance, data)) {
            ++count;
            ++data;
        }
    }
    
    return count;
}

int esp_write(int sockfd, const void *data, int length)
{
    I_DONT_USE(sockfd);

    if(EspSocketInstance.state == stConnected) {
        // write data
        serial_write_buf(&EspSocketInstance, data, length);
    }
    
    return length;
}

int esp_close(int sockfd)
{
    return esp_shutdown(sockfd, -1);
}

int esp_shutdown(int sockfd, int how)
{
    I_DONT_USE(sockfd);
    I_DONT_USE(how);
    const event e = { eDisconnect };
    
    RingBuffer_Insert(&EspSocketInstance.EspEventQ,&e);
    while(EspSocketInstance.state != stReady) {
    	// run esp task and run ticks
    	EspSocketRun(&EspSocketInstance);
    }

    return 0;
}



#if 0
/* this is state template */
void stStateTemplate(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
		break;
	case eExit:
		break;
	case eTick:
		break;
	case eReceive:
		break;
	default:
		break;
	}
}
#endif

void init_counters(smi *ctx) {
    ctx->count = 0;
    ctx->pos = 0;
    ctx->timer = 0;
}

/* Read and store characters upto specified length. 
 * Returns true when specified amount of characters have been accumulated. */
void sm_flush(smi *ctx) 
{
    //DEBUGP("flush: %d\n", (int)xSerialRxWaiting(ctx->ComPort));
    while(serial_get_char(ctx, ctx->buffer));
}


/* Read and store characters upto specified length. 
 * Returns true when specified amount of characters have been accumulated. */
bool sm_read_buffer(smi *ctx, int count) 
{
    while(ctx->pos < (SMI_BUFSIZE - 1) && ctx->pos < count && serial_get_char(ctx, ctx->buffer + ctx->pos)) {
        //putchar(ctx->buffer[ctx->pos]); // debugging
        ++ctx->pos;
    }
    return (ctx->pos >= count);
}


/* Read an integer.
 * Consumes characters until a non-nondigit is received. The nondigit is also consumed. */
bool sm_read_int(smi *ctx, int *value) 
{
    bool result = false;
    while(ctx->pos < (SMI_BUFSIZE - 1) && serial_get_char(ctx, ctx->buffer + ctx->pos)) {
        if(!isdigit((int)ctx->buffer[ctx->pos])) {
            ctx->buffer[ctx->pos] = '\0';
            *value = atoi(ctx->buffer);
            result = true;
            break;
        }
        else {
            ++ctx->pos;
        }
    }
    return result;
}



/* Read and store data until one of the specified strings is received.
 * The matched string is also included in the data .*/
int sm_read_until(smi *ctx, const char **p) 
{
    int result = RC_NOT_AVAILABLE;
    while(result < 0 && ctx->pos < (SMI_BUFSIZE - 1) && serial_get_char(ctx, ctx->buffer + ctx->pos)) {
        ++ctx->pos;
        ctx->buffer[ctx->pos] = '\0';
        for(int i = 0; result < 0 && p[i] != NULL; ++i) {
            if(strstr(ctx->buffer, p[i]) != NULL) {
                result = i;
            }
        }
    }
    return result;
    
}

/* read and store data until result is received */
int sm_read_result(smi *ctx) 
{
    static const char *result_list[] = { "OK\r\n", "ERROR\r\n", NULL };
    return sm_read_until(ctx, result_list);
}

/* read and consume characters until specified string occurs */
bool sm_wait_for(smi *ctx, const char *p)
{
    bool result = false;
    int len = strlen(p);

    while(sm_read_buffer(ctx, len)) {
        ctx->buffer[ctx->pos] = '\0';
        if(strstr(ctx->buffer, p) != NULL) {
            result = true;
            break;
        }
        else {
            memmove(ctx->buffer, ctx->buffer + 1, ctx->pos);
            --ctx->pos;
        }
    }
    return result;
}

static void stInit(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stInit\r\n");
        sm_flush(ctx);
        init_counters(ctx);
        serial_write_str(ctx, "AT\r\n");
		break;
	case eExit:
		break;
	case eTick:
        ++ctx->timer;
        //if(ctx->timer == 5) DEBUGP("[%s]", ctx->buffer);
        if(ctx->timer >= 10) {
            ctx->timer = 0;
            ++ctx->count;
            if(ctx->count < 5) {
                serial_write_str(ctx, "AT\r\n");
            }
            else {
                DEBUGP("Error: Module not responding\r\n");
                TRAN(stAT);
            }
        }
		break;
    case eReceive:
        if(sm_wait_for(ctx, "OK\r\n")) {
            TRAN(stEchoOff);
        }
        break;
	default:
		break;
	}
}

static void stAT(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stAT\r\n");
        init_counters(ctx);
		break;
	case eExit:
		break;
	case eTick:
        ++ctx->timer;
        if(ctx->timer == 10) serial_write_str(ctx, "+++");
        if(ctx->timer == 25) TRAN(stInit);
		break;
    case eReceive:
        break;
	default:
		break;
	}
}

static void stEchoOff(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stEchoOff\r\n");
        sm_flush(ctx);
        init_counters(ctx);
        serial_write_str(ctx, "ATE0\r\n");
		break;
	case eExit:
		break;
	case eTick:
        ++ctx->timer;
        if(ctx->timer >= 10) {
            ++ctx->count;
            if(ctx->count < 3) {
                serial_write_str(ctx, "ATE0\r\n");
            }
            else {
                DEBUGP("Error: setting local echo off failed\r\n");
                TRAN(stInit);
            }
        }
		break;
    case eReceive:
        if(sm_wait_for(ctx, "OK\r\n")) {
            TRAN(stStationModeCheck);
        }
        break;
	default:
		break;
	}
}


static void stStationModeCheck(smi *ctx, const event *e)
{
    int rc = -1;
	switch(e->ev) {
	case eEnter:
        DEBUGP("stStationModeCheck\r\n");
        init_counters(ctx);
        serial_write_str(ctx, "AT+CWMODE_CUR?\r\n");
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        rc = sm_read_result(ctx);
        if(rc == RC_OK) {
            //DEBUGP("%d: %s", rc, ctx->buffer);
            if(strstr(ctx->buffer, "+CWMODE_CUR:1\r\n") != NULL) {
                TRAN(stConnectAP);
            }
            else {
                TRAN(stStationModeSet);
            }
        }
        else if(rc == RC_ERROR) {
        }
        break;
	default:
		break;
	}
}


static void stStationModeSet(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stStationModeSet\r\n");
        init_counters(ctx);
        serial_write_str(ctx, "AT+CWMODE_CUR=1\r\n");
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        if(sm_wait_for(ctx, "OK\r\n")) {
            TRAN(stStationModeCheck);
        }
        break;
	default:
		break;
	}
}

static void connect_ssid(smi *ctx) 
{
    serial_write_str(ctx, "AT+CWJAP_CUR=\"");
    serial_write_str(ctx, ctx->ssid);
    serial_write_str(ctx, "\",\"");
    serial_write_str(ctx, ctx->pwd);
    serial_write_str(ctx, "\"\r\n");
}

static void stConnectAP(smi *ctx, const event *e)
{
    int rc;
	switch(e->ev) {
	case eEnter:
        DEBUGP("stConnectAP\r\n");
        init_counters(ctx);
		break;
	case eExit:
		break;
	case eTick:
        // may take upto 7 seconds. do we need a timeout?
        ++ctx->timer;
        if(ctx->timer == 1) {
            connect_ssid(ctx);
        }
        if(ctx->timer >= 70) {
            ctx->timer = 0;
        }
		break;
    case eReceive:
        rc = sm_read_result(ctx);
        if(rc == RC_OK) {
            //DEBUGP("%d: %s", rc, ctx->buffer);
            TRAN(stReady);
        }
        else if(rc == RC_ERROR) {
            // failed: what to do now?
        }
        break;
	default:
		break;
	}
}


static void stReady(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stReady\r\n");
        init_counters(ctx);
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        break;
    case eConnect:
        TRAN(stConnectTCP);
        break;
	default:
		break;
	}
    
}



static void connect_tcp(smi *ctx) 
{
    serial_write_str(ctx, "AT+CIPSTART=\"TCP\",\"");
    serial_write_str(ctx, ctx->sa_data);
    serial_write_str(ctx, "\",");
    serial_write_str(ctx, ctx->sa_port);
    serial_write_str(ctx, "\r\n");
}

static void stConnectTCP(smi *ctx, const event *e)
{
    int rc;
	switch(e->ev) {
	case eEnter:
        DEBUGP("stConnectTCP\r\n");
        init_counters(ctx);
        connect_tcp(ctx);
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        rc = sm_read_result(ctx);
        if(rc == RC_OK) {
            //DEBUGP("%d: %s", rc, ctx->buffer);
            if(strstr(ctx->buffer, "CONNECT\r\n") != NULL) {
                TRAN(stPassthrough);
            }
            else {
                // what else can we get with OK??
            }
        }
        else if(rc == RC_ERROR) {
            // failed: what to do now?
            DEBUGP("Connect failed\r\n");
            connect_tcp(ctx);
        }
        break;
	default:
		break;
	}
}


static void stPassthrough(smi *ctx, const event *e)
{
    int rc;
	switch(e->ev) {
	case eEnter:
        DEBUGP("stPassthrough\r\n");
        init_counters(ctx);
        serial_write_str(ctx, "AT+CIPMODE=1\r\n");
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        rc = sm_read_result(ctx);
        if(rc == RC_OK) {
            TRAN(stPassthroughOK);
        }
        else if(rc == RC_ERROR) {
            // failed: what to do now?
        }
        break;
	default:
		break;
	}
    
}

static void stPassthroughOK(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stPassthroughOK\r\n");
        init_counters(ctx);
        serial_write_str(ctx, "AT+CIPSEND\r\n");
		break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        if(sm_wait_for(ctx, ">")) {
            TRAN(stConnected);
        }
        break;
	default:
		break;
	}
    
}


static void stConnected(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stConnected\r\n");
        init_counters(ctx);
        break;
	case eExit:
		break;
	case eTick:
		break;
    case eReceive:
        break;
    case eDisconnect:
        TRAN(stCommandMode);
        break;
	default:
		break;
	}
    
}


static void stCommandMode(smi *ctx, const event *e)
{
	switch(e->ev) {
	case eEnter:
        DEBUGP("stCommandMode\r\n");
        init_counters(ctx);
		break;
	case eExit:
		break;
	case eTick:
        ++ctx->timer;
        if(ctx->timer == 10) serial_write_str(ctx, "+++");
        if(ctx->timer == 25) TRAN(stCloseTCP);
		break;
    case eReceive:
        break;
	default:
		break;
	}
}


static void stCloseTCP(smi *ctx, const event *e)
{
    int rc = -1;
	switch(e->ev) {
	case eEnter:
        DEBUGP("stCloseTCP\r\n");
        init_counters(ctx);
        serial_write_str(ctx, "AT+CIPMODE=0\r\n");;
        break;
	case eReceive:
        rc = sm_read_result(ctx);
        if(rc == RC_OK) {
            if(strstr(ctx->buffer, "CLOSED") != NULL) {
                TRAN(stReady);
            }
            else {
                init_counters(ctx);
                serial_write_str(ctx, "AT+CIPCLOSE\r\n");;
            }
        }
        else if(rc == RC_ERROR) {
        }
		break;
	case eExit:
		break;
	case eTick:
		break;
	default:
		break;
	}
}



static void dispatch_event(smi *ctx, const event *e)
{
	ctx->state(ctx, e); // dispatch event to current state
	if(ctx->state != ctx->next_state) { // check if state was changed
		ctx->state(ctx, &evExit); // exit old state (cleanup)
		ctx->state = ctx->next_state; // change state
		ctx->state(ctx, &evEnter); // enter new state
	}
}



/**
 * Receive events from queue and dispatch them to state machine
 */
static void EspSocketRun(smi *ctx)
{
	event e;

	static uint32_t old = 0 ;
    uint32_t now = 0 ;

	now = get_ticks()/100;
	if(now != old) {
	    const event tick = { eTick };
		old = now;
		// send ESP tick
		RingBuffer_Insert(&ctx->EspEventQ, &tick);
	}

	if(serial_peek(ctx)) {
	    const event rcv = { eReceive };
		RingBuffer_Insert(&ctx->EspEventQ, &rcv);
	}

	// read queue
	while (RingBuffer_Pop(&ctx->EspEventQ,&e)) {
		dispatch_event(ctx, &e); // dispatch event to current state
	}
}



/* [] END OF FILE */
