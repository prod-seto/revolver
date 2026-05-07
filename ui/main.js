/**
 * main.js — scaffold demo: ping/pong round trip + keyboard status.
 * Replace this file with your plugin's UI logic.
 */
document.addEventListener('DOMContentLoaded', function () {
  var statusEl      = document.getElementById('status');
  var responseEl    = document.getElementById('response');
  var pingBtn       = document.getElementById('ping-btn');
  var audioStatusEl = document.getElementById('audio-status');

  webplug.on('pong', function (payload) {
    responseEl.textContent = payload.message;
    statusEl.textContent   = 'connected';
    statusEl.className     = 'ok';
  });

  webplug.on('audioStatus', function (payload) {
    if (payload.sampleLoaded) {
      audioStatusEl.textContent = 'sample ready';
      audioStatusEl.className   = 'ok';
    } else {
      audioStatusEl.textContent = 'no sample loaded';
      audioStatusEl.className   = '';
    }
  });

  pingBtn.addEventListener('click', function () {
    statusEl.textContent = 'waiting…';
    statusEl.className   = '';
    webplug.send('ping', { message: 'Hello from the browser!' });
  });

  setTimeout(function () {
    webplug.send('ping', { message: 'auto-ping on load' });
  }, 600);
});
