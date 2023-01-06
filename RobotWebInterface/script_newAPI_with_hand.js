"use strict"
var oldState;  // previous user action is saved here
var ws = null; // variable for web socket
var robot = {
    motors: { // motors state
        dataType: "motor",
        leftSpeed: 0,
        rightSpeed: 0
    },
    hand: {
        dataType:"hand",
        base: 0,
        elbow: 0,
        wrist: 0,
        claw: 0
    },
    camera: { // camera servos state
        dataType: "camera",
        horizontal: 0,
        vertical: 0
    },
    lights: { // onboard lights state
        dataType: "lights",
        whiteLights: 255,
        IRLights: 255
    },
    goForward: function () {
        this.motors.leftSpeed = 255;
        this.motors.rightSpeed = 200;
        var dataToSend = JSON.stringify(robot.motors);
        console.log(dataToSend);
        ws.send(dataToSend);
    },
    goBack: function () {
        this.motors.leftSpeed = -255;
        this.motors.rightSpeed = -200;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
     
    stop: function () {
        this.motors.leftSpeed = 0;
        this.motors.rightSpeed = 0;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnRight: function () {
        this.motors.leftSpeed = 255;
        this.motors.rightSpeed = -255;
        var dataToSend = JSON.stringify(robot.motors);
        ws.send(dataToSend);
    },
    turnLeft: function () {
        this.motors.leftSpeed = -255;
        this.motors.rightSpeed = 255;
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
    ws = new WebSocket("ws://192.168.1.101/test");
     
    ws.onopen = function () {
        console.log('Connection Opened');
    };
 
    ws.onclose = function () {
        alert("Connection closed");
        console.log('Connection Closed');
        OpenWebsocket();//reopen connection
    };
}
function CloseWebsocket() {
    ws.close();
}
function lightsON() {
    
    document.getElementById("lightsON").disabled = true;
    document.getElementById("lightsOFF").disabled = false;
    console.log('Lights = On');
    robot.setLights(255);
}
function lightsOFF() {
    document.getElementById("lightsON").disabled = false;
    document.getElementById("lightsOFF").disabled = true;
    console.log('Lights = Off');
    robot.setLights(0);
}
function SendCameraPosition() {
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
    robot.hand.base = dataToSend;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendElbowPosition() {
    var dataToSend = document.getElementById("slider2").value;
    robot.hand.elbow = dataToSend;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendWristPosition() {
    var dataToSend = document.getElementById("slider3").value;
    robot.hand.wrist = dataToSend;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}
function SendClawPosition() {
    var dataToSend = document.getElementById("slider4").value;
    robot.hand.claw = dataToSend;
    dataToSend = JSON.stringify(robot.hand);
    console.log(dataToSend);
    ws.send(dataToSend);
}