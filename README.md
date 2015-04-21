##This a simple MQTT Greenhouse Sample (PoC).

This Open AT sample simulates temperature and humidity and sendss data to AirVantage M2M Cloud.

####In a nutshell:
* It provides an high level MQTT API based on Eclipse Paho Library(http://www.eclipse.org/paho/) 
* The data is serialized in JSON according to the doc here: https://doc.airvantage.net/display/USERGUIDE/Using+MQTT+with+AirVantage

#### Getting Started
1. Create an Open AT Project
2. Change the APN in src/GPRS/cfg_gprs.c (top of the file)
3. Change your MQTT password in src/entry_point.c (bottom of the file)
4. Compile and download on your target
5. You can modify the temperature with the at command at+temperature=<value>
6. Register your system on AirVantage (Follow the three first section from [the MQTT tutorial] (https://doc.airvantage.net/display/USERGUIDE/Using+MQTT+with+AirVantage))
