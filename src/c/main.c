#include <pebble.h>

// Persistent storage key
#define SETTINGS_KEY 1
#define BATT_DATA_KEY 2

// Define our settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TextColor;
  bool TemperatureUnit; // false = Celsius, true = Fahrenheit
  bool ShowDate;
} ClaySettings;

// An instance of the struct
static ClaySettings settings;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_batt_data_layer;

// Custom fonts
static GFont s_time_font;
static GFont s_date_font;
static GFont s_info_font;

// Battery
static Layer *s_battery_layer;
static int s_battery_level;
static bool s_charging;

// Define our battery data struct
typedef struct BatteryData {
  int seconds_per_percent_discharging;
  int seconds_per_percent_charging;
} BatteryData;

// An instance of the struct
static BatteryData batt_data;

static int sample_percent;
static bool sample_charging;
static time_t sample_time;

// Bluetooth
static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

// Unobstructed area
static Layer *s_window_layer;

// Set default settings
static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.TextColor = GColorWhite;
  settings.TemperatureUnit = false; // Celsius
  settings.ShowDate = true;
}

// Set default battery data
static void prv_default_batt_data() {
  batt_data.seconds_per_percent_discharging = 6048; //6048 * 100 = 604800 seconds = 7 days 
  batt_data.seconds_per_percent_charging = 60; //one minute per percent, fully charged in 1 hour 40 minutes
}

// Save settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save battery data to persistent storage
static void prv_save_batt_data() {
  persist_write_data(BATT_DATA_KEY, &batt_data, sizeof(batt_data));
}

// Read settings from persistent storage
static void prv_load_settings() {
  // Set defaults first
  prv_default_settings();
  // Then override with any saved values
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Read battery data from persistent storage
static void prv_load_batt_data() {
  // Set defaults first
  prv_default_batt_data();
  // Then override with any saved values
  persist_read_data(BATT_DATA_KEY, &batt_data, sizeof(batt_data));
}

// Apply settings to UI elements
static void prv_update_display() {
  // Set background color
  window_set_background_color(s_main_window, settings.BackgroundColor);

  // Set text colors
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_text_color(s_weather_layer, settings.TextColor);
  text_layer_set_text_color(s_batt_data_layer, settings.TextColor);

  // Show/hide date based on setting
  layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);

  // Mark battery layer for redraw (color may have changed)
  layer_mark_dirty(s_battery_layer);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                                    "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }
}

static void prv_format_seconds_elapsed(int e_seconds, char* e_buffer, int e_buff_size){
  // XXdXXh
  // XXhXXm
  // XXmXXs
  // XXs
  if (e_seconds < 0){
    snprintf(e_buffer, e_buff_size, "<0s");
    return;
  }
  if (e_seconds <= 90){ //XXs
    snprintf(e_buffer, e_buff_size, "%ds", e_seconds);
  } else if (e_seconds < 3600) { // < 1 hour, XXmXXs
    int e_minutes = e_seconds / 60;
    e_seconds = e_seconds - (e_minutes * 60);
    snprintf(e_buffer, e_buff_size, "%dm%ds", e_minutes, e_seconds);
  } else if (e_seconds < 172800) { // < 48 hours, XXhXXm
    int e_hours = e_seconds / 3600;
    int e_minutes = (e_seconds - (e_hours * 3600)) / 60;
    snprintf(e_buffer, e_buff_size, "%dh%dm", e_hours, e_minutes);
  } else if (e_seconds < 8640000) { // < 100 days, XXdXXh
    int e_days = e_seconds / 86400;
    int e_hours = (e_seconds - (e_days * 86400)) / 3600;
    snprintf(e_buffer, e_buff_size, "%dd%dh", e_days, e_hours);
  } else { //>99 days
    snprintf(e_buffer, e_buff_size, ">99d");
  }
}

static void prv_set_batt_data_text(int percent, int rate, bool charging){
  
  // XXX%, full in XXdXXh
  // XXX%, XXhXXm remaining
  // XXX%, XXdXXh left
  static char batt_data_buffer[42];
  // XXdXXh
  // XXhXXm
  // XXmXXs
  // XXs
  static char avg_rate_buffer[10];
  static char rate_buffer[10];
  static char eta_buffer[10];
  int eta;
  if (charging){
    eta = (100 - percent) * batt_data.seconds_per_percent_charging;
    prv_format_seconds_elapsed(batt_data.seconds_per_percent_charging, avg_rate_buffer, sizeof(avg_rate_buffer));
  } else {
    eta = percent * batt_data.seconds_per_percent_discharging;
    prv_format_seconds_elapsed(batt_data.seconds_per_percent_discharging, avg_rate_buffer, sizeof(avg_rate_buffer));
  }
  
  prv_format_seconds_elapsed(eta, eta_buffer, sizeof(eta_buffer));
  
  if (rate < 0){ // waiting for samples. App just started, or charging status just changed
    if (charging){
      snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, full in %s\n%s/1%% chrg", percent, eta_buffer, avg_rate_buffer);
    } else {
      snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, %s left\n%s/1%% drop", percent, eta_buffer, avg_rate_buffer);
    }
  } else {
    prv_format_seconds_elapsed(rate, rate_buffer, sizeof(rate_buffer));
    if (charging){
      snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, full in %s\n%s/1%% chrg", percent, eta_buffer, rate_buffer);
    } else {
      snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, %s left\n%s/1%% drop", percent, eta_buffer, rate_buffer);
    }
  }
  
  text_layer_set_text(s_batt_data_layer, batt_data_buffer);
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  s_charging = state.is_charging;
  layer_mark_dirty(s_battery_layer); 
  
  //do calculations
  
  if (sample_percent == -2){ //initial call when app is opened
    sample_percent = -1;
    prv_set_batt_data_text(state.charge_percent, -2, state.is_charging);
    return;
  }
  if (sample_percent == -1){ //first valid battery state change call
    sample_percent = state.charge_percent;
    sample_charging = state.is_charging;
    time(&sample_time);
    prv_set_batt_data_text(state.charge_percent, -1, state.is_charging);
    return;
  }
  
  if (sample_charging == state.is_charging){ // only do calculations if the charging status has not changed
    if ((sample_charging == true) && (sample_percent < state.charge_percent)){ // charging for two samples and percent increasing, calculate charge rate
      int deltaPercent = state.charge_percent - sample_percent;
      int deltaTime = time(NULL) - sample_time;
      int chargeRate = deltaTime / deltaPercent;
      if (deltaPercent > 5) { // for bigger gaps in charge percentage (10% is typical), take 62.5% of the running average, and 37.5% of the new rate
        batt_data.seconds_per_percent_charging = (10*batt_data.seconds_per_percent_charging + 6*chargeRate) / 16;
      } else { // for smaller gaps in charge percentage (1% is typical on emery and gabbro) take 93.75% of the running, and 6.25% of the new rate
        batt_data.seconds_per_percent_charging = (15*batt_data.seconds_per_percent_charging + 1*chargeRate) / 16;
      }
      prv_save_batt_data();
      prv_set_batt_data_text(state.charge_percent, chargeRate, state.is_charging);
    } else if ((sample_charging == false) && (sample_percent > state.charge_percent)){ // discharging for two samples and percent decreasing, caluclate discharge rate
      int deltaPercent = sample_percent - state.charge_percent;
      int deltaTime = time(NULL) - sample_time;
      int dischargeRate = deltaTime / deltaPercent;
      if (deltaPercent > 5) { // for bigger gaps in charge percentage (10% is typical), take 62.5% of the running average, and 37.5% of the new rate
        batt_data.seconds_per_percent_discharging = (10*batt_data.seconds_per_percent_discharging + 6*dischargeRate) / 16;
      } else { // for smaller gaps in charge percentage (1% is typical on emery and gabbro) take 93.75% of the running, and 6.25% of the new rate
        batt_data.seconds_per_percent_discharging = (15*batt_data.seconds_per_percent_discharging + 1*dischargeRate) / 16;
      }
      prv_save_batt_data();
      prv_set_batt_data_text(state.charge_percent, dischargeRate, state.is_charging);
    } else { 
      //percent change is inconsistent with charging status, do not calculate a rate;
      prv_set_batt_data_text(state.charge_percent, -1, state.is_charging);
    }
    // if the charging status has not chaged and the percentage has, save the current sample
    if (sample_percent != state.charge_percent) {
      sample_percent = state.charge_percent;
      sample_charging = state.is_charging;
      time(&sample_time);
    }
  } else { // the charging status has changed, wait for two new samples
    sample_percent = -2;
    prv_set_batt_data_text(state.charge_percent, -2, state.is_charging);
  }
  
  
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar (inside the border)
  int bar_width = ((s_battery_level * (bounds.size.w - 6)) / 100);

  // Draw the border using the text color
  graphics_context_set_stroke_color(ctx, settings.TextColor);
  graphics_draw_round_rect(ctx, GRect(0,0,bounds.size.w-2, bounds.size.h), 2);

  // Choose color based on battery level
  //GColor bar_color;
  //if (s_battery_level <= 20) {
  //  bar_color = PBL_IF_COLOR_ELSE(GColorRed, settings.TextColor);
  //} else if (s_battery_level <= 40) {
  //  bar_color = PBL_IF_COLOR_ELSE(GColorChromeYellow, settings.TextColor);
  //} else {
  //  bar_color = PBL_IF_COLOR_ELSE(GColorGreen, settings.TextColor);
  //}

  // Draw the filled bar inside the border
  graphics_context_set_fill_color(ctx, settings.TextColor);
  graphics_fill_rect(ctx, GRect(2, 2, bar_width, bounds.size.h - 4), 1, GCornerNone);
  
  // Draw battery nubbin
  graphics_fill_rect(ctx, GRect(bounds.size.w-2, bounds.size.h/2 - 2, 2, 4), 1, GCornerNone);
  
  // if charging, draw a lightning bolt in the middle
  if (s_charging){
    graphics_context_set_stroke_width(ctx, 1);
    //graphics_draw_line(ctx, GPoint(bounds.size.w/2-1,0), GPoint(bounds.size.w/2-1,bounds.size.h));
    int i;
    graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
    for (i = 0; i <= bounds.size.h/2 + 1; i++){
      graphics_draw_line(ctx, GPoint(bounds.size.w/2-2-(i/2),i), GPoint(bounds.size.w/2,i));
      graphics_draw_line(ctx, GPoint(bounds.size.w/2-1+(i/2),bounds.size.h-i-1), GPoint(bounds.size.w/2-3,bounds.size.h-i-1));
    }
    graphics_context_set_stroke_color(ctx, settings.TextColor);
    for (i = 0; i <= bounds.size.h/2; i++){ 
      graphics_draw_line(ctx, GPoint(bounds.size.w/2-1-(i/2),i), GPoint(bounds.size.w/2-1,i));
      graphics_draw_line(ctx, GPoint(bounds.size.w/2-2+(i/2),bounds.size.h-i-1), GPoint(bounds.size.w/2-2,bounds.size.h-i-1));
    }
  }
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  //layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  //if (!connected) {
  //  vibes_double_pulse();
  //}
}

// AppMessage received handler
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Check for weather data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[42];

    int temp_value = (int)temp_tuple->value->int32;

    // Convert to Fahrenheit if setting is enabled
    if (settings.TemperatureUnit) {
      temp_value = (temp_value * 9 / 5) + 32;
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", temp_value);
    } else {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", temp_value);
    }

    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }

  // Check for Clay settings data
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
    #ifdef PBL_BW
    settings.TextColor = gcolor_equal(GColorBlack, settings.BackgroundColor) ? GColorWhite : GColorBlack;
    #endif
    
    //#ifdef PBL_COLOR
    //if(bg_color_t->value->int32 > 0x888888) { //bright background of 0xFF or 0xAA for each color, subtract 0xAAAAAA to make it 0x55 or 0x00
    //  settings.TextColor = GColorFromHEX(bg_color_t->value->int32 - 0xAAAAAA);
    //} else { //dark background of 0x55 or 0x00 for each color, add 0xAAAAAA to make it 0xFF or 0xAA
    //  settings.TextColor = GColorFromHEX(bg_color_t->value->int32 + 0xAAAAAA);
    //}
    //#endif
  }

  Tuple *text_color_t = dict_find(iterator, MESSAGE_KEY_TextColor);
  if (text_color_t) {
    settings.TextColor = GColorFromHEX(text_color_t->value->int32);
  }

  Tuple *temp_unit_t = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_t) {
    settings.TemperatureUnit = temp_unit_t->value->int32 == 1;
  }

  Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (show_date_t) {
    settings.ShowDate = show_date_t->value->int32 == 1;
  }

  // Save and apply if any settings were changed
  if (bg_color_t || text_color_t || temp_unit_t || show_date_t) {
    prv_save_settings();
    prv_update_display();

    // Refetch weather if the temperature unit changed so the display updates
    if (temp_unit_t) {
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);
      dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
      app_message_outbox_send();
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// Unobstructed area handlers
static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area, void *context) {
  // Hide weather during the transition to reduce clutter
  //layer_set_hidden(text_layer_get_layer(s_weather_layer), true);
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bounds = layer_get_bounds(s_window_layer);
  // Reposition time, date, and weather to fit in the available space
  //int date_height = 30;
  //int block_height = 56 + date_height;
  //int time_y = (bounds.size.h / 2) - (block_height / 2) - 10;
  //int date_y = time_y + 56;
  int weather_y = bounds.size.h - 40;
  #ifdef PBL_PLATFORM_GABBRO
  weather_y = bounds.size.h - 55;
  #endif

  GRect time_frame = layer_get_frame(text_layer_get_layer(s_time_layer));
//  time_frame.origin.y = time_y;
  layer_set_frame(text_layer_get_layer(s_time_layer), time_frame);

  GRect date_frame = layer_get_frame(text_layer_get_layer(s_date_layer));
//  date_frame.origin.y = date_y;
  layer_set_frame(text_layer_get_layer(s_date_layer), date_frame);

  GRect weather_frame = layer_get_frame(text_layer_get_layer(s_weather_layer));
  weather_frame.origin.y = weather_y;
  layer_set_frame(text_layer_get_layer(s_weather_layer), weather_frame);
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bool obstructed = !grect_equal(&full_bounds, &bounds);
  
  //layer_set_hidden(text_layer_get_layer(s_weather_layer), obstructed);
  
  // Keep BT icon hidden when obstructed, otherwise restore based on connection
  //if (obstructed) {
  //  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);
  //} else {
  //  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer),
  //    connection_service_peek_pebble_app_connection());
  //}
}

static void main_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  #ifdef PBL_PLATFORM_EMERY
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  #define date_time_padding 15
  int time_y = 0;
  int date_y = time_y + 54;
  int batt_y = date_y + 34;
  int bar_y = 65;
  int weather_y = bounds.size.h - 40;
  
  #elif PBL_PLATFORM_CHALK
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  #define date_time_padding 10
  int time_y = 54;
  int date_y = 30;
  int batt_y = 95;
  int bar_y = 10;
  int weather_y = bounds.size.h - 40;
  
  #elif PBL_PLATFORM_GABBRO
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  #define date_time_padding 10
  int time_y = 74;
  int date_y = 42;
  int batt_y = 130;
  int bar_y = 20;
  int weather_y = bounds.size.h - 55;
  
  #else
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  #define date_time_padding 10
  int time_y = 0;
  int date_y = time_y + 44;
  int batt_y = date_y + 24;
  int bar_y = 50;
  int weather_y = bounds.size.h - 40;
  #endif

  // Create the time TextLayer
  s_time_layer = text_layer_create(
      GRect(date_time_padding, time_y, bounds.size.w - (2*date_time_padding), 70));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, PBL_IF_RECT_ELSE (GTextAlignmentLeft, GTextAlignmentCenter));

  // Create the date TextLayer — just below the time
  s_date_layer = text_layer_create(
      GRect(date_time_padding, date_y, bounds.size.w - (2*date_time_padding), 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, PBL_IF_RECT_ELSE (GTextAlignmentLeft, GTextAlignmentCenter));

  // Create weather TextLayer — aligned to the bottom of the screen
  s_weather_layer = text_layer_create(
      GRect(date_time_padding, weather_y, bounds.size.w - (2*date_time_padding), 30));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, settings.TextColor);
  text_layer_set_font(s_weather_layer, s_info_font);
  text_layer_set_text_alignment(s_weather_layer, PBL_IF_RECT_ELSE (GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text(s_weather_layer, "Loading...");
  
  // Create battery data TextLayer — just below the date
  s_batt_data_layer = text_layer_create(
      GRect(date_time_padding, batt_y, bounds.size.w - (2*date_time_padding), 70));
  text_layer_set_background_color(s_batt_data_layer, GColorClear);
  text_layer_set_text_color(s_batt_data_layer, settings.TextColor);
  text_layer_set_font(s_batt_data_layer, s_info_font);
  text_layer_set_text_alignment(s_batt_data_layer, PBL_IF_RECT_ELSE (GTextAlignmentRight, GTextAlignmentCenter));
  text_layer_set_text(s_batt_data_layer, "Battery Data");

  // Create battery meter Layer — visible bar near the top
  int bar_width = 34;
  int bar_x = (bounds.size.w - bar_width - date_time_padding);
  #ifndef PBL_RECT
  bar_x = (bounds.size.w - bar_width) / 2;
  #endif
  s_battery_layer = layer_create(GRect(bar_x, bar_y, bar_width, 12));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Create the Bluetooth icon GBitmap
  //s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  //int bt_y = bar_y + 12;
  //s_bt_icon_layer = bitmap_layer_create(GRect((bounds.size.w - 30) / 2, bt_y, 30, 30));
  //bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  //bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);

  // Add layers to the Window
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_batt_data_layer));
  layer_add_child(s_window_layer, s_battery_layer);
  //layer_add_child(s_window_layer, bitmap_layer_get_layer(s_bt_icon_layer));

  // Apply saved settings
  prv_update_display();

  // Apply correct layout in case Quick View is already active
  prv_unobstructed_change(0, NULL);
  prv_unobstructed_did_change(NULL);

  // Subscribe to unobstructed area events
  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_batt_data_layer);
  //fonts_unload_custom_font(s_time_font);
  //fonts_unload_custom_font(s_date_font);
  layer_destroy(s_battery_layer);
  //gbitmap_destroy(s_bt_icon_bitmap);
  //bitmap_layer_destroy(s_bt_icon_layer);
}

static void init() {
  // Load settings before creating UI
  prv_load_settings();
  
  // Load battery rate data
  prv_load_batt_data();

  s_main_window = window_create();
  window_set_background_color(s_main_window, settings.BackgroundColor);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  update_time();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Reset battery sampling data
  sample_percent = -2; // need 2 samples (one on app load, and one at a percentage boundary) to set inital sample
  sample_charging = false;
  sample_time = 0;
  
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
