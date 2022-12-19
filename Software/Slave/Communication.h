#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <mosquitto.h>
#include "ini.h"
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define QOS         2
#define TIMEOUT     3000L

#define MATCH(s,n) strcmp(section,s) == 0 && strcmp(name, n) == 0

void SAInitializeIni(const char *filename, void (*messageHandler), void *config);
void SAInitializeMQTT(const char* address, const char* clientid, void (*messageHandler));
void CheckSnapclientStatus();
short SASendMessage(const char* message, const char* topicName);
void SASubscribeToTopic(const char *topicName);
void on_connectionlost(void *context, char *cause);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void on_disconnect(struct mosquitto *mosq, void *userData, int rc);
void on_subscribe(struct mosquitto *mosq, void *userData, int messageID, int qosCount, const int *grantedQoS);
void on_connect(struct mosquitto *mosq, void *obj, int result);
void cleanup();

short CreateWaitingThread(short int *conditionVariable, const char* waitingReason);
short SAIsSubscribedAndConnected();
