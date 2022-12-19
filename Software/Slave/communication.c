#include "Communication.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

short SAInitialized = 0;

enum ProcessStatus { Alive, StillTerminating, Dead };
enum ProcessStatus SnapStatus = Dead;

struct ClientInformation
{
	struct mosquitto *Client;
	
	short Subscribed;
	short Connected;
	short MessageSent;
	int messageID;
	int errorcode;
}SAClient;

//pid of snapclient
pid_t scpid = -1;

/*
	The mqtt loop is a separate thread. To avoid blocking mqtt from receiving messages 
	mutexes will be used and snapclient gets started by the main thread.
	Furthermore, mqtt will lose its connection if it is timing out.
*/

//mutex for variable access
pthread_mutex_t varmutex = PTHREAD_MUTEX_INITIALIZER;

//1: turn on, 0: turn off
short turnOnSnapclient = 0;
/*
	filename: Requires the full path to the filename (config.ini as default)
*/
void SAInitializeIni(const char *filename, void (*messageHandler), void *config)
{
	//Read config from ini-File
	ini_parse(filename, messageHandler, config);
}

void cleanup()
{	
	if(SAClient.Client != NULL)
	{
		mosquitto_loop_stop(SAClient.Client, 1);

		if(SAClient.Connected == 1)
		{
			mosquitto_disconnect(SAClient.Client);
			mosquitto_lib_cleanup();
		}
		//system("pkill snapclient");
	}

	if(scpid > 0)
	{
		if (kill(scpid, 0) == 0)
			kill(scpid, SIGKILL);
	}

	exit(0);
}

void on_log(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	fprintf(stdout, "log: %s\n", str);
}

void SAInitializeMQTT(const char* address, const char* clientid, void (*messageHandler))
{	
	int rc = 0;
	if (SAInitialized == 1)
		return;

	//initialize variables
	SAClient.Subscribed = 0;
	SAClient.Connected = 0;

	//initialize mosquitto library (REQUIRED!)
	mosquitto_lib_init();
	SAClient.Client = mosquitto_new(address,0,NULL);
	
	if(SAClient.Client == NULL)
	{
		fprintf(stdout, "Error initializing mosquitto: %d\n", errno);
		exit(1);
		return;
	}

	mosquitto_log_callback_set(SAClient.Client, on_log);
	mosquitto_connect_callback_set(SAClient.Client, on_connect);
	mosquitto_message_callback_set(SAClient.Client, on_message);

	if ((rc = mosquitto_connect(SAClient.Client, address, 1883, QOS)) != MOSQ_ERR_SUCCESS)
	{
		switch(rc)
		{
			case MOSQ_ERR_INVAL:
				fprintf(stdout, "Mosquitto - Invalid input parameters\n");
				SAClient.Connected = 0;
			break;
			
			case MOSQ_ERR_ERRNO:
				fprintf(stdout, "Mosquitto - Errornumber: %d\n", rc);
				fprintf(stdout, "Errortext: %s\n", strerror(rc));
				SAClient.Connected = 0;
			break;
			
			default:
				fprintf(stdout, "Unknown error occurred: %d\n", rc);
		}
		exit(1);
		return;
	}
	else
		fprintf(stdout, "Connect was successful\n");
		
	SAClient.Connected = 1;

	if(mosquitto_loop_start(SAClient.Client) != MOSQ_ERR_SUCCESS)
	{
		fprintf(stdout, "Error while starting loop\n");
	}
	else
		fprintf(stdout, "loop started\n");
}

void CheckSnapclientStatus()
{
	int SnapOnOff = -1;

	pthread_mutex_lock(&varmutex);
	SnapOnOff = turnOnSnapclient;
	pthread_mutex_unlock(&varmutex);

	if(SnapOnOff == 1 && SnapStatus != Alive)
	{
		//fork new (child-)process and execute snapclient
		if(scpid == -1)
		{
			scpid = fork();
		
			//The newly forked process
			if(scpid == 0)
			{
				fprintf(stdout, "Process forked sucessfully\n");
				
				//redirect output stdout to /dev/null (suppress output)
				int fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);
				dup2(fd, 1);
				close(fd);

				/*
					The childprocess becomes the snapclient.
				*/
				char *args[] = {"-h volumiomaster", NULL};
				int errnum = 0;
				if((errnum = execv("/usr/bin/snapclient", args)) <0)
				{
					fprintf(stderr, "execerror %s::%d", strerror(errnum),errnum);
				}
			}
			else if(scpid < 0)
			{
				fprintf(stderr, "Error forking a process\n");
				scpid = -1;
			}
			else	//parent process!
			{
				delayMicroseconds(500000);

				if(scpid > 0)
				{
					/*
						Would be nice todo:
						ENSURE that the process is alive via IPC.
					*/

					if(kill(scpid, 0) != -1)
						SnapStatus = Alive;
					else
						fprintf(stdout, "Serious error forking the process\n");
				}
			}
		}
		else
		{
			fprintf(stdout, "Snapclient already running\n");
		}
	}
	
	if(scpid > 0 && (SnapStatus == Alive) && SnapOnOff == 0)	//returned pid of child
	{
		kill(scpid, SIGTERM);
		fprintf(stdout, "Killing snapclient\n");
		SnapStatus = StillTerminating;

		sleep(2);

		//let the snapclient exit properly
		if(waitpid(scpid, NULL, WNOHANG) == 0)
		{
			fprintf(stdout, "Snapclient has not exited yet...\n");
		}
		else
		{
			scpid = -1;
			fprintf(stdout, "Killed snapclient\n");
			SnapStatus = Dead;
		}
		
	}
	else if(SnapStatus == StillTerminating && scpid != -1)
	{
		if(kill(scpid, SIGKILL) == 0)
		{
			fprintf(stdout, "snapclient was killed forcefully\n");
			SnapStatus = Dead;
			scpid = -1;
		}
		else if(kill(scpid, 0) == 0)
		{
			fprintf(stdout, "Snapclient died in the meantime\n");
			SnapStatus = Dead;
		}
	}
}

//Event gets fired everytime a message was received or sent
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	//SAClient.MessageSent = 1;
	const char *msgstring = (const char *)message->payload;
	fprintf(stdout, "----Incoming message: %s----\n", msgstring);
	if(strcmp(msgstring, "VOn") == 0)
	{
		printf("Turning on snapclient\n");
		
		pthread_mutex_lock(&varmutex);
		turnOnSnapclient = 1;
		pthread_mutex_unlock(&varmutex);
	}
	if(strcmp(msgstring, "VOff") == 0)
	{
		fprintf(stdout, "VOff received\n");
		
		printf("Turning off snapclient\n");
		
		pthread_mutex_lock(&varmutex);
		turnOnSnapclient = 0;
		pthread_mutex_unlock(&varmutex);	
	}

	//allocated bytes for mosquitto message
	//mosquitto_message_free(message);
	return;
}


void on_disconnect(struct mosquitto *mosq, void *userData, int rc)
{
	if(rc == 0)
		fprintf(stdout, "Successful disconnection\n");
	else
		fprintf(stdout, "Unexpected disconnection\n");
		
	SAClient.Connected = 0;
}

void on_subscribe(struct mosquitto *mosq, void *userData, int messageID, int qosCount, const int *grantedQoS)
{
	fprintf(stdout, "Subscribed to \n");
}

void on_connect(struct mosquitto *mosq, void *obj, int result)
{
	
	if(result == 0)
	{
		fprintf(stdout, "Successful connection\n");
	}
	else
	{
		fprintf(stdout, "Error while connecting to broker: %d", result);
	}
}

void SASubscribeToTopic(const char *topicName)
{
	int rc = 0;

	//Connection required
	if (SAClient.Connected == 0)
	{
		fprintf(stdout, "Error while subscribing\n");
		return;
	}
	
	mosquitto_subscribe_callback_set(SAClient.Client, on_subscribe);

	if ((rc = mosquitto_subscribe(SAClient.Client, &SAClient.messageID, topicName, QOS)) != MOSQ_ERR_SUCCESS)
	{
		SAClient.Subscribed = 0;
		switch(rc)
		{
			case MOSQ_ERR_INVAL:
				fprintf(stdout, "Mosquitto - Invalid parameters\n");
			break;
			
			case MOSQ_ERR_NOMEM:
				fprintf(stdout, "Mosquitto - Out of memory condition\n");
			break;
			
			case MOSQ_ERR_NO_CONN:
				fprintf(stdout, "Mosquitto - Not connected\n");
			break;
			
			case MOSQ_ERR_MALFORMED_UTF8:
				fprintf(stdout, "Mosquitto - Topic is not valid UTF-8\n");
			break;
		}
		return;
	}
	else
	{
		fprintf(stdout, "Subscription was successful\n");
		SAClient.Subscribed = 1;
	}
}

/*
	Sends a message to the MQTT Broker if connection is existant
*/
short SASendMessage(const char* message, const char* topicName)
{
	if (SAClient.Connected != 1)
		return -1;

	int rc = 0;
	
	SAClient.MessageSent = 0;
	
	if ((rc = mosquitto_publish(SAClient.Client, &SAClient.messageID, topicName, strlen(message), message, QOS, 0)) != MOSQ_ERR_SUCCESS)
	{
		fprintf(stdout, "Failed to send message, return code %d\n", rc);
		fflush(stdout);
		return 1;
	}
	else
	{
		fprintf(stdout, "Message was sent\n");
		return 0;
	}
}

short SAIsSubscribedAndConnected()
{
	if(SAClient.Subscribed == 1 && SAClient.Connected == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
