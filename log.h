#define DEBUG 0 // change this to a 1 to enable logging

#if DEBUG
  #define LOG_MSG(...) Serial.print(__VA_ARGS__)
  #define LOG_MSG_CR(...) Serial.println(__VA_ARGS__)
#else
  #define LOG_MSG(...)
  #define LOG_MSG_CR(...)
#endif
