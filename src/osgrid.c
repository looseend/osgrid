#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;

// static TextLayer *s_lat_layer;
// static TextLayer *s_long_layer;
static TextLayer *s_acc_layer;
static TextLayer *s_grid_layer;
static TextLayer *s_update_layer;
// static char longitude[15];
// static char latitude[15];
static char accuracy[15];
static char gridRef[15];
static char error[15];
static bool dataInit;
static bool wasFirstMsg;
static char updateTime[] = "00:00:00";

static bool messageProcessing;

static uint8_t dictBuf[512];

enum {
    UPDATE_LOCATION = 0x0,
};

static bool getLocation() {
    if (! dataInit) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Not getting location, not been initted");
        return false;
    } else if (messageProcessing) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Not getting location, message already processing");
        return false;
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "getLocation");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter");
        return false;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "make tuple");
    dict_write_begin(iter, dictBuf, 512);
    dict_write_cstring(iter, UPDATE_LOCATION, "location");
    dict_write_end(iter);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "sendMessage");

    messageProcessing = true;
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "sent!");
    text_layer_set_text(s_update_layer, "Updating");
    return true;
}

static void select_button_clicked(ClickRecognizerRef recognizer, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Clicked");
    getLocation();
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_button_clicked);
}

static void tap_received_handler(AccelAxisType axis, int32_t direction) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tapped");
    getLocation();
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *init_tpl = dict_find(iter, 5);
    if (init_tpl) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "received init");
        dataInit = true;
        text_layer_set_text(s_update_layer, "Ready");
        return;
    }

    // Tuple *latitude_tpl = dict_find(iter, 2);
    // Tuple *longitude_tpl = dict_find(iter, 1);
    Tuple *grid_tpl = dict_find(iter, 3);
    Tuple *error_tpl = dict_find(iter, 4);
    Tuple *acc_tpl = dict_find(iter, 6);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "gotMessage");
    
    // set to true regardless of result, it just means JS is awake
    dataInit = true;
    messageProcessing = false;
    if (error_tpl) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Error");
        char *e = error_tpl->value->cstring;
        strncpy(error, e, 15);
        text_layer_set_text(s_acc_layer, error);
        
    } else if (acc_tpl && grid_tpl) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "OK");

        snprintf(accuracy, 15, "Accuracy %sm", acc_tpl->value->cstring);
        text_layer_set_text(s_acc_layer, accuracy);

        strncpy(gridRef, grid_tpl->value->cstring, 15);
        text_layer_set_text(s_grid_layer, gridRef);
        
        time_t temp = time(NULL); 
        struct tm *tick_time = localtime(&temp);
        strftime(updateTime, sizeof("00:00:00"), "%H:%M:%S", tick_time);
        text_layer_set_text(s_update_layer, updateTime);
        light_enable_interaction();
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed");
        //APP_LOG(APP_LOG_LEVEL_DEBUG, dict_read_first(iter)->value->cstring);
    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped ");
    switch (reason) {
    case APP_MSG_OK : APP_LOG(APP_LOG_LEVEL_DEBUG, "OK"); break;
    case APP_MSG_SEND_TIMEOUT : APP_LOG(APP_LOG_LEVEL_DEBUG, "Timeout"); break;
    case APP_MSG_SEND_REJECTED : APP_LOG(APP_LOG_LEVEL_DEBUG, "rejected"); break;
    case APP_MSG_NOT_CONNECTED : APP_LOG(APP_LOG_LEVEL_DEBUG, "not connected"); break;
    case APP_MSG_APP_NOT_RUNNING : APP_LOG(APP_LOG_LEVEL_DEBUG, "not running"); break;
    case APP_MSG_INVALID_ARGS : APP_LOG(APP_LOG_LEVEL_DEBUG, "invalid args"); break;
    case APP_MSG_BUSY : APP_LOG(APP_LOG_LEVEL_DEBUG, "busy"); break;
    case APP_MSG_BUFFER_OVERFLOW : APP_LOG(APP_LOG_LEVEL_DEBUG, "overflow"); break;
    case APP_MSG_ALREADY_RELEASED : APP_LOG(APP_LOG_LEVEL_DEBUG, "released"); break;
    case APP_MSG_CALLBACK_ALREADY_REGISTERED : APP_LOG(APP_LOG_LEVEL_DEBUG, "already registered"); break;
    case APP_MSG_CALLBACK_NOT_REGISTERED : APP_LOG(APP_LOG_LEVEL_DEBUG, "not registered"); break;
    case APP_MSG_OUT_OF_MEMORY : APP_LOG(APP_LOG_LEVEL_DEBUG, "out of memoery"); break;
    case APP_MSG_CLOSED : APP_LOG(APP_LOG_LEVEL_DEBUG, "closed"); break;
    case APP_MSG_INTERNAL_ERROR : APP_LOG(APP_LOG_LEVEL_DEBUG, "internal"); break;
    }
    messageProcessing = false;
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    if (wasFirstMsg && dataInit) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "First App Message Failed to Send!");
    }
    else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
    }
    wasFirstMsg = false;
    messageProcessing  = false;
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);

    // Create a long-lived buffer
    static char buffer[] = "00:00";

    // Write the current hours and minutes into the buffer
    if(clock_is_24h_style() == true) {
        //Use 2h hour format
        strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        //Use 12 hour format
        strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, buffer);
  
}

static void main_window_load(Window *window) {
    // Create time TextLayer
    s_time_layer = text_layer_create(GRect(0, 0, 144, 47));
#ifdef PBL_COLOR
    text_layer_set_background_color(s_time_layer, GColorDarkGreen);
#else
    text_layer_set_background_color(s_time_layer, GColorBlack);
#endif
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_text(s_time_layer, "00:00");

    // Improve the layout to be more like a watchface
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

    // grid
    s_grid_layer = text_layer_create(GRect(0, 62, 144, 42));
    text_layer_set_background_color(s_grid_layer, GColorWhite);
    text_layer_set_text_color(s_grid_layer, GColorBlack);
    text_layer_set_font(s_grid_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(s_grid_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_grid_layer));
    text_layer_set_text(s_grid_layer, "Click to Update");

    // Accuracy
    s_acc_layer = text_layer_create(GRect(0, 110, 144, 28));
    text_layer_set_background_color(s_acc_layer, GColorWhite);
    text_layer_set_text_color(s_acc_layer, GColorBlack);
    text_layer_set_font(s_acc_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_text_alignment(s_acc_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_acc_layer));
    
    // updated
    s_update_layer = text_layer_create(GRect(0, 135, 144, 30));
    text_layer_set_background_color(s_update_layer, GColorWhite);
    text_layer_set_text_color(s_update_layer, GColorBlack);
    text_layer_set_font(s_update_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(s_update_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_update_layer));
    text_layer_set_text(s_update_layer, "Connecting");

    wasFirstMsg = true;
    dataInit = false;
    messageProcessing = false;
    // Make sure the time is displayed from the start
    update_time();
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    //    text_layer_destroy(s_long_layer);
    // text_layer_destroy(s_lat_layer);
    text_layer_destroy(s_acc_layer);
    text_layer_destroy(s_grid_layer);
    text_layer_destroy(s_update_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}
  
static void init() {

    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    app_message_open(128, 64);

    // Not using shake for now
    // accel_tap_service_subscribe(tap_received_handler);
  
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    window_set_click_config_provider(s_main_window, click_config_provider);

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = main_window_load,
                .unload = main_window_unload
                });

    // set window full screen
    #ifdef PBL_COLOR
    #else
    window_set_fullscreen(s_main_window, true);
    #endif
    
    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);
    // Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
}




int main(void) {
    init();
    app_event_loop();
    deinit();
}
