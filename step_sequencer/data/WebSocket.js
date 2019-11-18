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

function sendDATA() {
    const pitch = 2 ** D('pitch').value | 0
    const m_amount = D('mod_intensity').value
    const m_period = D('mod_period').value
    // const m_type = sine | tri | saw | rsaw | square
    const G = x => `<span style="color:purple">${x}</span>`
    D('screen').innerHTML = [
        `pitch: ${G(pitch | 0)}hz`,
        `mod intensity: ${G(m_amount)}%`,
        `mod period: ${G(m_period)}hz`
    ].join` &nbsp; - &nbsp; `

    const payload = [pitch, m_amount*100|0, m_period*100|0].join`:`
    connection.send(payload)
}