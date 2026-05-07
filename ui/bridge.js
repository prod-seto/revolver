/**
 * bridge.js — transport-agnostic message layer.
 *
 * In dev mode (browser):   uses WebSocket → ws://localhost:9001
 * In plugin mode (JUCE):   uses window.__juce__ native function bridge
 *
 * Public API (window.webplug):
 *   webplug.send(type, payload)   — send message to C++
 *   webplug.on(type, fn)          — register handler for incoming message type
 */
(function () {
  'use strict';

  var _handlers = {};
  var _queue    = [];  // messages buffered before WebSocket opens
  var _ws       = null;

  function dispatch(msg) {
    var fn = _handlers[msg.type];
    if (fn) fn(msg.payload || {});
  }

  function sendRaw(json) {
    if (window.__juce__) {
      var juce = window.__juce__;
      if (juce.backend && juce.backend.triggerEvent) {
        juce.backend.triggerEvent('webplugMessage', json);
      } else if (juce.invoke) {
        juce.invoke('webplugMessage', json, function () {});
      }
    } else if (_ws && _ws.readyState === WebSocket.OPEN) {
      _ws.send(json);
    } else {
      _queue.push(json);
    }
  }

  // ── WebSocket transport (dev mode only) ──────────────────────────────────────
  if (!window.__juce__) {
    (function connect() {
      _ws = new WebSocket('ws://localhost:9001');

      _ws.onopen = function () {
        console.log('[webplug] connected to DevServer');
        _queue.forEach(function (m) { _ws.send(m); });
        _queue = [];
      };

      _ws.onclose = function () {
        console.log('[webplug] disconnected — retrying in 1.5s');
        setTimeout(connect, 1500);
      };

      _ws.onerror = function (e) {
        console.warn('[webplug] WebSocket error', e);
      };

      _ws.onmessage = function (e) {
        try { dispatch(JSON.parse(e.data)); }
        catch (err) { console.error('[webplug] parse error', err); }
      };
    })();
  }

  // ── JUCE transport: C++ pushes messages by calling this function ─────────────
  window.webplugReceive = function (json) {
    try { dispatch(typeof json === 'string' ? JSON.parse(json) : json); }
    catch (err) { console.error('[webplug] receive error', err); }
  };

  // ── Public API ───────────────────────────────────────────────────────────────
  window.webplug = {
    send: function (type, payload) {
      sendRaw(JSON.stringify({ type: type, payload: payload || {} }));
    },
    on: function (type, fn) {
      _handlers[type] = fn;
    },
  };

  // Signal C++ that the UI has loaded and is ready to receive messages
  document.addEventListener('DOMContentLoaded', function () {
    window.webplug.send('uiReady', {});
  });
})();
