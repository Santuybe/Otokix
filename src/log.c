#include "otokix.h"

static FILE *_log_file = NULL;

int log_init() {
  // Try app folder first, fallback /tmp
  const char *log_path = "./otokix.log";
  _log_file = fopen(log_path, "a");
  if (!_log_file) {
    log_path = "/tmp/otokix.log";
    _log_file = fopen(log_path, "a");
  }

  if (!_log_file) return -1;

  setvbuf(_log_file, NULL, _IOLBF, 256);
  setvbuf(stdout, NULL, _IONBF, 0);

  char cwd[512];
  getcwd(cwd, sizeof(cwd));
  fprintf(_log_file, "=== OTOKIX START ===\ncwd: %s\nlog: %s\n", cwd, log_path);
  fflush(_log_file);

  return 0;
}

void log_close() {
  if (_log_file) {
    fprintf(_log_file, "=== OTOKIX END ===\n");
    fflush(_log_file);
    fclose(_log_file);
    _log_file = NULL;
  }
}

FILE *log_file() {
  return _log_file;
}
