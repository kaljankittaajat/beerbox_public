/********************************
 ******* Beerbox backend  *******
 ********************************/
const express = require('express');
const db = require('./db');
const app = express();
const port = 8080;
const bodyParser = require("body-parser");
//const cors = require('cors'); // enable cors if needed
const nocache = require('nocache');
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://localhost:1883');

let deviceArray = [];
app.disable('x-powered-by');
//app.use(cors());
app.use(nocache());
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));

// GET
app.get('/latest/:id(\\d+)', async (req, res) => {
    try {
        const SQL = "SELECT  data.sensor_1 as sensor_1, data.sensor_2 as sensor_2, \
                        data.sensor_3 as sensor_3, data.sensor_4 as sensor_4, data.temp as temp, data.time as time \
                        FROM data INNER JOIN devices on devices.id = data.device_id \
                        WHERE data.device_id = id ORDER BY data.time DESC LIMIT 1;";
        const result = await db.pool.query(SQL, {
            id: req.params.id
        });
        res.send(result);
    } catch (err) {
        res.send('fail')
        throw err;
    }
});

app.get('/history/:id(\\d+)', async (req, res) => {
    try {
        const SQL = "SELECT data.sensor_1 as sensor_1, data.sensor_2 as sensor_2, data.sensor_3 as sensor_3, data.sensor_4 as sensor_4, \
                    data.time as time FROM data INNER JOIN devices ON devices.id = data.device_id \
                    WHERE data.device_id = ? ORDER BY data.time;";
        const result = await db.pool.query(SQL, [
            req.params.id,
        ]);
        res.send(result);
    } catch (err) {
        res.send('fail')
        throw err;
    }
});

app.get('/weight_chart/:id(\\d+)', async (req, res) => {
    try {
        const SQL = "SELECT data.sensor_1 as sensor_1, data.sensor_2 as sensor_2, \
                        data.sensor_3 as sensor_3, data.sensor_4 as sensor_4, data.temp as temp, data.time as time \
                        FROM data INNER JOIN devices on devices.id = data.device_id \
                        WHERE data.time BETWEEN current_date - 14 and now() AND data.device_id = id;";
        const result = await db.pool.query(SQL, [
            req.params.id
        ]);
        res.send(result);
    } catch (err) {
        res.send('fail')
        throw err;
    }
});

app.get('/device/:id(\\d+)', async (req, res) => {
    try {
        const SQL = "SELECT name, weight_min, weight_max, temp_min, temp_max FROM devices WHERE id = ?;";
        const result = await db.pool.query(SQL, [
            req.params.id
        ]);
        res.send(result);
    } catch (err) {
        res.send('fail');
        throw err;
    }
});

// PUT
app.put('/device/:id(\\d+)',  async (req, res) => {
    console.log('PUT: ' + req.body.name + ', ' + req.body.weight_min + ', '  + req.params.id);
    if(validatePut(req.body))
    {
        try {
            const SQL = "UPDATE devices SET name = ?, weight_min = ? WHERE id = ?;";
            const result = await db.pool.query(SQL, [
                req.body.name,
                req.body.weight_min,
                req.params.id
            ]);
            res.send(result);
        } catch (err) {
            res.send('fail')
            throw err;
        }
        if (client.connected === true) {
            client.publish("beerbox/" + req.params.id.toString() + '/alarm', weightReverse(req.body.weight_min).toString());
        }
    }
    else
    {
        res.send('fail');
    }
});


app.listen(port, () => console.log(`Listening on port ${port}`));


function writeToDatabase(data) {
    //This will take the object data collected and create an Insert Statement
    let SQL = "INSERT INTO data (device_id,sensor_1,sensor_2,sensor_3,sensor_4,temp,time) \
            VALUES(?, ?, ?, ?, ?, ?, now())"   //Connect to the database
    if(!isNaN(data.id) && !isNaN(data.weight1) && !isNaN(data.weight2) && !isNaN(data.weight3) && !isNaN(data.weight4) && !isNaN(data.temp)) {
        try {
            db.pool.query(SQL, [
                data.id,
                weight(data.weight1),
                weight(data.weight2),
                weight(data.weight3),
                weight(data.weight4),
                data.temp
            ]).then(data => console.log(data));
        } catch (err) {
            throw err;
        }
    }
}

function checkDevice(dev) {
    const query = "SELECT * FROM devices where id = ?;"
    //Connect to the database
    try {
        return db.pool.query({rowsAsArray: true, sql: query}, [
            dev.id
        ]).then(res => {
            if (res.length === 0) {
                const ins = "INSERT INTO devices(id) values(?);"
                try {
                    return db.pool.query(ins, [
                        dev.id
                    ]);
                } catch (e) {
                    console.log(e);
                    throw e;
                }
            }
            else return "device " + dev.id + " already found in database";
        });
    } catch (e) {
        console.log(e);
        throw e;
    }

}

client.on ("connect",function(){
    client.subscribe('beerbox/#');
    console.log('Connected to MQTT Server');
});

client.on ("message",function(topic,message){
    console.log("message is " + message);
    console.log("topic is " + topic);
    let topicArray = topic.split("/");
    if (topicArray.length === 3){

        let sensorID = topicArray[1];
        let reading = topicArray[2];
        let i = 1;
        let messageArray = [];

        if (reading !== 'alarm') {
            let device = deviceArray.find(device => device.id === sensorID);

            if (reading === "commit"){

                if (device !== undefined){
                    console.log(device);
                    checkDevice(device).then(res => {
                        console.log(res);
                        writeToDatabase(device);
                    });
                }

            } else {

                if (device === undefined){
                    console.log("Creating Object " + sensorID);
                    // create device object, with id, temp and 4 weight sensor values
                    device = {
                        id: sensorID,
                        temp: 0,
                        weight1: 1000,
                        weight2: 1000,
                        weight3: 1000,
                        weight4: 1000
                    }
                    if (reading === 'weight') {
                        i = 1;
                        messageArray = String(message).split(", ");
                        for (let value in messageArray) {
                            eval("device." + reading + i.toString() + "=" + messageArray[value]);
                            i++;
                        }
                    } else {
                        eval("device." + reading + "=" + message);
                    }
                    deviceArray.push(device);
                } else {
                    // if device is already defined
                    if (reading === 'weight') {
                        i = 1;
                        messageArray = String(message).split(", ");
                        for (let value in messageArray) {
                            eval("device." + reading + i.toString() + "=" + messageArray[value]);
                            i++;
                        }
                    } else {
                        eval("device." + reading + "=" + message);
                    }
                }
            }
        }
    } else {
        console.log("message is " + message);
        console.log("topic is " + topic);
    }

});

// muuntofunktiot
function weight(value) {
    if (0 <= value && value <= 2 ** 14) {
		let x = (value - 1000) / 140 * 1000;
        x *= 0.453592;
        return x.toFixed(0);
    } else {
        throw new Error('invalid weight value: ' + value.toString());
    }
}

function weightReverse(weight) {
    if (!isNaN(weight) && 1000<= weight && weight <= 2 ** 14) {
        let x = weight / 1000 / 0.453592;
        x = (x * 140) + 1000;
        return x.toFixed(0);
    }
    else {
        throw new Error('invalid weight data: ' + weight.toString());
    }
}

function validatePut(array) {
    let x = false;
    const regex = /^([a-zA-ZåäöÅÄÖ0-9-_]{3,32})$/;
    if (regex.test(array.name) && 0 <= array.weight_min && array.weight_min <= 45000) {
        x = true;
    }
    return x;
}
