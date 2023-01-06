"use strict"
var oldState;  // previous user action is saved here
var ws = null; // variable for web socket
var maxSpeed = 150;
var robot = {
    motors: { // motors state
        dT: 0,
        lS: 0,
        rS: 0
    },
    camera: { // camera servos state
        dT: 1,
        h: 0,
        v: 0,
        lt:0
    },
    lights: { // onboard lights state
        dT: 2,
        lts: 255,
    },
    hand: {
        dT: 3,
        b: 0,
        e: 0,
        w: 0,
        c: 0
    },
    goForward: function () {
        this.motors.lS = maxSpeed;
        this.motors.rS = maxSpeed;
        var dataToSend = JSON.stringify(robot.motors);
        console.log(dataToSend);
        ws.send(dataToSend);
    },
    goBack: function () {
        this.motors.lS = -maxSpeed;
        this.motors.rS = -maxSpeed;
        var dataToSend = JSON.stringify(robot.motors);
        console.log(dataToSend);
        ws.send(dataToSend);
    },
     
    stop: function () {
        this.motors.lS = 0;
        this.motors.rS = 0;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnRight: function () {
        this.motors.lS = maxSpeed;
        this.motors.rS = -maxSpeed;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnLeft: function () {
        this.motors.lS = -maxSpeed;
        this.motors.rS = maxSpeed;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    setLights: function () {
        var dataToSend = JSON.stringify(robot.lights);
        console.log(dataToSend);
        ws.send(dataToSend);
    }
};

OpenWebsocket();
addEventListener("keydown", function (event) { // For going forward
    if (event.keyCode === 38 && event.keyCode !== oldState) { //if this key is pressed for the first time
        oldState = event.keyCode; // this key was pressed
        console.log('Pressed: UP');
        robot.goForward();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 40 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: DOWN');
        robot.goBack();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 37 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: LEFT');
        robot.turnLeft();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 39 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: RIGHT');
        robot.turnRight();
    }
});

addEventListener("keyup", function (event) {
    if (1) {
        oldState = 0;
        console.log('Released');
        robot.stop();
    }
});

function OpenWebsocket() {
    //ws = new WebSocket("ws://192.168.1.100/");
    //ws = new WebSocket("ws://192.168.43.49/");
    
ws = new WebSocket("ws://192.168.43.49/");
    ws.onopen = function () {
        console.log('Connection Opened');
    };
 
    ws.onclose = function () {
        alert("Connection closed");
        console.log('Connection Closed');
        OpenWebsocket();//reopen connection
    };
    ws.onmessage = function(event) {
        console.log("Received data is " + event.data);
        var voltgData;
        var voltage;
        var rawVolt;
        voltgData = JSON.parse(event.data);
        //rawVolt = voltgData.adc_raw;
        //voltage = 13.9*voltgData.adc_raw/4096;
        voltage = 13.9*voltgData.adc_raw/4096;
        voltage = Math.round(voltage * 10) / 10; 
        console.log("Current battery voltage is " + voltage);
    };
}
function CloseWebsocket() {
    ws.close();
}
function lightsON() {
    console.log('Lights = On');
    robot.lights.lts = 1;
    robot.setLights();
}
function camLightsON() {
    console.log('Camera lights = On');
    robot.camera.lt = 1;
    var dataToSend = JSON.stringify(robot.camera);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function lightsOFF() {
    console.log('Lights = Off');
    robot.lights.lts = 0;
    robot.setLights();
    robot.camera.lt = 0;
    var dataToSend = JSON.stringify(robot.camera);
    ws.send(dataToSend);
}
function SendData() {
    var dataToSend = document.getElementById("sliderH").value;
    robot.camera.horizontal = dataToSend;
    dataToSend = document.getElementById("sliderV").value;
    robot.camera.vertical = dataToSend;
    dataToSend = JSON.stringify(robot.camera);
    console.log(dataToSend);
    ws.send(dataToSend);
}

function SendBasePosition() {
    var dataToSend = document.getElementById("slider1").value;
    robot.hand.b = Number(dataToSend);
    //robot.hand.base = 200;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendElbowPosition() {
    var dataToSend = document.getElementById("slider2").value;
    robot.hand.e = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendWristPosition() {
    var dataToSend = document.getElementById("slider3").value;
    robot.hand.w = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendClawPosition() {
    var dataToSend = document.getElementById("slider4").value;
    robot.hand.c = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendHorizontalCameraPosition() {
    var dataToSend = document.getElementById("sliderH").value;
    robot.camera.h = Number(dataToSend);
    dataToSend = JSON.stringify(robot.camera);
    console.log(dataToSend);
    ws.send(dataToSend);   
}
function SendVerticalCameraPosition() {
    var dataToSend = document.getElementById("sliderV").value;
    robot.camera.v = Number(dataToSend);
    dataToSend = JSON.stringify(robot.camera);
    console.log(dataToSend);
    ws.send(dataToSend);   
}
function setSpeed() {
    maxSpeed = Number(document.getElementById("speed_slider").value);
    console.log("Speed is set to " + maxSpeed);
}
/*"use strict"
var oldState;  // previous user action is saved here
var ws = null; // variable for web socket
var robot = {
    motors: { // motors state
        dataType: 0,
        lS: 0,
        rS: 0
    },
    camera: { // camera servos state
        dataType: 1,
        horizontal: 0,
        vertical: 0
    },
    lights: { // onboard lights state
        dataType: 2,
        whiteLights: 255,
        IRLights: 255
    },
    hand: {
        dataType: 3,
        base: 0,
        elbow: 0,
        wrist: 0,
        claw: 0
    },
    goForward: function () {
        this.motors.lS = 255;
        this.motors.rS = 255;
        var dataToSend = JSON.stringify(robot.motors);
        console.log(dataToSend);
        ws.send(dataToSend);
    },
    goBack: function () {
        this.motors.lS = -255;
        this.motors.rS = -200;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
     
    stop: function () {
        this.motors.lS = 0;
        this.motors.rS = 0;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnRight: function () {
        this.motors.lS = 255;
        this.motors.rS = -255;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnLeft: function () {
        this.motors.lS = -255;
        this.motors.rS = 255;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    setLights: function (brightness) {
        this.lights.whiteLights = brightness;
        var dataToSend = JSON.stringify(robot.lights);
        console.log(dataToSend);
        ws.send(dataToSend);
    }
};

OpenWebsocket();

addEventListener("keydown", function (event) { // For going forward
    if (event.keyCode === 38 && event.keyCode !== oldState) { //if this key is pressed for the first time
        oldState = event.keyCode; // this key was pressed
        console.log('Pressed: UP');
        robot.goForward();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 40 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: DOWN');
        robot.goBack();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 37 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: LEFT');
        robot.turnLeft();
    }
});
addEventListener("keydown", function (event) {
    if (event.keyCode === 39 && event.keyCode !== oldState) {
        oldState = event.keyCode;
        console.log('Pressed: RIGHT');
        robot.turnRight();
    }
});

addEventListener("keyup", function (event) {
    if (1) {
        oldState = 0;
        console.log('Released');
        robot.stop();
    }
});

function OpenWebsocket() {
    //ws = new WebSocket("ws://192.168.1.100/");
    ws = new WebSocket("ws://192.168.43.49/");
    ws.onopen = function () {
        console.log('Connection Opened');
    };
 
    ws.onclose = function () {
        alert("Connection closed");
        console.log('Connection Closed');
        OpenWebsocket();//reopen connection
    };
    ws.onmessage = function(event) {
        console.log("Received data is " + event.data);
        var voltgData;
        var voltage;
        var rawVolt;
        voltgData = JSON.parse(event.data);
        //rawVolt = voltgData.adc_raw;
        //voltage = 13.9*voltgData.adc_raw/4096;
        voltage = 13.9*voltgData.adc_raw/4096;
        voltage = Math.round(voltage * 10) / 10; 
        console.log("Current battery voltage is " + voltage);
    };
}
function CloseWebsocket() {
    ws.close();
}
function lightsON() {
    
    document.getElementById("lightsON").disabled = true;
    document.getElementById("lightsOFF").disabled = false;
    console.log('Lights = On');
    robot.setLights(1);
}
function lightsOFF() {
    document.getElementById("lightsON").disabled = false;
    document.getElementById("lightsOFF").disabled = true;
    console.log('Lights = Off');
    robot.setLights(0);
}
function SendData() {
    var dataToSend = document.getElementById("sliderH").value;
    robot.camera.horizontal = dataToSend;
    dataToSend = document.getElementById("sliderV").value;
    robot.camera.vertical = dataToSend;
    dataToSend = JSON.stringify(robot.camera);
    console.log(dataToSend);
    ws.send(dataToSend);
}

function SendBasePosition() {
    var dataToSend = document.getElementById("slider1").value;
    robot.hand.base = Number(dataToSend);
    //robot.hand.base = 200;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendElbowPosition() {
    var dataToSend = document.getElementById("slider2").value;
    robot.hand.elbow = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendWristPosition() {
    var dataToSend = document.getElementById("slider3").value;
    robot.hand.wrist = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendClawPosition() {
    var dataToSend = document.getElementById("slider4").value;
    robot.hand.claw = Number(dataToSend);
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
*/