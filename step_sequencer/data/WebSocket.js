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
    // const pitch    = 2 ** D('pitch').value | 0
    // const m_amount = D('mod_intensity').value
    // const m_period = 2 ** D('mod_period').value
    // const waveform = D('waveform').value
    // // const m_type = sine | tri | saw | rsaw | square
    // const G = x => `<span style="color:purple">${x}</span>`
    // D('screen').innerHTML = [
    //     `pitch: ${G(pitch | 0)}hz`,
    //     `mod intensity: ${G(m_amount)}%`,
    //     `mod period: ${G(m_period.toFixed(2))}hz <br>`,
    //     `mod waveform: ${G(
    //         'sine,tri,square,saw,reverse-saw,X,Y,Z,noise'.split`,`[waveform]
    //     )}`
    // ].join` &nbsp; - &nbsp; `

    // const payload = [pitch, m_amount*100|0, m_period*100|0, waveform].join`:`

    // payload = [
    //     D('tempo').value,
    //     ...[...notes.map(note => Object.values(note.dataset))]
    // ].join`:`
    // console.log(payload)
    const items = [D('tempo').value]
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