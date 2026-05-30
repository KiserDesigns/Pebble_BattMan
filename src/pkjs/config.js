module.exports = [
  {
    "type": "heading",
    "defaultValue": "Watchface Settings"
  },
  {
    "type": "text",
    "defaultValue": "Customize your watchface appearance and preferences."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color",
        "layout": [["0x000000", "0x550000", "0x555500", "0x005500", "0x005555", "0x000055", "0x550055"],
                   ["0xFFFFFF", "0xFFAAAA", "0xFFFFAA", "0xAAFFAA", "0xAAFFFF", "0xAAAAFF", "0xFFAAFF"]],
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "TextColor",
        "defaultValue": "0xFFFFFF",
        "label": "Text Color",
        "layout": [["0x000000", "0x550000", "0x555500", "0x005500", "0x005555", "0x000055", "0x550055"],
                   ["0xFFFFFF", "0xFFAAAA", "0xFFFFAA", "0xAAFFAA", "0xAAFFFF", "0xAAAAFF", "0xFFAAFF"]],
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Color Theme",
        "capabilities": ["BW"]
      }
      
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Preferences"
      },
      {
        "type": "toggle",
        "messageKey": "TemperatureUnit",
        "label": "Use Fahrenheit",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "ShowDate",
        "label": "Show Date",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
