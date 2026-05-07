/**
 * main.js — scaffold demo: ping/pong round trip with C++.
 * Replace this file with your plugin's UI logic.
 */
document.addEventListener('DOMContentLoaded', function () {
  var statusEl  = document.getElementById('status');
  var responseEl = document.getElementById('response');
  var pingBtn   = document.getElementById('ping-btn');

  // ── Incoming ─────────────────────────────────────────────────────────────────
  webplug.on('pong', function (payload) {
    responseEl.textContent  = payload.message;
    statusEl.textContent    = 'connected';
    statusEl.className      = 'ok';
  });

  // ── Outgoing ─────────────────────────────────────────────────────────────────
  pingBtn.addEventListener('click', function () {
    statusEl.textContent = 'waiting…';
    statusEl.className   = '';
    webplug.send('ping', { message: 'Hello from the browser!' });
  });

  // Auto-ping once the WebSocket is open (or immediately in plugin mode)
  setTimeout(function () {
    webplug.send('ping', { message: 'auto-ping on load' });
  }, 600);
});
