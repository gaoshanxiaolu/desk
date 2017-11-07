/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file shadow_sample.c
 * @brief A simple connected window example demonstrating the use of Thing Shadow
 */
#define __time_t_defined
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <signal.h>
#ifndef QCA4010_SUPPORT
#include <memory.h>
#include <sys/time.h>
#endif
#include <limits.h>

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_json_data.h"
#include "aws_iot_config.h"
#ifndef QCA4010_SUPPORT
#include "aws_iot_mqtt_interface.h"
#else
#include "qcom_mqtt_interface.h"
#include "qcom/base.h"
#include "qcom/basetypes.h"
#include "qcom_timer.h"
#include "qcom/qcom_system.h"
#include "qcom/qcom_sec.h"
#include "qcom/qcom_scan.h"
#include "qcom/qcom_utils.h"
#include "qcom/qcom_network.h"
#include "qcom/qcom_mem.h"
#include "qcom_time.h"
#include "swat_parse.h"
#endif

/*!
 * The goal of this sample application is to demonstrate the capabilities of shadow.
 * This device(say Connected Window) will open the window of a room based on temperature
 * It can report to the Shadow the following parameters:
 *  1. temperature of the room (double)
 *  2. status of the window (open or close)
 * It can act on commands from the cloud. In this case it will open or close the window based on the json object "windowOpen" data[open/close]
 *
 * The two variables from a device's perspective are double temperature and bool windowOpen
 * The device needs to act on only on windowOpen variable, so we will create a primitiveJson_t object with callback
 The Json Document in the cloud will be
 {
 "reported": {
 "temperature": 0,
 "windowOpen": false
 },
 "desired": {
 "windowOpen": false
 }
 }
 */
#ifdef QCA4010_SUPPORT
#define QCOM_PACK   static
#define PATH_MAX 128
#define ROOMTEMPERATURE_UPPERLIMIT 32
#define ROOMTEMPERATURE_LOWERLIMIT 25
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT
#define ThingNameMaxLength 20
void swat_aws_loop();
QCOM_PACK bool updateAccept = false; 
#else
#define QCOM_PACK
#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT
#endif

#ifdef QCA4010_SUPPORT
static void simulateRoomTemperature(int32_t *pRoomTemperature) {
    static int32_t deltaChange;
#else
static void simulateRoomTemperature(float *pRoomTemperature) {
	static float deltaChange;
#endif

	if (*pRoomTemperature >= ROOMTEMPERATURE_UPPERLIMIT) {
        #ifdef QCA4010_SUPPORT
        deltaChange = -1;
        #else
		deltaChange = -0.5f;
        #endif
	} else if (*pRoomTemperature <= ROOMTEMPERATURE_LOWERLIMIT) {
	    #ifdef AR6002_REV76
        deltaChange += 1;
        #else
		deltaChange = 0.5f;
        #endif
	}

	*pRoomTemperature += deltaChange;
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
		const char *pReceivedJsonDocument, void *pContextData) {

	if (status == SHADOW_ACK_TIMEOUT) {
		INFO("Update Timeout--");
	} else if (status == SHADOW_ACK_REJECTED) {
		INFO("Update RejectedXX");
	} else if (status == SHADOW_ACK_ACCEPTED) {
	    updateAccept = true;
		INFO("Update Accepted !!");
	}
}

void windowActuate_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	if (pContext != NULL) {
		INFO("Delta - Window state changed to %d", *(bool *)(pContext->pData));
	}
}

char certDirectory[PATH_MAX + 1] = "../../certs";
char HostAddress[255] = AWS_IOT_MQTT_HOST;
#ifdef QCA4010_SUPPORT
uint32_t awsPort = AWS_IOT_MQTT_PORT;
#else
uint32_t port = AWS_IOT_MQTT_PORT;
#endif
uint8_t numPubs = 5;

void parseInputArgsForConnectParams(int argc, char** argv) {
	int opt;

	while (-1 != (opt = getopt(argc, argv, "h:p:c:n:"))) {
		switch (opt) {
		case 'h':
			strcpy(HostAddress, optarg);
			DEBUG("Host %s", optarg);
			break;
		case 'p':
#ifdef QCA4010_SUPPORT
			awsPort = atoi(optarg);
#else
			port = atoi(optarg);
#endif
			DEBUG("arg %s", optarg);
			break;
		case 'c':
			strcpy(certDirectory, optarg);
			DEBUG("cert root directory %s", optarg);
			break;
		case 'n':
			numPubs = atoi(optarg);
			DEBUG("num pubs %s", optarg);
			break;
		case '?':
			if (optopt == 'c') {
				ERROR("Option -%c requires an argument.", optopt);
			} else if (isprint(optopt)) {
				WARN("Unknown option `-%c'.", optopt);
			} else {
				WARN("Unknown option character `\\x%x'.", optopt);
			}
			break;
		default:
			ERROR("ERROR in command line argument parsing");
			break;
		}
	}

}

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200

#ifdef QCA4010_SUPPORT
QCOM_PACK MQTTClient_t mqttClient;
QCOM_PACK char thingName[ThingNameMaxLength + 1] = AWS_IOT_MY_THING_NAME; 
QCOM_PACK char* clientCRT;
QCOM_PACK bool loopBreak = false;
int shadow_main() {
#else
int main(int argc, char** argv) {

	int32_t i = 0;
	MQTTClient_t mqttClient;
#endif

    IoT_Error_t rc = NONE_ERROR;
#ifdef QCA4010_SUPPORT
    qcom_mqtt_init(&mqttClient);
#else
	aws_iot_mqtt_init(&mqttClient);
#endif

	char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
	size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
#ifdef QCA4010_SUPPORT
    int32_t temperature = 0;
#else
    char *pJsonStringToUpdate;
	float temperature = 0.0;
#endif

	QCOM_PACK bool windowOpen = false;
	QCOM_PACK jsonStruct_t windowActuator;
	windowActuator.cb = windowActuate_Callback;
	windowActuator.pData = &windowOpen;
	windowActuator.pKey = "windowOpen";
	windowActuator.type = SHADOW_JSON_BOOL;

	QCOM_PACK jsonStruct_t temperatureHandler;
	temperatureHandler.cb = NULL;
	temperatureHandler.pKey = "temperature";
	temperatureHandler.pData = &temperature;
    temperatureHandler.type = SHADOW_JSON_INT32;
#ifndef QCA4010_SUPPORT
	temperatureHandler.type = SHADOW_JSON_FLOAT;

	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
	char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
	char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;

	parseInputArgsForConnectParams(argc, argv);
#endif

	INFO("\nAWS IoT SDK Version(dev) %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

#ifndef QCA4010_SUPPORT
	getcwd(CurrentWD, sizeof(CurrentWD));
	sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
	sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
	sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);

	DEBUG("Using rootCA %s", rootCA);
	DEBUG("Using clientCRT %s", clientCRT);
	DEBUG("Using clientKey %s", clientKey);
#endif
	ShadowParameters_t sp = ShadowParametersDefault;
	sp.pMyThingName = AWS_IOT_MY_THING_NAME;
	sp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	sp.pHost = HostAddress;

#ifdef QCA4010_SUPPORT
    sp.port = awsPort;
	sp.pClientCRT = clientCRT;
    sp.pMyThingName = thingName;
#else
	sp.port = port;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;
#endif
	INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient);

	INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &sp);

	if (NONE_ERROR != rc) {
		ERROR("Shadow Connection Error %d", rc);
	}
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = mqttClient.setAutoReconnectStatus(true);
	if (NONE_ERROR != rc) {
		ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}
    
#ifdef QCA4010_SUPPORT
    loopBreak = false;
    extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
    qcom_task_start(swat_aws_loop, 200, 2048, 10);

    qcom_thread_msleep(10);
#endif

	rc = aws_iot_shadow_register_delta(&mqttClient, &windowActuator);

	if (NONE_ERROR != rc) {
		ERROR("Shadow Register Delta Error");
	}
	
	#ifdef QCA4010_SUPPORT
	temperature = (time_ms() % 49);
	#else
	temperature = STARTING_ROOMTEMPERATURE;
	#endif

	// loop and publish a change in temperature
	while (NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc) {
        #ifndef QCA4010_SUPPORT
		rc = aws_iot_shadow_yield(&mqttClient, 200);
        #endif
		if (NETWORK_ATTEMPTING_RECONNECT == rc) {
            #ifdef QCA4010_SUPPORT
            qcom_thread_msleep(100);
            #else
            sleep(1);
            #endif
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}
        if(updateAccept)
            break;
		INFO("\n=======================================================================================\n");
		INFO("On Device: window state %s", windowOpen?"true":"false");
		simulateRoomTemperature(&temperature);

		rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
		if (rc == NONE_ERROR) {
			rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &temperatureHandler,
					&windowActuator);
			if (rc == NONE_ERROR) {
				rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
				if (rc == NONE_ERROR) {
					INFO("Update Shadow: %s", JsonDocumentBuffer);
                    #ifdef QCA4010_SUPPORT
                    rc = aws_iot_shadow_update(&mqttClient, thingName, JsonDocumentBuffer,
							ShadowUpdateStatusCallback, NULL, 4, true);
                    #else
					rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
							ShadowUpdateStatusCallback, NULL, 4, true);
                    #endif
				}
			}
		}
		INFO("*****************************************************************************************\n");
        #ifdef QCA4010_SUPPORT
        qcom_thread_msleep(100);
        #else
		sleep(1);
        #endif
	}

	if (NONE_ERROR != rc) {
		ERROR("An error occurred in the loop %d", rc);
	}

	INFO("Disconnecting");
	rc = aws_iot_shadow_disconnect(&mqttClient);
    updateAccept = false;

	if (NONE_ERROR != rc) {
		ERROR("Disconnect error %d", rc);
	}
	else
    {
        loopBreak = true;
    }

	return rc;
}

#ifdef QCA4010_SUPPORT
void swat_aws_loop()
{
    IoT_Error_t rc = NONE_ERROR;

    // loop and publish a change in temperature
	while (NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc) {
        if (loopBreak)
            break;
		rc = aws_iot_shadow_yield(&mqttClient, 200);

		if (NETWORK_ATTEMPTING_RECONNECT == rc) {
            qcom_thread_msleep(100);
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}
        qcom_thread_msleep(10);
	}

    INFO("mqtt task exit %d.\n", rc);
    qcom_task_exit();
}

int swat_aws_usage(const char* str)
{
    const char* strHelpShadow = \
                                     "wmiconfig --aws shadow <URL> <port> <thing name> <CRT>\r\n"
                                     "Description: start aws shadow sample \r\n\n";

    if (!str || !A_STRCMP(str, "shadow") )
    {
        printf(strHelpShadow);
    }
    return SWAT_ERROR;
}

int swat_wmiconfig_aws_handle(A_INT32 argc, A_CHAR * argv[])
{
    if (!swat_strcmp(argv[1], "--aws"))
    {
        if (argc < 7)
        {
            return swat_aws_usage(NULL);
        }
        if (!swat_strcmp(argv[2], "shadow"))
        {
            strcpy(HostAddress, argv[3]);
            awsPort = atoi(argv[4]);
			strcpy(thingName, argv[5]);
            clientCRT = argv[6];
			//strcpy(clientCRT, argv[6]);
            INFO("%s  %s  %s",HostAddress, clientCRT, thingName);
            shadow_main();
            return 1;
        }
        else if(!swat_strcmp(argv[2], "shadow_echo"))
        {
            //new demo 
            return SWAT_OK;
        }
    }

    return SWAT_NOFOUND;
}
#endif
