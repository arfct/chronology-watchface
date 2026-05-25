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
        "type": "toggle",
        "messageKey": "INVERT_COLORS",
        "label": "Light mode",
        "defaultValue": false
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
      },
      {
        "type": "select",
        "messageKey": "FACE_COLOR",
        "label": "Face color",
        "defaultValue": 0,
        "options": [
          { "label": "Default", "value": 0 },
          { "label": "White", "value": 1 },
          { "label": "Black", "value": 2 },
          { "label": "Red", "value": 3 },
          { "label": "Orange", "value": 4 },
          { "label": "Yellow", "value": 5 },
          { "label": "Green", "value": 6 },
          { "label": "Blue", "value": 7 },
          { "label": "Purple", "value": 8 },
          { "label": "Pink", "value": 9 },
          { "label": "Gray", "value": 10 }
        ]
      }
      // {
      //   "type": "select",
      //   "messageKey": "FONT_SIZE",
      //   "label": "Font Size",
      //   "defaultValue": "medium",
      //   "options": [
      //     {
      //       "label": "Large (34pt)",
      //       "value": "large"
      //     },
      //     {
      //       "label": "Medium (28pt)",
      //       "value": "medium"
      //     },
      //     {
      //       "label": "Small (24pt)",
      //       "value": "small"
      //     },
      //     {
      //       "label": "Extra Small (18pt)",
      //       "value": "xsmall"
      //     }
      //   ]
      // }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];