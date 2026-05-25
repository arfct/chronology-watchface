module.exports = [
  {
    "type": "heading",
    "defaultValue": "Chronology Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display Settings"
      },
      {
        "type": "select",
        "messageKey": "BACKGROUND_COLOR",
        "label": "Background color",
        "defaultValue": 0,
        "options": [
          { "label": "Black", "value": 0 },
          { "label": "White", "value": 1 },
          { "label": "Red", "value": 2 },
          { "label": "Orange", "value": 3 },
          { "label": "Yellow", "value": 4 },
          { "label": "Green", "value": 5 },
          { "label": "Blue", "value": 6 },
          { "label": "Purple", "value": 7 },
          { "label": "Pink", "value": 8 },
          { "label": "Gray", "value": 9 }
        ]
      },
      {
        "type": "select",
        "messageKey": "FACE_COLOR",
        "label": "Face color",
        "defaultValue": 0,
        "options": [
          { "label": "Transparent", "value": 0 },
          { "label": "Black", "value": 1 },
          { "label": "White", "value": 2 },
          { "label": "Red", "value": 3 },
          { "label": "Orange", "value": 4 },
          { "label": "Yellow", "value": 5 },
          { "label": "Green", "value": 6 },
          { "label": "Blue", "value": 7 },
          { "label": "Purple", "value": 8 },
          { "label": "Pink", "value": 9 },
          { "label": "Gray", "value": 10 }
        ]
      },
      {
        "type": "select",
        "messageKey": "HAND_COLOR",
        "label": "Hand color",
        "defaultValue": 0,
        "options": [
          { "label": "Red", "value": 0 },
          { "label": "Orange", "value": 1 },
          { "label": "Yellow", "value": 2 },
          { "label": "Green", "value": 3 },
          { "label": "Blue", "value": 4 },
          { "label": "Purple", "value": 5 },
          { "label": "Pink", "value": 6 },
          { "label": "Gray", "value": 7 }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
