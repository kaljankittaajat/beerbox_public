CREATE DATABASE IF NOT EXISTS beerbox;
USE beerbox;
CREATE OR REPLACE TABLE devices (
	id int not null auto_increment primary key,
	name varchar(255),
	mac int not null unique key,
	weight_min int default 1000,
	weight_max int default 45000,
	temp_min dec(4,1) default 0,
	temp_max dec(4,1) default 10
	
);
CREATE OR REPLACE TABLE data (
	device_id int,
	sensor_1 int,
	sensor_2 int,
	sensor_3 int,
	sensor_4 int,
	temp dec(4,1),
	time timestamp,
	CONSTRAINT `fk_device_id` FOREIGN KEY (device_id) REFERENCES devices (id) ON DELETE CASCADE ON UPDATE CASCADE
);	
