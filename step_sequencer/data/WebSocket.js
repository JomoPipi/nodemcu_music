const log = console.log
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.onopen = function () {
    connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
    console.log('Server: ', e.data);
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};
function D(x) { return document.getElementById(x) }
function E(e) { return document.createElement(e)  }
const notes = [...Array(8)]
/*

    Data needs to be of the form
    tempo
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave
    :isActive:pitchHZ:modAmount:modPeriod:wave

*/


function sendDATA() {
    const items = [((2**D('tempo').value)**2/256|0)]
    notes.forEach(note => {
        const d=note.dataset
        items.push([
            d.active,
            d.pitch,
            d.modA,
            d.modP,
            d.wave
        ].join`:`)
    })
    const payload = items.join`:`
    log(payload)
    connection.send(payload)
}