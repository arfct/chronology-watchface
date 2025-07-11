#include <pebble.h>

// PDC (Pebble Draw Command) structures
typedef struct
{
  int16_t x;
  int16_t y;
} __attribute__((packed)) Point;

typedef struct
{
  uint8_t type;
  uint8_t flags;
  uint8_t stroke_color;
  uint8_t stroke_width;
  uint8_t fill_color;
  uint16_t path_open_radius;
  uint16_t num_points;
  Point points[];
} __attribute__((packed)) PebbleDrawCommand;

typedef struct
{
  uint16_t num_commands;
  PebbleDrawCommand commands[];
} __attribute__((packed)) PebbleDrawCommandList;

typedef struct
{
  uint16_t width;
  uint16_t height;
} __attribute__((packed)) ViewBox;

typedef struct
{
  uint8_t version;
  uint8_t reserved;
  ViewBox view_box;
  PebbleDrawCommandList command_list;
} __attribute__((packed)) PebbleDrawCommandImage;

typedef struct
{
  char magic[4];
  PebbleDrawCommandImage image;
} __attribute__((packed)) PebbleDrawCommandImageFile;

// Function to create a simple PDC image in memory
PebbleDrawCommandImageFile *create_pdc_image(uint16_t width, uint16_t height, uint16_t num_commands)
{
  size_t total_size = sizeof(PebbleDrawCommandImageFile) + (num_commands * sizeof(PebbleDrawCommand));
  PebbleDrawCommandImageFile *pdc = malloc(total_size);
  if (!pdc)
    return NULL;

  memcpy(pdc->magic, "PDCI", 4);
  pdc->image_size = total_size - 8;
  pdc->image.version = 1;
  pdc->image.reserved = 0;
  pdc->image.view_box.width = width;
  pdc->image.view_box.height = height;
  pdc->image.command_list.num_commands = num_commands;

  return pdc;
}

// Function to add points to a PDC draw command
bool add_points_to_command(PebbleDrawCommand *command, const Point *points, uint16_t num_points)
{
  if (!command || !points || num_points == 0)
    return false;

  command->num_points = num_points;
  memcpy(command->points, points, num_points * sizeof(Point));

  return true;
}

static Window *s_main_window;
static Layer *s_face_layer;
static Layer *s_hand_layer;
static TextLayer *s_battery_layer;
static bool debug = false;
static bool inverted = true;

static void click_config_provider(void *context);

static void update_time()
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
}
static void battery_handler(BatteryChargeState charge_state)
{
  static char s_battery_buffer[16];

  if (charge_state.is_charging)
  {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d", charge_state.charge_percent);
  }
  else
  {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_frame_location()
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int bhour = tick_time->tm_hour;
  int bmin = tick_time->tm_min;
  float angle = 30 * ((float)(bhour % 12) + ((float)bmin / 60));
  if (debug)
    angle = 12 * tick_time->tm_sec;

  GRect frame = layer_get_frame(s_face_layer);
  GRect frame2 = layer_get_frame(s_hand_layer);

  GPoint origin = gpoint_from_polar(grect_inset(frame2, GEdgeInsets(-150)), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle + 180));
  frame.origin = origin;
  frame.origin.x -= frame.size.w / 2;
  frame.origin.y -= frame.size.h / 2;

  layer_set_frame(s_face_layer, frame);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
  layer_mark_dirty(s_hand_layer);
  update_frame_location();
}

static void my_hand_draw(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);
  GRect face_frame = layer_get_frame(s_face_layer);

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int bhour = tick_time->tm_hour;
  int bmin = tick_time->tm_min;

  float angle = 30 * ((float)(bhour % 12) + ((float)bmin / 60));
  if (debug)
    angle = 12 * tick_time->tm_sec;

  graphics_context_set_fill_color(ctx, PBL_IF_BW_ELSE(inverted ? GColorDarkGray : GColorLightGray, GColorRed));

  GPoint center = GPoint(face_frame.origin.x + face_frame.size.w / 2, face_frame.origin.y + face_frame.size.h / 2);
  GPoint end_point = gpoint_from_polar(face_frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle));

  int32_t hour_angle = angle;
  int32_t perp_angle = DEG_TO_TRIGANGLE(angle);
  int32_t perp_thickness = 3;

  GPoint offset = {
      .x = (int16_t)(perp_thickness * cos_lookup(perp_angle) / TRIG_MAX_RATIO),
      .y = (int16_t)(perp_thickness * sin_lookup(perp_angle) / TRIG_MAX_RATIO)};

  GPoint hand_points[4] = {
      {center.x - offset.x, center.y - offset.y},
      {center.x + offset.x, center.y + offset.y},
      {end_point.x + offset.x, end_point.y + offset.y},
      {end_point.x - offset.x, end_point.y - offset.y}};

  GPath *hand_path = gpath_create(&(GPathInfo){
      .num_points = 4,
      .points = hand_points});

  gpath_draw_filled(ctx, hand_path);
  gpath_destroy(hand_path);
}

static void my_face_draw(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);

  // Draw a white filled circle a radius of half the layer height
  graphics_context_set_fill_color(ctx, inverted ? GColorBlack : GColorWhite);
  const int16_t half_h = bounds.size.h / 2;

  graphics_draw_circle(ctx, GPoint(half_h, half_h), 90);

  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_text_color(ctx, inverted ? GColorWhite : GColorBlack);

  for (int i = 0; i < 12; i++)
  {
    int angle = DEG_TO_TRIGANGLE(i * 30);

    static char buf[] = "000"; /* <-- implicit NUL-terminator at the end here */
    snprintf(buf, sizeof(buf), "%02d", i == 0 ? 12 : i);
    int ascender = 8;
    GPoint text_point = gpoint_from_polar(grect_crop(bounds, 50), GOvalScaleModeFitCircle, angle);
    GRect text_rect = GRect(text_point.x - 24, text_point.y - 24, 48, 48);

    GSize size = graphics_text_layout_get_content_size(buf,
                                                       fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                                                       text_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);

    /// graphics_draw_bitmap_in_rect(ctx, image, layer_get_bounds(layer));

    text_rect.size = size;
    text_rect.size.h -= ascender;
    text_rect.origin = GPoint(text_point.x - size.w / 2, text_point.y - size.h / 2);

    graphics_draw_text(ctx, buf,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       grect_inset(text_rect, GEdgeInsets4(-8, 0, 0, 0)),
                       GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

    // graphics_draw_rect(ctx, text_rect);

    //  Draw hour
    graphics_context_set_stroke_color(ctx, inverted ? GColorWhite : GColorBlack);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx,
                       gpoint_from_polar(grect_crop(bounds, 30), GOvalScaleModeFitCircle, angle),
                       gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle));

    // Draw all 5-minute markers in one loop
    for (int j = 1; j < 12; j++)
    {
      int16_t line_length;
      GColor line_color = inverted ? GColorLightGray : GColorDarkGray;

      if (j % 6 == 0)
      { // Half hour marks
        line_length = 20;
      }
      else if (j % 3 == 0)
      { // Quarter hour marks
        line_length = 10;
      }
      else
      { // 5-minute marks
        line_length = 3;
      }
      angle += DEG_TO_TRIGANGLE(2.5);

      graphics_context_set_stroke_color(ctx, line_color);
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_line(ctx,
                         gpoint_from_polar(grect_crop(bounds, line_length), GOvalScaleModeFitCircle, angle),
                         gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle));
    }
  }
}

static void main_window_load(Window *window)
{
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds

  s_face_layer = layer_create(GRect(0, 0, bounds.size.h * 3, bounds.size.h * 3));
  layer_set_update_proc(s_face_layer, my_face_draw);

  s_hand_layer = layer_create(bounds);
  layer_set_update_proc(s_hand_layer, my_hand_draw);

  // Create the TextLayer with specific bounds
  s_battery_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorDarkGray);
  text_layer_set_text(s_battery_layer, "50");
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  // layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  //   s_example_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_FACE);
  //   s_bitmap_layer = bitmap_layer_create(bounds);
  //   bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  //   bitmap_layer_set_bitmap(s_bitmap_layer, s_example_bitmap);
  //   bitmap_layer_set_alignment(s_bitmap_layer, GAlignCenter);
  //   layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  update_frame_location();

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, s_hand_layer);
  layer_add_child(window_layer, s_face_layer);

  window_set_click_config_provider(window, click_config_provider);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  inverted = !inverted;
  persist_write_bool(0, inverted);
  window_set_background_color(s_main_window, inverted ? GColorBlack : GColorWhite);
  layer_mark_dirty(s_face_layer);
  layer_mark_dirty(s_hand_layer);
}

static void click_config_provider(void *context)
{
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void main_window_unload(Window *window)
{
  // Destroy TextLayer
  layer_destroy(s_face_layer);
}

static void init()
{
  inverted = PBL_IF_BW_ELSE(true, false); // persist_read_bool(0)

  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, inverted ? GColorBlack : GColorWhite);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload});

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
}

static void deinit()
{
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}