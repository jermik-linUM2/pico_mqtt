
import paho.mqtt.client as mqtt
import numpy
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
from time import sleep

bucket = "test"

dbclient = InfluxDBClient(url = "http://localhost:8086",
token = "Og-tglqEWz6qmljefMQ7DM0PVU9YzN5BFhUMj6-sGz6cv-OkcpadARfHM-YJ4BURCThnpG5xp5YyHxtt6XArlw==", org = "jensen")
write_api = dbclient.write_api(write_options = SYNCHRONOUS)

def on_connect(client, userdata, flags, rc):
    print("connected with result code", rc)
    client.subscribe("pico_bme280")

def on_message(client, userdata, msg):
    #print("received message:", msg.payload.decode())
    payload = msg.payload.decode()
    values = payload.split()
    temp = float(values[0])
    press = float(values[1])
    hum = float(values[2])
    p = Point("weatherstation").field("temperature (C)", temp)
    write_api.write(bucket = bucket, record = p)
    sleep(0.1)
    p = Point("weatherstation").field("pressure (Pa)", press)
    write_api.write(bucket = bucket, record = p)
    sleep(0.1)
    p = Point("weatherstation").field("humidity (%)", hum)
    write_api.write(bucket = bucket, record = p)
    sleep(0.1)
    print(temp, type(temp))
    print(press, type(press))
    print(hum, type(hum))

client_id = "influx"
client = mqtt.Client(client_id = client_id)
client.on_connect = on_connect
client.on_message = on_message
client.connect("192.168.1.100", 1883, 60)
client.loop_forever()
