#ifndef SIMDJSON_INTERNAL_LOGGER_H
#define SIMDJSON_INTERNAL_LOGGER_H

namespace simdjson {
namespace internal {
namespace logger {

static constexpr const bool LOG_ENABLED = false;
static constexpr const char * DASHES = "----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------";
static constexpr const int LOG_EVENT_LEN = 30;
static constexpr const int LOG_BUFFER_LEN = 20;
static constexpr const int LOG_DETAIL_LEN = 50;
static constexpr const int LOG_DEPTH_LEN = 5;

static int log_depth; // Not threadsafe. Log only.

namespace {

// Helper to turn unprintable or newline characters into spaces
static really_inline char printable_char(char c) {
  if (c >= 0x20) {
    return c;
  } else {
    return ' ';
  }
}

} // namespace {}

void log_start() {
  if (LOG_ENABLED) {
    log_depth = 0;
    printf("\n");
    printf("| %-*s | %-*s | %-*s | %-*s |\n", LOG_EVENT_LEN, "Event", LOG_BUFFER_LEN, "Buffer", LOG_DEPTH_LEN, "Depth", LOG_DETAIL_LEN, "Detail");
    printf("|%.*s|%.*s|%.*s|%.*s|\n", LOG_EVENT_LEN+2, DASHES, LOG_BUFFER_LEN+2, DASHES, LOG_DEPTH_LEN+2, DASHES, LOG_DETAIL_LEN+2, DASHES);
  }
}

static really_inline void log_string(const char *message) {
  if (LOG_ENABLED) {
    printf("%s\n", message);
  }
}

static really_inline void log_event(const char *event_prefix, const char *event, const uint8_t *buf, const int depth, const char *detail) {
  static_assert(LOG_BUFFER_LEN <= SIMDJSON_PADDING, "LOG_BUFFER_LEN must be smaller than SIMDJSON_PADDING!");
  if (LOG_ENABLED) {
    printf("| %*s%s%-*s ", log_depth*2, "", event_prefix, LOG_EVENT_LEN - log_depth*2 - int(strlen(event_prefix)), event);
    {
      // Print the next N characters in the buffer.
      printf("| ");
      // Otherwise, print the characters starting from the buffer position.
      // Print spaces for unprintable or newline characters.
      for (int i=0;i<LOG_BUFFER_LEN;i++) {
        printf("%c", printable_char(buf[i]));
      }
      printf(" ");
    }
    printf("| %-d ", depth);
    printf("| %s ", detail);
    printf("|\n");
  }
}

template<typename T>
static really_inline void log_event(const char *event_prefix, const char *event, T &json, const char *detail, bool prev=false) {
  log_event(event_prefix, event, prev ? json.peek_prev() : json.get(), json.depth, detail);
}

template<>
really_inline void log_event<const uint8_t * const>(const char *event_prefix, const char *event, const uint8_t * const&buf, const char *detail, bool) {
  log_event(event_prefix, event, buf, 0, detail);
}

template<typename T>
static really_inline void log_event(const char *event, const T &json, bool prev = false) {
  log_event("", event, json, "", prev);
}

template<typename T>
static really_inline void log_error(const char *event, const T &json, bool prev = false) {
  log_event("ERROR ", event, json, "", prev);
}

template<typename T>
static really_inline void log_start_event(const char *event, const T &json, bool prev = false) {
  log_event("start ", event, json, "", prev);
  log_depth++;
}

template<typename T>
static really_inline void log_end_event(const char *event, const T &json, bool prev = false) {
  log_depth--;
  log_event("end ", event, json, "", prev);
}

} // namespace logger
} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_LOGGER_H
