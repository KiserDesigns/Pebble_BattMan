#include <pebble.h>
#include <battman/battman.h>

// Persistent storage key
#define SETTINGS_KEY 2

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
//static TextLayer *s_weather_layer;
static TextLayer *s_batt_data_layer;

// Custom fonts
static GFont s_time_font;
static GFont s_date_font;
static GFont s_info_font;

// Battery
static Layer *s_battery_layer;
static int s_battery_level;
static bool s_charging;

// Battery Stats Graph
static Layer *s_stats_layer;

// Unobstructed area
static Layer *s_window_layer;

// Set default settings
static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.TextColor = GColorWhite;
  settings.ShowDate = true; //No longer used.
}


// Save settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Read settings from persistent storage
static void prv_load_settings() {
  // Set defaults first
  prv_default_settings();
  // Then override with any saved values
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Apply settings to UI elements
static void prv_update_display() {
  // Set background color
  window_set_background_color(s_main_window, settings.BackgroundColor);

  // Set text colors
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_text_color(s_batt_data_layer, settings.TextColor);

  // Show/hide date based on setting
  //FORCE THIS TO BE SHOWN, this is a design choice
  // layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);

  // Mark battery layer for redraw (color may have changed)
  layer_mark_dirty(s_battery_layer);
  layer_mark_dirty(s_stats_layer);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                                    "%H:%M" : (tick_time->tm_hour > 12 ? "%l:%M " : "%l:%M"), tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);

  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %d", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
  
  battman.process();
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
  
  int days = e_seconds / 86400;
  int seconds = e_seconds % 86400;
  int hours = seconds / 3600;
  seconds = seconds % 3600;
  int minutes = seconds / 60;
  seconds = seconds % 60;
  
  if (days > 99) { //>99 days
    snprintf(e_buffer, e_buff_size, ">99d");
    return;
  }
  
  if (days > 0 && days <= 99){ // < 99 days, XXdXXh
    snprintf(e_buffer, e_buff_size, "%dd %dh", days, hours);
  } else if (hours > 0) { // < 24 hours, XXhXXm
    snprintf(e_buffer, e_buff_size, "%dh %dm", hours, minutes);
  } else if (minutes > 0) { // < 1 hours, XXmXXs
    snprintf(e_buffer, e_buff_size, "%dm %ds", minutes, seconds);
  } else { // < 1 minute, XXs    
    snprintf(e_buffer, e_buff_size, "%ds", e_seconds);
  }
}

static void prv_set_batt_data_text(BatteryChargeState state) {
  
  int percent = state.charge_percent;
  bool charging = state.is_charging;
  
  // XXX%, full in XXdXXh
  // XXX%, XXhXXm remaining
  // XXX%, XXdXXh left
  static char batt_data_buffer[64];
  // XXdXXh
  // XXhXXm
  // XXmXXs
  // XXs
  //static char avg_rate_buffer[10];
  //static char rate_buffer[10];
  static char eta_buffer[10];
  //static char last_buffer[10];
  int eta;
  if (charging){
    eta = battman.time_at_full - time(NULL);
    //APP_LOG (APP_LOG_LEVEL_INFO,"Time at Full: %d", battman.time_at_full);
    //prv_format_seconds_elapsed(battman.charge_rate, avg_rate_buffer, sizeof(avg_rate_buffer));
    //APP_LOG (APP_LOG_LEVEL_INFO,"Charge Rate: %d", battman.charge_rate);
  } else {
    eta = battman.time_at_empty - time(NULL);
    //APP_LOG (APP_LOG_LEVEL_INFO,"Time at Empty: %d", battman.time_at_empty);
    //prv_format_seconds_elapsed(0-battman.discharge_rate, avg_rate_buffer, sizeof(avg_rate_buffer));
    //APP_LOG (APP_LOG_LEVEL_INFO,"Discharge Rate: %d", 0-battman.discharge_rate);
  }
  
  prv_format_seconds_elapsed(eta, eta_buffer, sizeof(eta_buffer));
  
  //prv_format_seconds_elapsed(rate, rate_buffer, sizeof(rate_buffer));
  
  //prv_format_seconds_elapsed(time(NULL)-(battman.data[BATTMAN_NUM_SAMPLES-1] & 0xFFFFFF80), last_buffer, sizeof(last_buffer));
  
  // //(int)battman.data[BATTMAN_NUM_SAMPLES-1] & 0x0000007F
  
  if (charging){
    //snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, full in %s.\n%s/1%% chrg\n(%d%% %s ago)", percent, eta_buffer, rate_buffer, (int)battman.data[BATTMAN_NUM_SAMPLES-1] & 0x0000007F, last_buffer);
    snprintf(batt_data_buffer, sizeof(batt_data_buffer), "full: %s", eta_buffer);
  } else {
    //snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%d%%, %s left.\n%s/1%% drop\n(%d%% %s ago)", percent, eta_buffer, rate_buffer, (int)battman.data[BATTMAN_NUM_SAMPLES-1] & 0x0000007F, last_buffer);
    snprintf(batt_data_buffer, sizeof(batt_data_buffer), "%s left", eta_buffer);
  }
  text_layer_set_text(s_batt_data_layer, batt_data_buffer);
}


static void battery_callback(BatteryChargeState state) {
  //do the battman callback within the main battery callback
  battman.callback(state);
  APP_LOG (APP_LOG_LEVEL_INFO ,"BATTERY CALLBACK");
  s_battery_level = state.charge_percent;
  s_charging = state.is_charging;
  layer_mark_dirty(s_battery_layer);
  layer_mark_dirty(s_stats_layer); 
  
  prv_set_batt_data_text(state);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  prv_set_batt_data_text(battery_state_service_peek());
}

static void stats_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, settings.TextColor);
  graphics_draw_round_rect(ctx, GRect(0,0,bounds.size.w, bounds.size.h), 0);
  
  time_t max_t = battman.time_at_empty;
  time_t min_t = 0;
  
  APP_LOG (APP_LOG_LEVEL_INFO,"Updating Stats");
  
  int i;
  int recent_peak_index = -1;
  // iterate through the samples backwards to find the first rising edge. the right side of that edge will be the most recent peak
  for (i = BATTMAN_NUM_SAMPLES - 1; i > 1; i--){
    if((battman.data[i]&battman.percent_mask) > (battman.data[i-1]&battman.percent_mask)){
      recent_peak_index = i;
      //APP_LOG (APP_LOG_LEVEL_INFO,"Found Peak at Index: %d", recent_peak_index);
      i = -1; //exit loop
    }
  }
  if (recent_peak_index == -1){
    // no rising edge found? This shouldn't be possible (unless there are no samples??)
    // so don't do anything else.
    //APP_LOG (APP_LOG_LEVEL_INFO,"No recent peak found");
  } else {
    graphics_context_set_stroke_width(ctx, 1);
    min_t = battman.data[recent_peak_index]&battman.time_mask;
    int dT = max_t - min_t; //this is the time "width" of the window, from the start of the most recent peak to the time when the battery should be dead
    //iterate through to find the first non-zero sample to base the first time on:
    min_t = 0;
    //iterate through the samples drawing lines connecting samples, and whenever a rising edge is found, start again
    for (i = 0; i < BATTMAN_NUM_SAMPLES - 1; i++) {
      uint32_t curr = battman.data[i];
      uint32_t next = battman.data[i+1];
      if(curr!=0 && min_t == 0){
        min_t = curr&battman.time_mask;
        //APP_LOG (APP_LOG_LEVEL_INFO,"Start drawing at t=%ul", min_t);
      }
      if (i == recent_peak_index){ //it's the start of the most recent discharge, make this one big
        graphics_context_set_stroke_width(ctx, 3);
      }
      if (curr!=0 && next!=0){
        if ((curr&battman.percent_mask) < (next&battman.percent_mask)){
          //this is a rising edge, start over
          min_t = next&battman.time_mask;
          //APP_LOG (APP_LOG_LEVEL_INFO,"Start again at t=%ul", min_t);
        } else {
          //this is a falling edge or flat line, draw it
          int y1 = bounds.size.h * (curr&battman.percent_mask) / 100;
          int y2 = bounds.size.h * (next&battman.percent_mask) / 100;
          int x1 = bounds.size.w * ((curr&battman.time_mask) - min_t) / dT;
          int x2 = bounds.size.w * ((next&battman.time_mask) - min_t) / dT;
          graphics_draw_line(ctx, GPoint(x1, bounds.size.h - y1), GPoint(x2, bounds.size.h - y2));
          //APP_LOG (APP_LOG_LEVEL_INFO,"Drawing curve at: (%d, %d) (%d, %d)", x1, bounds.size.h - y1, x2, bounds.size.h - y2);
        }
      } 
    }
    // draw the projected discharge line, starting from (time_at_empty, 0%), up the left with a slope equal to discharge_rate
    // 3 pixels on, 2 pixels off, ending at the time of the last sample.
    min_t = battman.data[recent_peak_index]&battman.time_mask;
    //APP_LOG (APP_LOG_LEVEL_INFO,"Drawing projeceted line form end to %d", bounds.size.w * (int)((battman.data[BATTMAN_NUM_SAMPLES-1]&battman.time_mask) - min_t) / dT);
    for(i = bounds.size.w; i > (bounds.size.w * (int)((battman.data[BATTMAN_NUM_SAMPLES-1]&battman.time_mask) - min_t) / dT); i--){
      if(i%5 < 3){
        //max_t just used as an integer to store the y value of the pixel to be drawn, since it was already allocated and won't be used for anything else
        int seconds = (i - bounds.size.w) * dT / bounds.size.w;
       // APP_LOG (APP_LOG_LEVEL_INFO,"Seconds: %d", seconds);
        max_t = bounds.size.h * seconds / battman.discharge_rate / 100;
        graphics_draw_pixel(ctx, GPoint(i,bounds.size.h - max_t));
      //  APP_LOG (APP_LOG_LEVEL_INFO,"Drawing point at: (%d, %d)", i, bounds.size.h - max_t);
      }
    }
  }
  
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int nub_w = bounds.size.w/15;
  int nub_h = nub_w * 2 + 1;
  int fill_width = ((s_battery_level * (bounds.size.w - nub_w)) / 100);
  
  GColor8 mid_color = {((((settings.TextColor.argb & 0x2A) + (settings.BackgroundColor.argb & 0x2A) )>>1) + (settings.TextColor.argb & 0x15)) | 0xC0};
  //Draw filled battery background, with color that is average of text and background colors
  graphics_context_set_fill_color(ctx, mid_color);
  
  graphics_fill_rect(ctx, GRect(0,0, bounds.size.w-nub_w, bounds.size.h), nub_w, GCornersAll);
  
  graphics_fill_rect(ctx, GRect(bounds.size.w-nub_w, (bounds.size.h-nub_h)/2, nub_w, nub_h), nub_w, GCornersRight);
  
  //Draw filled battery level in text color
  graphics_context_set_fill_color(ctx, settings.TextColor);
  
  graphics_fill_rect(ctx, GRect(0,0, fill_width, bounds.size.h), nub_w, fill_width>=nub_w*2? GCornersAll: GCornersLeft);
  
  //Draw percentage number in center in background color
  graphics_context_set_text_color(ctx, settings.BackgroundColor);
  static char percent_buff[4];
  int percent_buff_size = sizeof(percent_buff);
  snprintf(percent_buff, percent_buff_size, "%d", s_battery_level);
  if (bounds.size.h < 20) { // Aplite, Basalt, Chalk, Diorite, Flint
    graphics_draw_text(ctx, percent_buff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(0,-4, bounds.size.w-nub_w, bounds.size.h),\
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, graphics_text_attributes_create());
  } else { // Emery, Gabbro
    graphics_draw_text(ctx, percent_buff, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(0,-6, bounds.size.w-nub_w, bounds.size.h),\
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, graphics_text_attributes_create());
  }
  
}

static void battery_update_proc_old(Layer *layer, GContext *ctx) {
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
  //if (!connected) {
  //  vibes_double_pulse();
  //}
}

// AppMessage received handler
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Check for Clay settings data
  Tuple *bg_color_t = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  if (bg_color_t) {
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
    #ifdef PBL_BW
    settings.TextColor = gcolor_equal(GColorBlack, settings.BackgroundColor) ? GColorWhite : GColorBlack;
    #endif
  }

  Tuple *text_color_t = dict_find(iterator, MESSAGE_KEY_TextColor);
  if (text_color_t) {
    settings.TextColor = GColorFromHEX(text_color_t->value->int32);
  }

  Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (show_date_t) {
    settings.ShowDate = show_date_t->value->int32 == 1;
  }

  // Save and apply if any settings were changed
  if (bg_color_t || text_color_t || show_date_t) {
    prv_save_settings();
    prv_update_display();
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
  // Hide things that may be obscured in the near future
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bounds = layer_get_bounds(s_window_layer);
  
  int y = bounds.size.h;
  
  //move things around if you need to

  GRect time_frame = layer_get_frame(text_layer_get_layer(s_time_layer));
//  time_frame.origin.y = time_y;
  layer_set_frame(text_layer_get_layer(s_time_layer), time_frame);

  GRect date_frame = layer_get_frame(text_layer_get_layer(s_date_layer));
//  date_frame.origin.y = date_y;
  layer_set_frame(text_layer_get_layer(s_date_layer), date_frame);
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bool obstructed = !grect_equal(&full_bounds, &bounds);
  
  // unhide things if they were hidden and obstructed is false
}

static void main_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  #ifdef PBL_PLATFORM_EMERY //Big Rectangle
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  #define date_time_padding 1
  #define stats_padding 1
  int time_y = 0;
  int date_y = 60;
  int batt_y = 192;
  int bar_y = 200;
  int bar_width = 46;
  int bar_height = 23;
  
  #elif PBL_PLATFORM_CHALK //Small Circle
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  #define date_time_padding 1
  #define stats_padding 15
  int time_y = 23  ;
  int date_y = 6;
  int batt_y = 130;
  int bar_y = 158;
  int bar_width = 34;
  int bar_height = 17;
  
  #elif PBL_PLATFORM_GABBRO //Big Circle
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  #define date_time_padding 1
  #define stats_padding 23
  int time_y = 32;
  int date_y = 10;
  int batt_y = 193;
  int bar_y = 227;
  int bar_width = 46;
  int bar_height = 23;
  
  #else //It's a small rectange (Aplite, Basalt, Diorite, or Flint)
  // Load custom fonts
  s_time_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  s_date_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_info_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  #define date_time_padding 1
  #define stats_padding 1
  int time_y = 0;
  int date_y = 42;
  int batt_y = 139;
  int bar_y = 147;
  int bar_width = 34;
  int bar_height = 17;
  #endif

  // Create the time TextLayer
  s_time_layer = text_layer_create(
      GRect(date_time_padding, time_y, bounds.size.w - (2*date_time_padding), 70));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, settings.TextColor);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the date TextLayer — just below the time
  s_date_layer = text_layer_create(
      GRect(date_time_padding, date_y, bounds.size.w - (2*date_time_padding), 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, settings.TextColor);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // Create battery data TextLayer — just below the date
  s_batt_data_layer = text_layer_create(
      GRect(date_time_padding, batt_y, bounds.size.w - (2*date_time_padding), 100));
  text_layer_set_background_color(s_batt_data_layer, GColorClear);
  text_layer_set_text_color(s_batt_data_layer, settings.TextColor);
  text_layer_set_font(s_batt_data_layer, s_info_font);
  text_layer_set_text_alignment(s_batt_data_layer, PBL_IF_RECT_ELSE (GTextAlignmentLeft, GTextAlignmentCenter));
  text_layer_set_text(s_batt_data_layer, "Collecting Battery Data");

  // Create battery meter Layer — visible bar near the top
  int bar_x = (bounds.size.w - bar_width - date_time_padding); //this right-aligns the battery bar
  #ifndef PBL_RECT
  bar_x = (bounds.size.w - bar_width) / 2; //this centers the battery bar
  #endif
  s_battery_layer = layer_create(GRect(bar_x, bar_y, bar_width, bar_height));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Create the battery stats Layer
  int stats_width = bounds.size.w - (2*stats_padding);
  int stats_height = PBL_IF_RECT_ELSE (stats_width / 2, 5 * stats_width / 11);
  int stats_y = PBL_IF_RECT_ELSE (bounds.size.h - stats_height - (bounds.size.h / 7) - 3 ,(bounds.size.h - stats_height) / 2 + (bounds.size.h / 13));
  s_stats_layer = layer_create(GRect(stats_padding, stats_y, stats_width, stats_height));
  layer_set_update_proc(s_stats_layer, stats_update_proc);

  // Add layers to the Window
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_batt_data_layer));
  layer_add_child(s_window_layer, s_battery_layer);
  layer_add_child(s_window_layer, s_stats_layer);

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
  text_layer_destroy(s_batt_data_layer);
  //fonts_unload_custom_font(s_time_font);
  //fonts_unload_custom_font(s_date_font);
  layer_destroy(s_battery_layer);
  layer_destroy(s_stats_layer);
}

static void init() {
  // Load settings before creating UI
  prv_load_settings();
  
  battman.init();
  
  s_main_window = window_create();
  window_set_background_color(s_main_window, settings.BackgroundColor);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  update_time();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
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
  battman.deinit();
  
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
