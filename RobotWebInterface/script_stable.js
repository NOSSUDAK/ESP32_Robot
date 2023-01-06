/*jslint es6:false*/
var oldState;
var newState;
var ws = null;
var carState = {
        leftSpeed:  0,
        rightSpeed: 0,
        lights: 0
};
OpenWebsocket();

addEventListener("keydown", function(event) { // For going forward
    if (event.keyCode == 38 && event.keyCode!= oldState){ //if this key is pressed for the first time
        oldState = event.keyCode; // this key was pressed
        console.log('Pressed: UP');
        //console.log(oldState);
        carState.leftSpeed = 255; 
        carState.rightSpeed = 200;
        SendData(); // send new values to the car
    }
});
addEventListener("keydown", function(event) {
    if (event.keyCode == 40 && event.keyCode!= oldState){
        oldState = event.keyCode;
        console.log('Pressed: DOWN');
        //console.log(oldState);
        carState.leftSpeed = -200;
        carState.rightSpeed = -255;
        SendData();
    }
});
addEventListener("keydown", function(event) {
    if (event.keyCode == 37 && event.keyCode!= oldState){
        oldState = event.keyCode;
        console.log('Pressed: LEFT');
        //console.log(oldState);
        carState.leftSpeed = -255;
        carState.rightSpeed = 255;
        SendData();
    }
});
addEventListener("keydown", function(event) {
    if (event.keyCode == 39 && event.keyCode!= oldState){
        oldState = event.keyCode;
        console.log('Pressed: RIGHT');
        //console.log(oldState);
        carState.leftSpeed = 255;
        carState.rightSpeed = -255;
        SendData();
    }
});

addEventListener("keyup", function(event) {
    if (1){
        oldState = 0;
        console.log('Released');
        carState.leftSpeed = 0;
        carState.rightSpeed = 0;
        SendData();
    }
});

function OpenWebsocket() {
    ws = new WebSocket("ws://192.168.1.102/test");
    ws.onopen = function() {
    console.log('Connection Opened');
    };
 
    ws.onclose = function() {
        alert("Connection closed");
        console.log('Connection Closed');
        OpenWebsocket();//reopen connection
        };
}
function CloseWebsocket(){
    ws.close();
}
function lightsON(){
    
    document.getElementById("lightsON").disabled = true;
    document.getElementById("lightsOFF").disabled = false;
    console.log('Lights = On');
    carState.lights = 255;
    SendData();
}
function lightsOFF(){
    document.getElementById("lightsON").disabled = false;
    document.getElementById("lightsOFF").disabled = true;
    console.log('Lights = Off');
    carState.lights = 0;
    SendData();
}
function SendData(){
    
    var textToSend = JSON.stringify(carState);
    //console.log(textToSend);
    ws.send(textToSend);
}