module.exports = [
  {
    "type": "heading",
    "defaultValue": "BattMan Settings"
  },
  {
    "type": "text",
    "defaultValue": "Set Colors and Preferences."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },{
        "type": "select",
        "messageKey": "Theme",
        "defaultValue": "d",
        "label": "Theme",
        "options": [
          { 
            "label": "Dark",
            "value": "d" 
          },
          { 
            "label": "Light",
            "value": "l" 
          }
        ],
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color",
        "layout": [["0xAAAAAA", "0xFF0000", "0xFFFF00", "0x00FF00", "0x00FFFF", "0x0000FF", "0xFF00FF"]],
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "TextColor",
        "defaultValue": "0xFFFFFF",
        "label": "Text Color",
        "layout": [["0xAAAAAA", "0xFF0000", "0xFFFF00", "0x00FF00", "0x00FFFF", "0x0000FF", "0xFF00FF"]],
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Color Theme",
        "capabilities": ["BW"]
      },
      {
        "type": "color",
        "messageKey": "LightColor",
        "defaultValue": "0xFFFFFF",
        "label": "Backlight Color",
        "layout": [[false,      "0xFFAAAA", "0xFFFFAA", "0xAAFFAA", "0xAAFFFF", "0xAAAAFF", "0xFFAAFF"],
                   [false,      "0xFF5555", "0xFFFF55", "0x55FF55", "0x55FFFF", "0x5555FF", "0xFF55FF"],
                   ["0xFFFFFF", "0xFF0000", "0xFFFF00", "0x00FF00", "0x00FFFF", "0x0000FF", "0xFF00FF"],
                   [false,      "0xFF00AA", "0xFFAA00", "0xAAFF00", "0x00FFAA", "0x00AAFF", "0xAA00FF"],
                   [false,      "0xFF55AA", "0xFFAA55", "0xAAFF55", "0x55FFAA", "0x55AAFF", "0xAA55FF"]],
        "capabilities": ["PLATFORM_EMERY"]
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
        "type": "select",
        "messageKey": "DateFormat",
        "defaultValue": "1",
        "label": "Day/Date Format",
        "options": [
          { 
            "label": "Mon, Apr 25",
            "value": "1" 
          },
          { 
            "label": "Mon, 25 Apr",
            "value": "2" 
          },
          { 
            "label": "Apr 25",
            "value": "3" 
          },
          { 
            "label": "25 Apr",
            "value": "4" 
          },
          { 
            "label": "25",
            "value": "5" 
          },
          { 
            "label": "Monday",
            "value": "6" 
          },
          { 
            "label": "Monday 25",
            "value": "7" 
          },
          { 
            "label": "2026-04-25",
            "value": "8" 
          },
          { 
            "label": "04/25/26",
            "value": "9" 
          }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
