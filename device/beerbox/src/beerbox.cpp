/*
===============================================================================
 Name        : beerbox.cpp
 Author      : kaljankittaajat
 Version     : Public: v1.0
 Copyright   : kaljankittaajat
 Description : Main source code for BeerBox IoT Device.
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <string.h>
#include "systick.h"
#include "LpcUart.h"
#include "esp8266_socket.h"
#include "retarget_uart.h"
#include "ModbusMaster.h"
#include "ModbusRegister.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <inttypes.h>
#include "I2C.h"

/*
 * TODO: Make dynamic later from device memory?
 * Memory address for deviceId should be 0x01000100-0x0100010C.
*/
static uint8_t deviceId = 1;



// Alarm Weight values are between 1000 and 15000.
static uint16_t alarmWeight = 1617; // 1617 = 2kg

// TODO: Make a class/object for the weights to clean up the code.
// Component I2C memory addresses.
static uint8_t weight1Addr = 0x01;
static uint8_t weight2Addr = 0x02;
static uint8_t weight3Addr = 0x03;
static uint8_t weight4Addr = 0x04;
static uint8_t temperatureAddr = 0x05;

// Differences between the sensor value and actual weight.
/*
 * Should use uint16_t, but one of the sensors is malfunctioning
 * and will show negative values.
*/
static short int weight1Diff = 0;
static short int weight2Diff = 0;
static short int weight3Diff = 0;
static short int weight4Diff = 0;

/* SLEEP */
static volatile int counter;
static volatile uint32_t systicks;

#ifdef __cplusplus
extern "C" {
#endif

void SysTick_Handler(void) {
	systicks++;
	if(counter > 0) counter--;
}

uint32_t get_ticks(void) {
	return systicks;
}

#ifdef __cplusplus
}
#endif

void Sleep(int ms) {
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}

uint32_t millis() {
	return systicks;
}
/* END OF SLEEP */

// TODO: This breaks the load cell sensors...
void checkForAlarmWeight(MessageData* data) {
	char alarmChannel[64];
	sprintf(alarmChannel, "beerbox/%d/alarm", deviceId);

	if ((strncmp (alarmChannel,data->topicName->lenstring.data, data->topicName->lenstring.len) == 0)){
		((char *)data->message->payload)[data->message->payloadlen] = 0;
		sscanf((char *)data->message->payload, "%d", &alarmWeight);
	}
}

void messageArrived(MessageData* data) {
	printf("%.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
		data->message->payloadlen, (char *)data->message->payload);

	// Check if the message arrived has new min weight.
	//checkForAlarmWeight(data);
}

uint16_t readWeightRaw(I2C i2c, uint8_t addr){
	uint8_t rx[2];
	i2c.read(addr, rx, 2);
	return (rx[0] << 8 | rx[1]) & 0x3fff;
}

uint16_t readWeight(I2C i2c, uint8_t addr, short int diff){
	uint16_t tempWeight = 0;
	tempWeight = readWeightRaw(i2c, addr);
	tempWeight = (tempWeight - diff < 1000) ? 1000 : tempWeight - diff;
	return tempWeight;
}

void tareWeight(I2C i2c){
	uint8_t rx[2];
	// Loop 10 times to get rid of the garbage initial values.
	for(int i = 0; i < 10; i++){
		i2c.read(weight1Addr, rx, 2);
		i2c.read(weight2Addr, rx, 2);
		i2c.read(weight3Addr, rx, 2);
		i2c.read(weight4Addr, rx, 2);
		Sleep(100); // Short delay.
	}

	uint16_t w1 = 0;
	uint16_t w2 = 0;
	uint16_t w3 = 0;
	uint16_t w4 = 0;

	// Loop 50 times and calculate the average weight difference.
	for(int i = 0; i < 50; i++){
		w1 += readWeightRaw(i2c, weight1Addr);
		w2 += readWeightRaw(i2c, weight2Addr);
		w3 += readWeightRaw(i2c, weight3Addr);
		w4 += readWeightRaw(i2c, weight4Addr);
		Sleep(100); // Short delay.
	}
	weight1Diff = (uint16_t)((w1 / 50) - 1000);
	weight2Diff = (uint16_t)((w2 / 50) - 1000);
	weight3Diff = (uint16_t)((w3 / 50) - 1000);
	weight4Diff = (uint16_t)((w4 / 50) - 1000);
}

/* MAIN */
int main(void) {
#if defined (__USE_LPCOPEN)
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
	Board_Init();
	Board_LED_Set(0, false);
#endif
#endif
	retarget_init();
	Chip_SWM_MovablePortPinAssign(SWM_SWO_O, 1, 2);
	SysTick_Config(SystemCoreClock / 1000);
	printf("\nBoot\n");

	I2C_config cfg;
	I2C i2c(cfg);

	uint8_t temp_tx[1];
	uint8_t temp_rx[1];

	tareWeight(i2c);
	printf("Taring: %d, %d, %d, %d\n", weight1Diff, weight2Diff, weight3Diff, weight4Diff);

	uint16_t weight1 = 0;
    uint16_t weight2 = 0;
    uint16_t weight3 = 0;
    uint16_t weight4 = 0;
    uint16_t totalWeight = 0;

    char temperature[32];

    // Commit count that is iterating after every loop.
    int commitCount = 0;

	MQTTClient client;
	Network network;
	unsigned char sendbuf[256], readbuf[2556];
	int rc = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

	/* NETWORK SETTINGS */
	NetworkInit(&network,"beerbox","beerbox");
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	char* address = (char *)"127.0.0.1";
	if ((rc = NetworkConnect(&network, address, 1883)) != 0)
		printf("Return code from network connect is %d\n", rc);
	/* END OF NETWORK SETTINGS */

	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = (char *)"beerbox";

	if ((rc = MQTTConnect(&client, &connectData)) != 0)
		printf("Return code from MQTT connect is %d\n", rc);
	else
		printf("MQTT Connected\n");

	if ((rc = MQTTSubscribe(&client, "beerbox/#", QOS2, messageArrived)) != 0)
		printf("Return code from MQTT subscribe is %d\n", rc);

	uint32_t wait = 0;

	char mqtt_channel_temperature[64];
	sprintf(mqtt_channel_temperature, "beerbox/%d/%s", deviceId, "temp");

	char mqtt_channel_weight[64];
	sprintf(mqtt_channel_weight, "beerbox/%d/%s", deviceId, "weight");

	char mqtt_channel_commit[64];
	sprintf(mqtt_channel_commit, "beerbox/%d/%s", deviceId, "commit");
	i2c.read(weight1Addr, nullptr, 0);
	i2c.read(weight2Addr, nullptr, 0);
	i2c.read(weight3Addr, nullptr, 0);
	i2c.read(weight4Addr, nullptr, 0);
	/* MAIN LOOP */
	while (1) {
		if(get_ticks() / 10000 != wait) {
			// Wait for 10 seconds.
			wait = get_ticks() / 10000;

			/* TEMPERATURE */
	    	temp_tx[0] = 0x01;
	    	i2c.transaction(temperatureAddr, temp_tx, 1, temp_rx, 1);
	    	temp_tx[0] = 0x00;
	    	i2c.transaction(temperatureAddr, temp_tx, 1, temp_rx, 1);
	    	sprintf(temperature, "%d", static_cast<signed char>(temp_rx[0]));

			MQTTMessage message_temperature;
			char payload_temperature[256];

			message_temperature.qos = QOS1;
			message_temperature.retained = 0;
			message_temperature.payload = payload_temperature;
			sprintf(payload_temperature, "%s", temperature);
			message_temperature.payloadlen = strlen(payload_temperature);

			if ((rc = MQTTPublish(&client, mqtt_channel_temperature, &message_temperature)) != 0)
				printf("Return code from MQTT publish is %d\n", rc);
			/* END OF TEMPERATURE */

			/* WEIGHT */
			weight1 = readWeight(i2c, weight1Addr, weight1Diff);
			weight2 = readWeight(i2c, weight2Addr, weight2Diff);
			weight3 = readWeight(i2c, weight3Addr, weight3Diff);
			weight4 = readWeight(i2c, weight4Addr, weight4Diff);
			// Multiply sensors on the other side by 2.7, because they are broken :(
			weight2 = ((weight2 - 1000) * 2.7) + 1000;
			weight3 = ((weight3 - 1000) * 2.7) + 1000;
			totalWeight = (uint16_t)((weight1 + weight2 + weight3 + weight4) - 3000);

			MQTTMessage message_weight;
			char payload_weight[256];

			message_weight.qos = QOS1;
			message_weight.retained = 0;
			message_weight.payload = payload_weight;
			sprintf(payload_weight, "%d, %d, %d, %d", weight1, weight2, weight3, weight4);
			message_weight.payloadlen = strlen(payload_weight);

			if ((rc = MQTTPublish(&client, mqtt_channel_weight, &message_weight)) != 0)
				printf("Return code from MQTT publish is %d\n", rc);
			/* END OF WEIGHT */

			/* COMMIT */
			commitCount++;

			MQTTMessage message_commit;
			char payload_commit[256];

			message_commit.qos = QOS1;
			message_commit.retained = 0;
			message_commit.payload = payload_commit;
			sprintf(payload_commit, "%d", commitCount);
			message_commit.payloadlen = strlen(payload_commit);

			if ((rc = MQTTPublish(&client, mqtt_channel_commit, &message_commit)) != 0)
				printf("Return code from MQTT publish is %d\n", rc);
			/* END OF COMMIT */

			// Display correct Board LED if totalWeight is below alarmWeight.
			if(totalWeight <= alarmWeight){
				Board_LED_Set(0, true); //RED (bad, buy more beer)
				Board_LED_Set(1, false);
			} else {
				Board_LED_Set(1, true); //GREEN (good, drink more beer)
				Board_LED_Set(0, false);
			}
		}
		if(rc != 0) {
			NetworkDisconnect(&network);
			break;
		}
		if ((rc = MQTTYield(&client, 100)) != 0)
			printf("Return code from yield is %d\n", rc);
	}
	/* END OF MAIN */

	printf("MQTT connection closed!\n");
    return 0;
}
