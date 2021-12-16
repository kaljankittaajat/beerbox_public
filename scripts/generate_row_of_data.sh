#!/bin/bash
r1=$(($RANDOM %20 *100))
r2=$(($RANDOM %20 * 100))
r3=$(($RANDOM %20 * 100))
r4=$(($RANDOM %20 * 100))
t=$(($RANDOM %250 /10 - 10))

mysql -u beerbox --password=beerbox -D beerbox -e "INSERT INTO data (device_id, sensor_1, sensor_2, sensor_3, sensor_4, temp, time) VALUES (1, $r1, $r2, $r3, $r4 ,$t, now());"
