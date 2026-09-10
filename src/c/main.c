#include <pebble.h>
#include <stdlib.h>

enum {
  ORIENTATION_TOP_RIGHT = 0,
  ORIENTATION_TOP_LEFT,
  ORIENTATION_BOTTOM_RIGHT,
  ORIENTATION_BOTTOM_LEFT,
  ORIENTATION_COUNT
};

enum {
  PERSIST_ORIENTATION = 8100,
  PERSIST_BACKGROUND_COLOR,
  PERSIST_LINE_COLOR,
  PERSIST_TEXT_COLOR,
  PERSIST_FONT_STYLE,
  PERSIST_SETTINGS_VERSION
};

static Window *s_window;
static Layer *s_face_layer;
static GBitmap *s_backgrounds[ORIENTATION_COUNT];
static GFont s_time_light_font;
static GFont s_date_light_font;
static GFont s_time_bold_font;
static GFont s_date_bold_font;

static int s_orientation = ORIENTATION_TOP_RIGHT;
static int s_font_style = 0;
static GColor s_background_color;
static GColor s_line_color;
static GColor s_text_color;

static int tuple_to_int(const Tuple *tuple) {
  if (!tuple) return 0;
  return tuple->type == TUPLE_CSTRING
    ? atoi(tuple->value->cstring)
    : (int)tuple->value->int32;
}

static GColor web_color_to_gcolor(int value) {
  const uint32_t rgb = (uint32_t)value;
  return GColorFromRGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

static void recolor_bitmap(GBitmap *bitmap) {
  GColor *palette = gbitmap_get_palette(bitmap);
  if (!palette) return;
  palette[0] = s_background_color;
  palette[1] = s_line_color;
}

static void recolor_backgrounds(void) {
  for (int i = 0; i < ORIENTATION_COUNT; ++i) {
    recolor_bitmap(s_backgrounds[i]);
  }
}

static GFont time_font(void) {
  switch (s_font_style) {
    case 1: return s_time_bold_font;
    case 2: return fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM);
    case 3: return fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
    default: return s_time_light_font;
  }
}

static GFont date_font(void) {
  switch (s_font_style) {
    case 0: return s_date_light_font;
    case 1: return s_date_bold_font;
    case 2: return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    case 3: return fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
    default: return s_date_light_font;
  }
}

static void face_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  const bool left = s_orientation == ORIENTATION_TOP_LEFT ||
                    s_orientation == ORIENTATION_BOTTOM_LEFT;
  const bool bottom = s_orientation == ORIENTATION_BOTTOM_RIGHT ||
                      s_orientation == ORIENTATION_BOTTOM_LEFT;
  const int date_y = bottom ? 132 : 10;
  const int time_y = bottom ? 153 : 31;
  const GTextAlignment alignment = left ? GTextAlignmentLeft : GTextAlignmentRight;

  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_draw_bitmap_in_rect(ctx, s_backgrounds[s_orientation], bounds);

  graphics_context_set_text_color(ctx, s_text_color);

  time_t now = time(NULL);
  struct tm *time_now = localtime(&now);
  char date_buffer[16];
  char time_buffer[8];
  strftime(date_buffer, sizeof(date_buffer), "%a %d", time_now);
  for (char *c = date_buffer; *c; ++c) {
    if (*c >= 'a' && *c <= 'z') *c -= ('a' - 'A');
  }
  strftime(time_buffer, sizeof(time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", time_now);
  if (!clock_is_24h_style() && time_buffer[0] == '0') {
    memmove(time_buffer, time_buffer + 1, strlen(time_buffer));
  }

  graphics_draw_text(ctx, date_buffer, date_font(),
                     GRect(left ? 9 : 51, date_y, 135, 29),
                     GTextOverflowModeTrailingEllipsis, alignment, NULL);
  graphics_draw_text(ctx, time_buffer, time_font(),
                     GRect(left ? 7 : 39, time_y, 149, 65),
                     GTextOverflowModeTrailingEllipsis, alignment, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_face_layer);
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *orientation = dict_find(iterator, MESSAGE_KEY_Orientation);
  Tuple *background = dict_find(iterator, MESSAGE_KEY_BackgroundColor);
  Tuple *line = dict_find(iterator, MESSAGE_KEY_LineColor);
  Tuple *text = dict_find(iterator, MESSAGE_KEY_TextColor);
  Tuple *font = dict_find(iterator, MESSAGE_KEY_FontStyle);

  if (orientation) {
    s_orientation = tuple_to_int(orientation);
    if (s_orientation < 0 || s_orientation >= ORIENTATION_COUNT)
      s_orientation = ORIENTATION_TOP_RIGHT;
    persist_write_int(PERSIST_ORIENTATION, s_orientation);
  }
  if (background) {
    s_background_color = web_color_to_gcolor(tuple_to_int(background));
    persist_write_int(PERSIST_BACKGROUND_COLOR, s_background_color.argb);
  }
  if (line) {
    s_line_color = web_color_to_gcolor(tuple_to_int(line));
    persist_write_int(PERSIST_LINE_COLOR, s_line_color.argb);
  }
  if (text) {
    s_text_color = web_color_to_gcolor(tuple_to_int(text));
    persist_write_int(PERSIST_TEXT_COLOR, s_text_color.argb);
  }
  if (font) {
    s_font_style = tuple_to_int(font);
    if (s_font_style < 0 || s_font_style > 3) s_font_style = 0;
    persist_write_int(PERSIST_FONT_STYLE, s_font_style);
  }
  persist_write_int(PERSIST_SETTINGS_VERSION, 2);

  recolor_backgrounds();
  layer_mark_dirty(s_face_layer);
}

static void load_settings(void) {
  s_background_color = GColorBlack;
  s_line_color = GColorLightGray;
  s_text_color = GColorWhite;

  if (persist_exists(PERSIST_ORIENTATION))
    s_orientation = persist_read_int(PERSIST_ORIENTATION);
  if (persist_exists(PERSIST_FONT_STYLE))
    s_font_style = persist_read_int(PERSIST_FONT_STYLE);
  // Version 1 accidentally stored 24-bit web colors as 8-bit Pebble colors.
  // Ignore those old color values so an upgrade cannot retain transparent black.
  if (persist_read_int(PERSIST_SETTINGS_VERSION) == 2) {
    if (persist_exists(PERSIST_BACKGROUND_COLOR))
      s_background_color.argb = persist_read_int(PERSIST_BACKGROUND_COLOR);
    if (persist_exists(PERSIST_LINE_COLOR))
      s_line_color.argb = persist_read_int(PERSIST_LINE_COLOR);
    if (persist_exists(PERSIST_TEXT_COLOR))
      s_text_color.argb = persist_read_int(PERSIST_TEXT_COLOR);
  }

  if (s_orientation < 0 || s_orientation >= ORIENTATION_COUNT)
    s_orientation = ORIENTATION_TOP_RIGHT;
  if (s_font_style < 0 || s_font_style > 3)
    s_font_style = 0;
}

static void window_load(Window *window) {
  const uint32_t resource_ids[ORIENTATION_COUNT] = {
    RESOURCE_ID_TOPO_TOP_RIGHT,
    RESOURCE_ID_TOPO_TOP_LEFT,
    RESOURCE_ID_TOPO_BOTTOM_RIGHT,
    RESOURCE_ID_TOPO_BOTTOM_LEFT
  };
  for (int i = 0; i < ORIENTATION_COUNT; ++i) {
    s_backgrounds[i] = gbitmap_create_with_resource(resource_ids[i]);
  }
  s_time_light_font = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_TIME_46));
  s_date_light_font = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_DATE_21));
  s_time_bold_font = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_TIME_46));
  s_date_bold_font = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_DATE_21));
  recolor_backgrounds();

  Layer *root = window_get_root_layer(window);
  s_face_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_face_layer, face_update_proc);
  layer_add_child(root, s_face_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_face_layer);
  fonts_unload_custom_font(s_time_light_font);
  fonts_unload_custom_font(s_date_light_font);
  fonts_unload_custom_font(s_time_bold_font);
  fonts_unload_custom_font(s_date_bold_font);
  for (int i = 0; i < ORIENTATION_COUNT; ++i) {
    gbitmap_destroy(s_backgrounds[i]);
  }
}

static void init(void) {
  load_settings();
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, false);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 64);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
