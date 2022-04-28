#include <pebble.h>
#include "netdownload.h"

static Window *window;
static TextLayer *text_layer;
static BitmapLayer *bitmap_layer;
static GBitmap *current_bmp;
static TextLayer *s_time_layer; //the actual clock

//color images list
static char *colorImages[] = {
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/color/cande.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/color/republic.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/color/violator.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/color/wish.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/color/wms.PNG?raw=true"
};

//monochrome images list
static char *monoImages[] = {
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/mono/cande.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/mono/republic.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/mono/violator.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/mono/wish.PNG?raw=true",
  "https://github.com/JohnSpahr/CatCovers/blob/master/images/mono/wms.PNG?raw=true"
};

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}


static unsigned long image = 0;

void show_next_image() {
  // show that we are loading by showing no image
  bitmap_layer_set_bitmap(bitmap_layer, NULL);

  text_layer_set_text(text_layer, "Loading...");

  // Unload the current image if we had one and save a pointer to this one
  if (current_bmp) {
    gbitmap_destroy(current_bmp);
    current_bmp = NULL;
  }

//WORK-IN-PROGRESS PART BELOW:
  switch (PBL_PLATFORM_TYPE_CURRENT) {
    case PlatformTypeAplite:
      //monochrome pebbles
      netdownload_request(monoImages[image]); //request faces from internet
image++;
  if (image >= sizeof(monoImages)/sizeof(char*)) {
    image = 0;
  }
    break;
    case PlatformTypeBasalt:
      //color pebbles
      netdownload_request(colorImages[image]); //request faces from internet
image++;
  if (image >= sizeof(colorImages)/sizeof(char*)) {
    image = 0;
  }
    break;
  }

}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });

  text_layer_set_text(text_layer, "Shake to pick a cover!");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  bitmap_layer = bitmap_layer_create(bounds);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
  current_bmp = NULL;


  //create clock text layer...
  s_time_layer = text_layer_create(
      GRect(0, 138, bounds.size.w, bounds.size.h));

  //various setup stuff...
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  //add clock text layer to window layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));


}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  bitmap_layer_destroy(bitmap_layer);
  gbitmap_destroy(current_bmp);


  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
}

void download_complete_handler(NetDownload *download) {
  printf("Loaded image with %lu bytes", download->length);
  printf("Heap free is %u bytes", heap_bytes_free());

  GBitmap *bmp = gbitmap_create_from_png_data(download->data, download->length);
  bitmap_layer_set_bitmap(bitmap_layer, bmp);

  // Save pointer to currently shown bitmap (to free it)
  if (current_bmp) {
    gbitmap_destroy(current_bmp);
  }
  current_bmp = bmp;

  // Free the memory now
  free(download->data);

  // We null it out now to avoid a double free
  download->data = NULL;
  netdownload_destroy(download);
}

void tap_handler(AccelAxisType accel, int32_t direction) {
  show_next_image();
}

static void init(void) {
  // Need to initialize this first to make sure it is there when
  // the window_load function is called by window_stack_push.
  netdownload_initialize(download_complete_handler);

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  accel_tap_service_subscribe(tap_handler);


update_time();
  // Register with TickTimerService
tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);


}

static void deinit(void) {
  netdownload_deinitialize(); // call this to avoid 20B memory leak
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();


}