import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import paho.mqtt.client as client
import configparser
import os
import sys
#create an empty dictionary
#this dict contains the sensorid and number of persons in room
sensors = {}

SensorsToRoom ={}
RoomToStream = {}
PersonsInRoom = {}

config = configparser.ConfigParser()
curwd = os.getcwd() + "/config.ini"
print(curwd)

config.read(curwd)

for key in config['SensorToRoom']:
    #SensorsToRoom[s1]['Room1'] = R1
    #SensorsToRoom[s1]['Room2'] = R3
    #SensorsToRoom[s1]['MQTT'] =  'smartaudio/s1'
    
    #SensorsToRoom[smartaudio/s1][Room1] = r1
    #SensorsToRoom[smartaudio/s1][Room2] = r3
    SensorsToRoom[str(key)] = {}
    value = config["SensorToRoom"][str(key)].split(",")
    SensorsToRoom[key]["Room1"] = str(value[0])
    SensorsToRoom[key]["Room2"] = str(value[1])

for key in config['RoomToStream']:
    #RoomToStream[r1] = smartaudio/s1
    #PersonsInRoom[r1] = 0
    RoomToStream[str(key)] = {}
    RoomToStream[str(key)] = config['RoomToStream'][key]
    
    PersonsInRoom[str(key)] = 0

#    message = 'in'
#PersonsInRoom[SensorsToRoom[message.topic]['Room1']] += 1
#PersonsInRoom[SensorsToRoom[message.topic]['Room2']] -= 1
    
    
MQTT_SERVER = "localhost"

mainchannel = "smartaudio"

#subscribe to all subchannels
channel = mainchannel + "/#"

#name of master
clid = "master"

def on_connect(client, userdata, flags, rc):
    print("Registering to " + str(channel))

    #subscription or else no data
    client.subscribe(channel,2)

def on_message(cl, userdata, message):
    msg = str(message.payload.decode('utf-8', 'ignore'))

    #check if sensor is already registered:
    topic = message.topic

    #debugging
    if msg == 'ping':
	client.publish(topic,"pong", qos=2)

    #somebody entered the room
    if msg == 'in':
        print("Received in")
        PersonsInRoom[SensorsToRoom[topic]['Room1']] += 1

        #If the first person enters the room, turn volume on
        if PersonsInRoom[SensorsToRoom[topic]['Room1']] == 1:
            print("Sending on")
            #send volume on here
            client.publish(RoomToStream[SensorsToRoom[topic]['Room1']], "VOn", qos=2)
            print("Sending VOn to " + str(RoomToStream[SensorsToRoom[topic]['Room1']]))
        
        #Decrease only, if the amount of person is bigger than 0
        if PersonsInRoom[SensorsToRoom[topic]['Room2']] > 0:
            PersonsInRoom[SensorsToRoom[topic]['Room2']] -= 1
            #Turn volume off
            if PersonsInRoom[SensorsToRoom[topic]['Room2']] == 0:
                client.publish(RoomToStream[SensorsToRoom[topic]['Room2']], "VOff", qos=2)
                print("Sending VOff to " + str(RoomToStream[SensorsToRoom[topic]['Room2']]))        
        
    #somebody leaves the room
    elif msg == 'out':
        print("Received out")
        PersonsInRoom[SensorsToRoom[topic]['Room2']] += 1

        #if the first person enters the room, turn volume on
        if PersonsInRoom[SensorsToRoom[topic]['Room2']] == 1:
            #send volume on here
            client.publish(RoomToStream[SensorsToRoom[topic]['Room2']], "VOn", qos=2)
            print("Sending VOn to " + str(RoomToStream[SensorsToRoom[topic]['Room2']]))
        
        if PersonsInRoom[SensorsToRoom[topic]['Room1']] > 0:
            PersonsInRoom[SensorsToRoom[topic]['Room1']] -= 1

            if PersonsInRoom[SensorsToRoom[topic]['Room1']] == 0:
                client.publish(RoomToStream[SensorsToRoom[topic]['Room1']], "VOff", qos=2)
                print("Sending VOff to " + str(RoomToStream[SensorsToRoom[topic]['Room1']]))

    else:
        pass #unknown command or message from server...

client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_SERVER, 1883, 60)
client.loop_forever()
