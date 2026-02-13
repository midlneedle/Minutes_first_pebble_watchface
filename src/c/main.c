#include <pebble.h>

#define SMALL_WIDTH 10
#define SMALL_HEIGHT 14
#define SMALL_SPACING 22
#define SMALL_MARGIN 12
#define SMALL_DIGIT_GAP 2

#define LARGE_WIDTH 20
#define LARGE_HEIGHT 28
#define LARGE_DIGIT_GAP 4

#define HOURS_RING_COUNT 12
#define INTRO_TIMER_MS 25
#define INTRO_START_DELAY_MS 140
#define INTRO_ROTATE_START_OFFSET (-3)
#define INTRO_ROTATE_STEP_COUNT 3
#define INTRO_ROTATE_MOVE_MS 380
#define INTRO_ROTATE_HOLD_MS 220
#define INTRO_BRIGHTEN_LIGHT_MS 140
#define INTRO_BRIGHTEN_WHITE_MS 120
#define Q16_ONE 65536

static Window *s_main_window;
static Layer *s_canvas_layer;
static int s_hour = 12;
static int s_minute = 0;
static AppTimer *s_intro_timer;

typedef enum {
  INTRO_PHASE_START_DELAY = 0,
  INTRO_PHASE_HOURS_ROTATE,
  INTRO_PHASE_BRIGHTEN_LIGHT,
  INTRO_PHASE_BRIGHTEN_WHITE,
  INTRO_PHASE_DONE
} IntroPhase;

static IntroPhase s_intro_phase = INTRO_PHASE_DONE;
static uint32_t s_intro_phase_elapsed_ms = 0;
static int32_t s_intro_rotation_offset_q16 = 0;

static const int HOUR_NUMBERS[HOURS_RING_COUNT] = {11, 12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

static const char *SMALL_DIGITS[10][SMALL_HEIGHT] = {
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "0011111100",
    "0011111100"
  },
  {
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000",
    "0000110000"
  },
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "0000000011",
    "0000000011",
    "0000001100",
    "0000001100",
    "0000110000",
    "0000110000",
    "0011000000",
    "0011000000",
    "1111111111",
    "1111111111"
  },
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "0000000011",
    "0000000011",
    "0000111100",
    "0000111100",
    "0000000011",
    "0000000011",
    "1100000011",
    "1100000011",
    "0011111100",
    "0011111100"
  },
  {
    "0000001111",
    "0000001111",
    "0000110011",
    "0000110011",
    "0011000011",
    "0011000011",
    "1100000011",
    "1100000011",
    "1111111111",
    "1111111111",
    "0000000011",
    "0000000011",
    "0000000011",
    "0000000011"
  },
  {
    "1111111111",
    "1111111111",
    "1100000000",
    "1100000000",
    "1100000000",
    "1100000000",
    "1111111100",
    "1111111100",
    "0000000011",
    "0000000011",
    "0000000011",
    "0000000011",
    "1111111100",
    "1111111100"
  },
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "1100000000",
    "1100000000",
    "1111111100",
    "1111111100",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "0011111100",
    "0011111100"
  },
  {
    "1111111111",
    "1111111111",
    "0000000011",
    "0000000011",
    "0000001100",
    "0000001100",
    "0000110000",
    "0000110000",
    "0011000000",
    "0011000000",
    "1100000000",
    "1100000000",
    "1100000000",
    "1100000000"
  },
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "0011111100",
    "0011111100"
  },
  {
    "0011111100",
    "0011111100",
    "1100000011",
    "1100000011",
    "1100000011",
    "1100000011",
    "0011111111",
    "0011111111",
    "0000000011",
    "0000000011",
    "0000001100",
    "0000001100",
    "0011110000",
    "0011110000"
  }
};

static const char *LARGE_DIGITS[10][LARGE_HEIGHT] = {
  {
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  },
  {
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000000111100000000"
  },
  {
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "00000000000000001111",
    "00000000000000001111",
    "00000000000000111111",
    "00000000000000111111",
    "00000000001111111100",
    "00000000001111111100",
    "00000011111111000000",
    "00000011111111000000",
    "00001111111100000000",
    "00001111111100000000",
    "00111111000000000000",
    "00111111000000000000",
    "00111100000000000000",
    "00111100000000000000",
    "11111100000000000000",
    "11111100000000000000",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111"
  },
  {
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "00000000000011111100",
    "00000000000011111100",
    "00000000001111110000",
    "00000000001111110000",
    "00000000111111000000",
    "00000000111111000000",
    "00000011111111110000",
    "00000011111111110000",
    "00000011111111111100",
    "00000011111111111100",
    "00000000000000111111",
    "00000000000000111111",
    "00000000000000001111",
    "00000000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  },
  {
    "00000000000011111100",
    "00000000000011111100",
    "00000000001111111100",
    "00000000001111111100",
    "00000000111111111100",
    "00000000111100111100",
    "00000011111100111100",
    "00000011110000111100",
    "00001111110000111100",
    "00001111000000111100",
    "00111111000000111100",
    "00111100000000111100",
    "11111100000000111100",
    "11110000000000111100",
    "11110000000000111100",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000000111100 "
  },
  {
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11110000000000000000",
    "11110000000000000000",
    "11110000000000000000",
    "11110000000000000000",
    "11110011111111110000",
    "11110011111111110000",
    "11111111111111111100",
    "11111100000011111100",
    "11111100000011111111",
    "11111000000000111111",
    "11111000000000111111",
    "00000000000000001111",
    "00000000000000001111",
    "00000000000000001111",
    "00000000000000001111",
    "00000000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  },
  {
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000000000",
    "11110000000000000000",
    "11110011111111110000",
    "11110011111111110000",
    "11111111111111111100",
    "11111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  },
  {
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "11111111111111111111",
    "00000000000000001111",
    "00000000000000001111",
    "00000000000000111111",
    "00000000000000111111",
    "00000000000000111100",
    "00000000000000111100",
    "00000000000011111100",
    "00000000000011111100",
    "00000000000011110000",
    "00000000000011110000",
    "00000000001111110000",
    "00000000001111110000",
    "00000000001111000000",
    "00000000001111000000",
    "00000000111111000000",
    "00000000111111000000",
    "00000000111100000000",
    "00000000111100000000",
    "00000011111100000000",
    "00000011111100000000",
    "00000011110000000000",
    "00000011110000000000",
    "00001111110000000000",
    "00001111110000000000"
  },
  {
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "00111100000000111100",
    "00111100000000111100",
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  },
  {
    "00001111111111110000",
    "00001111111111110000",
    "00111111111111111100",
    "00111111111111111100",
    "11111100000000111111",
    "11111100000000111111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111110000000111111",
    "11111110000000111111",
    "00111111111111111111",
    "00111111111111111111",
    "00001111111111001111",
    "00001111111111001111",
    "00000000000000001111",
    "00000000000000001111",
    "11110000000000001111",
    "11110000000000001111",
    "11111100000000111111",
    "11111100000000111111",
    "00111111111111111100",
    "00111111111111111100",
    "00001111111111110000",
    "00001111111111110000"
  }
};

static int positive_mod(int value, int divisor) {
  int result = value % divisor;
  return (result < 0) ? result + divisor : result;
}

static int32_t ease_in_out_cubic_q16(int32_t t_q16) {
  if(t_q16 <= 0) {
    return 0;
  }
  if(t_q16 >= Q16_ONE) {
    return Q16_ONE;
  }

  if(t_q16 < (Q16_ONE / 2)) {
    int64_t t = t_q16;
    int64_t denom = (int64_t)Q16_ONE * Q16_ONE;
    return (int32_t)((4LL * t * t * t) / denom);
  }

  int64_t inv_t = Q16_ONE - t_q16;
  int64_t denom = (int64_t)Q16_ONE * Q16_ONE;
  return (int32_t)(Q16_ONE - ((4LL * inv_t * inv_t * inv_t) / denom));
}

static GPoint lerp_point_q16(GPoint from, GPoint to, int32_t t_q16) {
  int32_t x = from.x + (int32_t)(((int64_t)(to.x - from.x) * t_q16) / Q16_ONE);
  int32_t y = from.y + (int32_t)(((int64_t)(to.y - from.y) * t_q16) / Q16_ONE);
  return GPoint((int16_t)x, (int16_t)y);
}

static void get_hours_ring_centers(GRect bounds, GPoint centers[HOURS_RING_COUNT]) {
  const int cx = bounds.size.w / 2;
  const int cy = bounds.size.h / 2;

  const int top_y = SMALL_MARGIN + SMALL_HEIGHT / 2;
  const int bottom_y = bounds.size.h - SMALL_MARGIN - SMALL_HEIGHT / 2;
  const int left_x = SMALL_MARGIN + SMALL_WIDTH / 2;
  const int right_x = bounds.size.w - SMALL_MARGIN - SMALL_WIDTH / 2;

  const int horizontal_offset = SMALL_WIDTH + SMALL_SPACING;
  const int vertical_offset = SMALL_HEIGHT + SMALL_SPACING;

  centers[0] = GPoint(cx - horizontal_offset, top_y);      // 11
  centers[1] = GPoint(cx, top_y);                          // 12
  centers[2] = GPoint(cx + horizontal_offset, top_y);      // 1
  centers[3] = GPoint(right_x, cy - vertical_offset);      // 2
  centers[4] = GPoint(right_x, cy);                        // 3
  centers[5] = GPoint(right_x, cy + vertical_offset);      // 4
  centers[6] = GPoint(cx + horizontal_offset, bottom_y);   // 5
  centers[7] = GPoint(cx, bottom_y);                       // 6
  centers[8] = GPoint(cx - horizontal_offset, bottom_y);   // 7
  centers[9] = GPoint(left_x, cy + vertical_offset);       // 8
  centers[10] = GPoint(left_x, cy);                        // 9
  centers[11] = GPoint(left_x, cy - vertical_offset);      // 10
}

static GPoint get_ring_position_q16(const GPoint centers[HOURS_RING_COUNT], int32_t ring_index_q16) {
  int32_t base_index = ring_index_q16 / Q16_ONE;
  int32_t frac_q16 = ring_index_q16 % Q16_ONE;
  if(frac_q16 < 0) {
    frac_q16 += Q16_ONE;
    base_index -= 1;
  }

  int idx0 = positive_mod(base_index, HOURS_RING_COUNT);
  int idx1 = (idx0 + 1) % HOURS_RING_COUNT;
  return lerp_point_q16(centers[idx0], centers[idx1], frac_q16);
}

static int32_t floor_q16_to_int(int32_t value_q16) {
  int32_t value = value_q16 / Q16_ONE;
  if(value_q16 < 0 && (value_q16 % Q16_ONE) != 0) {
    value -= 1;
  }
  return value;
}

static void stop_intro_animation(void) {
  if(s_intro_timer) {
    app_timer_cancel(s_intro_timer);
    s_intro_timer = NULL;
  }
}

static void intro_timer_callback(void *context) {
  (void)context;
  s_intro_timer = NULL;

  if(s_intro_phase == INTRO_PHASE_START_DELAY) {
    s_intro_phase_elapsed_ms += INTRO_TIMER_MS;
    if(s_intro_phase_elapsed_ms >= INTRO_START_DELAY_MS) {
      s_intro_phase = INTRO_PHASE_HOURS_ROTATE;
      s_intro_phase_elapsed_ms = 0;
      s_intro_rotation_offset_q16 = INTRO_ROTATE_START_OFFSET * Q16_ONE;
    }
  } else if(s_intro_phase == INTRO_PHASE_HOURS_ROTATE) {
    const uint32_t segment_total_ms = INTRO_ROTATE_MOVE_MS + INTRO_ROTATE_HOLD_MS;
    const int total_segments = INTRO_ROTATE_STEP_COUNT;
    const uint32_t total_rotate_ms = (uint32_t)total_segments * segment_total_ms;

    s_intro_phase_elapsed_ms += INTRO_TIMER_MS;

    if(s_intro_phase_elapsed_ms >= total_rotate_ms) {
      s_intro_rotation_offset_q16 = 0;
      s_intro_phase = INTRO_PHASE_BRIGHTEN_LIGHT;
      s_intro_phase_elapsed_ms = 0;
    } else {
      int segment_index = s_intro_phase_elapsed_ms / segment_total_ms;
      uint32_t segment_elapsed_ms = s_intro_phase_elapsed_ms % segment_total_ms;
      int start_slot = INTRO_ROTATE_START_OFFSET + segment_index;

      if(segment_elapsed_ms < INTRO_ROTATE_MOVE_MS) {
        int32_t t_q16 = (int32_t)(((int64_t)segment_elapsed_ms * Q16_ONE) / INTRO_ROTATE_MOVE_MS);
        int32_t eased_q16 = ease_in_out_cubic_q16(t_q16);
        s_intro_rotation_offset_q16 = start_slot * Q16_ONE + eased_q16;
      } else {
        s_intro_rotation_offset_q16 = (start_slot + 1) * Q16_ONE;
      }
    }
  } else if(s_intro_phase == INTRO_PHASE_BRIGHTEN_LIGHT) {
    s_intro_phase_elapsed_ms += INTRO_TIMER_MS;
    if(s_intro_phase_elapsed_ms >= INTRO_BRIGHTEN_LIGHT_MS) {
      s_intro_phase = INTRO_PHASE_BRIGHTEN_WHITE;
      s_intro_phase_elapsed_ms = 0;
    }
  } else if(s_intro_phase == INTRO_PHASE_BRIGHTEN_WHITE) {
    s_intro_phase_elapsed_ms += INTRO_TIMER_MS;
    if(s_intro_phase_elapsed_ms >= INTRO_BRIGHTEN_WHITE_MS) {
      s_intro_phase = INTRO_PHASE_DONE;
      s_intro_phase_elapsed_ms = 0;
    }
  }

  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }

  if(s_intro_phase != INTRO_PHASE_DONE) {
    s_intro_timer = app_timer_register(INTRO_TIMER_MS, intro_timer_callback, NULL);
  }
}

static void start_intro_animation(void) {
  stop_intro_animation();

  s_intro_phase = INTRO_PHASE_START_DELAY;
  s_intro_phase_elapsed_ms = 0;
  s_intro_rotation_offset_q16 = INTRO_ROTATE_START_OFFSET * Q16_ONE;

  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }

  s_intro_timer = app_timer_register(INTRO_TIMER_MS, intro_timer_callback, NULL);
}

static void draw_digit_bitmap(GContext *ctx, const char *rows[], int width, int height, GPoint origin, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  for(int y = 0; y < height; ++y) {
    const char *row = rows[y];
    for(int x = 0; x < width; ++x) {
      if(row[x] == '1') {
        graphics_fill_rect(ctx, GRect(origin.x + x, origin.y + y, 1, 1), 0, GCornerNone);
      }
    }
  }
}

static void get_small_digit_ink_bounds(int digit, int *out_min_x, int *out_max_x) {
  int min_x = SMALL_WIDTH - 1;
  int max_x = 0;
  bool found = false;

  for(int y = 0; y < SMALL_HEIGHT; ++y) {
    const char *row = SMALL_DIGITS[digit][y];
    for(int x = 0; x < SMALL_WIDTH; ++x) {
      if(row[x] == '1') {
        if(x < min_x) {
          min_x = x;
        }
        if(x > max_x) {
          max_x = x;
        }
        found = true;
      }
    }
  }

  if(!found) {
    min_x = 0;
    max_x = SMALL_WIDTH - 1;
  }

  *out_min_x = min_x;
  *out_max_x = max_x;
}

static void get_small_number_ink_bounds(const char *buffer, int len, int *out_min_x, int *out_max_x) {
  int min_x = SMALL_WIDTH * len;
  int max_x = 0;

  for(int i = 0; i < len; ++i) {
    int digit = buffer[i] - '0';
    int digit_min_x = 0;
    int digit_max_x = 0;
    int offset_x = i * (SMALL_WIDTH + SMALL_DIGIT_GAP);

    get_small_digit_ink_bounds(digit, &digit_min_x, &digit_max_x);

    if(offset_x + digit_min_x < min_x) {
      min_x = offset_x + digit_min_x;
    }
    if(offset_x + digit_max_x > max_x) {
      max_x = offset_x + digit_max_x;
    }
  }

  *out_min_x = min_x;
  *out_max_x = max_x;
}

static void draw_small_number(GContext *ctx, int value, GPoint center, GColor color) {
  char buffer[3];
  snprintf(buffer, sizeof(buffer), "%d", value);
  int len = strlen(buffer);
  int ink_min_x = 0;
  int ink_max_x = 0;
  get_small_number_ink_bounds(buffer, len, &ink_min_x, &ink_max_x);
  int ink_width = ink_max_x - ink_min_x + 1;
  int start_x = center.x - ink_width / 2 - ink_min_x;
  int start_y = center.y - SMALL_HEIGHT / 2;

  for(int i = 0; i < len; ++i) {
    int digit = buffer[i] - '0';
    GPoint origin = GPoint(start_x + i * (SMALL_WIDTH + SMALL_DIGIT_GAP), start_y);
    draw_digit_bitmap(ctx, SMALL_DIGITS[digit], SMALL_WIDTH, SMALL_HEIGHT, origin, color);
  }
}

static void draw_minutes(GContext *ctx, GRect bounds, int left_digit, int right_digit, GColor color) {
  int total_width = 2 * LARGE_WIDTH + LARGE_DIGIT_GAP;
  int start_x = (bounds.size.w - total_width) / 2;
  int start_y = (bounds.size.h - LARGE_HEIGHT) / 2;

  GPoint left_origin = GPoint(start_x, start_y);
  GPoint right_origin = GPoint(start_x + LARGE_WIDTH + LARGE_DIGIT_GAP, start_y);
  draw_digit_bitmap(ctx, LARGE_DIGITS[left_digit], LARGE_WIDTH, LARGE_HEIGHT, left_origin, color);
  draw_digit_bitmap(ctx, LARGE_DIGITS[right_digit], LARGE_WIDTH, LARGE_HEIGHT, right_origin, color);
}

static void draw_hours_ring(GContext *ctx, GRect bounds) {
  GPoint centers[HOURS_RING_COUNT];
  get_hours_ring_centers(bounds, centers);

  const GColor active = GColorWhite;
  const GColor inactive = PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorLightGray);
  const GColor light_active = PBL_IF_COLOR_ELSE(GColorFromRGB(0xAA, 0xAA, 0xAA), GColorLightGray);
  int active_hour = s_hour;
  GColor active_color = active;

  if(s_intro_phase == INTRO_PHASE_START_DELAY || s_intro_phase == INTRO_PHASE_HOURS_ROTATE) {
    active_hour = -1;
  } else if(s_intro_phase == INTRO_PHASE_BRIGHTEN_LIGHT) {
    active_color = light_active;
  }

  for(int i = 0; i < HOURS_RING_COUNT; ++i) {
    GPoint center = centers[i];
    if(s_intro_phase == INTRO_PHASE_START_DELAY || s_intro_phase == INTRO_PHASE_HOURS_ROTATE) {
      int32_t ring_index_q16 = i * Q16_ONE + s_intro_rotation_offset_q16;
      center = get_ring_position_q16(centers, ring_index_q16);
    }

    int number = HOUR_NUMBERS[i];
    GColor color = (number == active_hour) ? active_color : inactive;
    draw_small_number(ctx, number, center, color);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_hours_ring(ctx, bounds);

  const GColor inactive = PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorLightGray);
  const GColor light_active = PBL_IF_COLOR_ELSE(GColorFromRGB(0xAA, 0xAA, 0xAA), GColorLightGray);
  int left_digit = s_minute / 10;
  int right_digit = s_minute % 10;
  GColor minutes_color = GColorWhite;

  if(s_intro_phase == INTRO_PHASE_START_DELAY || s_intro_phase == INTRO_PHASE_HOURS_ROTATE) {
    int shift = (int)floor_q16_to_int(s_intro_rotation_offset_q16);
    right_digit = positive_mod(right_digit + shift, 10);
    minutes_color = inactive;
  } else if(s_intro_phase == INTRO_PHASE_BRIGHTEN_LIGHT) {
    minutes_color = light_active;
  }

  draw_minutes(ctx, bounds, left_digit, right_digit, minutes_color);
}

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  int hour = tick_time->tm_hour % 12;
  if(hour == 0) {
    hour = 12;
  }

  s_hour = hour;
  s_minute = tick_time->tm_min;

  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(units_changed & MINUTE_UNIT) {
    update_time();
  }
}

static void main_window_load(Window *window) {
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  start_intro_animation();
}

static void main_window_unload(Window *window) {
  stop_intro_animation();
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init(void) {
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time();

  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  stop_intro_animation();
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
