#include <errno.h>
#include <stdbool.h>

#ifdef DARWIN
int setUid();
int elevate(char *path, char *prompt, char *iconPath);
#endif

extern const char* proxyHost;
extern const char* proxyPort;

#ifdef _WIN32
void setupSystemShutdownHandler();
#endif

int show();
int toggleProxy(bool turnOn);

enum RET_ERRORS {
  RET_NO_ERROR = 0,
  INVALID_FORMAT = 1,
  NO_PERMISSION = 2,
  SYSCALL_FAILED = 3,
  NO_MEMORY = 4
};
