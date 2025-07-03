#include <pebble.h>

static Window *s_main_window;
static Layer *s_face_layer;
static Layer *s_hand_layer;
static TextLayer *s_battery_layer;
static bool debug = true;
static bool inverted = true;
static GDrawCommandImage *s_hand_command_image;

// PDC structure for in-memory hand
typedef struct
{
  uint8_t magic[4];      // "PDCI"
  uint32_t size;         // Total size
  uint8_t version;       // Version (1)
  uint8_t reserved;      // Reserved (0)
  uint16_t view_width;   // View box width
  uint16_t view_height;  // View box height
  uint16_t num_commands; // Number of commands (1)
  // Draw command data
  uint8_t type;              // Command type (1 = Path)
  uint8_t flags;             // Flags (0)
  uint8_t stroke_color;      // Stroke color
  uint8_t stroke_width;      // Stroke width
  uint8_t fill_color;        // Fill color
  uint16_t path_open_radius; // Path open (0 = closed)
  uint16_t num_points;       // Number of points (4)
  GPoint points[4];          // Points array
} __attribute__((packed)) HandPDC;

static void click_config_provider(void *context);

static uint8_t *create_hand_pdc_data()
{
  static HandPDC hand_pdc = {
      .magic = {'P', 'D', 'C', 'I'},
      .size = sizeof(HandPDC) - 8, // Size excludes magic and size fields
      .version = 1,
      .reserved = 0,
      .view_width = 200,
      .view_height = 200,
      .num_commands = 1,
      .type = 1, // GDrawCommandTypePath
      .flags = 0,
      .stroke_color = 0, // Will be set dynamically
      .stroke_width = 1,
      .fill_color = 0,       // Will be set dynamically
      .path_open_radius = 0, // Closed path
      .num_points = 4,
      .points = {{0, 0}, {0, 0}, {0, 0}, {0, 0}} // Will be set dynamically
  };

  return (uint8_t *)&hand_pdc;
}

static void update_hand_pdc_points(GPoint *points)
{
  static HandPDC *hand_pdc = NULL;
  if (!hand_pdc)
  {
    hand_pdc = (HandPDC *)create_hand_pdc_data();
  }

  // Update the points in our PDC structure
  for (int i = 0; i < 4; i++)
  {
    hand_pdc->points[i] = points[i];
  }

  // Update colors based on current theme
  hand_pdc->fill_color = PBL_IF_BW_ELSE(inverted ? GColorDarkGray : GColorLightGray, GColorRed);
}

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

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Points: %d, %d, %d, %d", hand_points[0].x, hand_points[0].y, hand_points[1].x, hand_points[1].y);

  // Update our in-memory PDC with current points and colors
  update_hand_pdc_points(hand_points);

  // Get the PDC data and try to use it as a command list
  HandPDC *pdc_data = (HandPDC *)create_hand_pdc_data();

  // Create a GDrawCommand from our PDC data
  GDrawCommand *hand_command = (GDrawCommand *)&(pdc_data->type);

  if (hand_command)
  {
    // Set current fill color
    gdraw_command_set_fill_color(hand_command, PBL_IF_BW_ELSE(inverted ? GColorDarkGray : GColorLightGray, GColorRed));

    // Update points
    uint16_t num_points = gdraw_command_get_num_points(hand_command);
    for (uint16_t i = 0; i < num_points && i < 4; i++)
    {
      gdraw_command_set_point(hand_command, i, hand_points[i]);
    }

    // Draw the command directly
    gdraw_command_draw(ctx, hand_command);
  }
  else
  {
    // Fallback to GPath if PDC approach fails
    graphics_context_set_fill_color(ctx, PBL_IF_BW_ELSE(inverted ? GColorDarkGray : GColorLightGray, GColorRed));
    graphics_context_set_antialiased(ctx, true);

    GPath *hand_path = gpath_create(&(GPathInfo){
        .num_points = 4,
        .points = hand_points});

    gpath_draw_filled(ctx, hand_path);
    gpath_destroy(hand_path);
  }
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
    int angle = i * 30;

    static char buf[] = "000"; /* <-- implicit NUL-terminator at the end here */
    snprintf(buf, sizeof(buf), "%02d", i == 0 ? 12 : i);
    int ascender = 8;
    GPoint text_point = gpoint_from_polar(grect_crop(bounds, 50), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle));
    GRect text_rect = GRect(text_point.x - 24, text_point.y - 24, 48, 48);

    GSize size = graphics_text_layout_get_content_size(buf,
                                                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
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
                       gpoint_from_polar(grect_crop(bounds, 30), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)),
                       gpoint_from_polar(bounds, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)));

    angle += 15;
    // Draw half hour

    graphics_context_set_stroke_color(ctx, inverted ? GColorLightGray : GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx,
                       gpoint_from_polar(grect_crop(bounds, 10), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)),
                       gpoint_from_polar(bounds, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)));

    angle += 7.5;
    // Draw quarter hours

    graphics_context_set_stroke_color(ctx, inverted ? GColorLightGray : GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx,
                       gpoint_from_polar(grect_crop(bounds, 10), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)),
                       gpoint_from_polar(bounds, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)));
    angle -= 15;
    graphics_draw_line(ctx,
                       gpoint_from_polar(grect_crop(bounds, 10), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)),
                       gpoint_from_polar(bounds, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle)));
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
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
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

  // Initialize in-memory PDC for hand drawing
  APP_LOG(APP_LOG_LEVEL_INFO, "Using in-memory PDC for hand rendering");

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
  // Clean up PDC resource
  if (s_hand_command_image)
  {
    gdraw_command_image_destroy(s_hand_command_image);
  }

  // Destroy Window
  window_destroy(s_main_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}