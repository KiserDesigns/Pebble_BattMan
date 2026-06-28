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
        "description": "'Dark' darkens background, lightens text<br>'Light' lightens background, darkens text",
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
        "capabilities": ["PLATFORM_EMERY"],
        "sunlight": false
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
            "label": "Locale Default",
            "value": "1" 
          },
          { 
            "label": "Mon, Apr 25",
            "value": "2" 
          },
          { 
            "label": "Mon, 25 Apr",
            "value": "3" 
          },
          { 
            "label": "mm/dd/yyyy",
            "value": "4" 
          },
          { 
            "label": "dd/mm/yyyy",
            "value": "6" 
          },
          { 
            "label": "yyyy/mm/dd",
            "value": "8" 
          },
          { 
            "label": "mm-dd-yyyy",
            "value": "5" 
          },
          { 
            "label": "dd-mm-yyyy",
            "value": "7" 
          },
          { 
            "label": "yyyy-mm-dd",
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
