# Beerbox v1.0 - Public Repository

This is a public repository for an "IoT Project"-course project called Beerbox.
Difference between the private and public repository are edited folder structure to be more presentable to the "public" and removed cloud configuration files plus (hopefully all) other sensitive information.

- **Beerbox v1.0 - Feature List**
    - Weight data.
        - Monthly consumption.
        - Graph for consumption of last 24 hours/week/2 weeks.
    - Temperature data.
		- Current temperature.
    - Device name.
		- Editable through the web client.
    - Alerts for beer level.
		- Device displays GREEN/RED light when the beer level is GOOD/BAD.
		- Editable weight through the web client. (Not implemented in v1.0)

## What is Beerbox?
The main functionality of Beerbox is to calculate current weight and temperature, and display them on a web client.
The web client also shows some statistics of beer usage which can be seen in the feature list.
The ultimate idea for Beerbox is that it fits(and does not break) inside a fridge and predicts when the beer level is too low and orders more by itself or adds beer to a shopping list.

## Server / Web Client
The server can be ran locally or on cloud, but for our Proof of Concept we are hosting everything on a Linode Linux server.
We are using Nginx as the web server, the configuration files for it might be added here later (or not).
The web stack used is Angular, Express.js and MariaDB.
Basically, the backend is reading MQTT messages sent by the device and storing the data to the database.

## Device Parts
- MCU: LPC1549
- Wi-Fi: ESP8266
- Load Cells: FK29
- Temp Sensor: TC74

The parts need some configuring for the I2C addressing, ask your nearest Joe how to do that...