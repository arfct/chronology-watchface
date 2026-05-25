var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

function valueOf(setting) {
  return typeof setting === 'object' ? setting.value : setting;
}

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;

  var settings = clay.getSettings(e.response, false);
  var dict = {};

  if (settings.INVERT_COLORS !== undefined) {
    dict.INVERT_COLORS = valueOf(settings.INVERT_COLORS) ? 1 : 0;
  }
  if (settings.HAND_COLOR !== undefined) {
    dict.HAND_COLOR = parseInt(valueOf(settings.HAND_COLOR), 10);
  }
  if (settings.FACE_COLOR !== undefined) {
    dict.FACE_COLOR = parseInt(valueOf(settings.FACE_COLOR), 10);
  }

  Pebble.sendAppMessage(dict);
});
