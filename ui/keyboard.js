/**
 * keyboard.js — 2-octave playable piano keyboard (C3 to B4).
 * Sends noteOn / noteOff via window.webplug.
 */
(function () {
  'use strict';

  // One octave layout. White keys get a sequential index; black keys store
  // their left-edge position as a multiple of white-key width from the
  // octave's left edge.
  var OCTAVE = [
    { offset: 0,  white: true  },            // C
    { offset: 1,  white: false, bx: 0.62 },  // C#
    { offset: 2,  white: true  },            // D
    { offset: 3,  white: false, bx: 1.62 },  // D#
    { offset: 4,  white: true  },            // E
    { offset: 5,  white: true  },            // F
    { offset: 6,  white: false, bx: 3.64 },  // F#
    { offset: 7,  white: true  },            // G
    { offset: 8,  white: false, bx: 4.63 },  // G#
    { offset: 9,  white: true  },            // A
    { offset: 10, white: false, bx: 5.62 },  // A#
    { offset: 11, white: true  },            // B
  ];

  var START_MIDI  = 48; // C3
  var NUM_OCTAVES = 2;
  var WHITE_W     = 34;
  var WHITE_H     = 100;
  var BLACK_W     = 22;
  var BLACK_H     = 62;

  var active = {};

  function noteOn(note) {
    if (active[note]) return;
    active[note] = true;
    window.webplug.send('noteOn', { note: note, velocity: 0.8 });
    var el = document.querySelector('[data-note="' + note + '"]');
    if (el) el.classList.add('active');
  }

  function noteOff(note) {
    if (!active[note]) return;
    delete active[note];
    window.webplug.send('noteOff', { note: note });
    var el = document.querySelector('[data-note="' + note + '"]');
    if (el) el.classList.remove('active');
  }

  function releaseAll() {
    Object.keys(active).forEach(function (n) { noteOff(parseInt(n, 10)); });
  }

  function makeKey(container, note, isBlack, left, w, h) {
    var el = document.createElement('div');
    el.className    = 'key ' + (isBlack ? 'black-key' : 'white-key');
    el.dataset.note = note;
    el.style.cssText = [
      'position:absolute',
      'left:'   + left + 'px',
      'width:'  + w    + 'px',
      'height:' + h    + 'px',
      'z-index:' + (isBlack ? 2 : 1),
    ].join(';');

    el.addEventListener('mousedown',  function (e) { e.preventDefault(); noteOn(note); });
    el.addEventListener('mouseenter', function (e) { if (e.buttons & 1) noteOn(note); });
    el.addEventListener('mouseup',    function ()  { noteOff(note); });
    el.addEventListener('mouseleave', function ()  { noteOff(note); });

    container.appendChild(el);
  }

  function build() {
    var container = document.getElementById('keyboard');
    if (!container) return;

    container.style.position   = 'relative';
    container.style.width      = (NUM_OCTAVES * 7 * WHITE_W) + 'px';
    container.style.height     = WHITE_H + 'px';
    container.style.userSelect = 'none';

    for (var oct = 0; oct < NUM_OCTAVES; oct++) {
      var octX    = oct * 7 * WHITE_W;
      var octMidi = START_MIDI + oct * 12;
      var wIdx    = 0;

      for (var i = 0; i < OCTAVE.length; i++) {
        var k    = OCTAVE[i];
        var note = octMidi + k.offset;

        if (k.white) {
          makeKey(container, note, false, octX + wIdx * WHITE_W, WHITE_W - 1, WHITE_H);
          wIdx++;
        } else {
          makeKey(container, note, true, octX + k.bx * WHITE_W, BLACK_W, BLACK_H);
        }
      }
    }

    document.addEventListener('mouseup', releaseAll);
  }

  document.addEventListener('DOMContentLoaded', build);
})();
