typedef struct payload_args {
  void* (*sceKernelDlsym)(int fd, const char* sym, void* addr);
  int *rwpipe;
  int *rwpair;
  long kpipe_addr;
  long kdata_base_addr;
  int *payloadout;
} payload_args_t;

typedef struct notify_request {
  char useless1[45];
  char message[3075];
} notify_request_t;

int (*printf)(const char *, ...);
int (*getpid)();
int (*sceKernelSendNotificationRequest)(int, notify_request_t*, unsigned long, int);

void* (*memset)(void *, int, unsigned long);
void* (*strncpy)(char *, const char *, unsigned long);

int _start(payload_args_t *args) {
  int pid;

  args->sceKernelDlsym(0x2001, "getpid", &getpid);
  args->sceKernelDlsym(0x2, "printf", &printf);
  args->sceKernelDlsym(0x2, "memset", &memset);
  args->sceKernelDlsym(0x2, "strncpy", &strncpy);
  if (args->sceKernelDlsym(0x2001, "sceKernelSendNotificationRequest", &sceKernelSendNotificationRequest) != 0) {
    printf("Failed to load sceKernelSendNotificationRequest\n");
    return 1;
  }

  pid = getpid();
  printf("PID: %d\n", pid);

  notify_request_t req;
  memset(&req, 0, sizeof(req));
  strncpy(req.message, "getpid Payload: PID = ", sizeof(req.message));
  char pid_str[16];
  printf("Converting PID %d to string\n", pid);
  int i = 0;
  if (pid == 0) {
    pid_str[i++] = '0';
  } else {
    while (pid > 0 && i < sizeof(pid_str) - 1) {
      pid_str[i++] = '0' + (pid % 10);
      pid /= 10;
    }
  }
  pid_str[i] = '\0';

  for (int j = 0; j < i / 2; j++) {
    char temp = pid_str[j];
    pid_str[j] = pid_str[i - 1 - j];
    pid_str[i - 1 - j] = temp;
  }

  int msg_len = 0;
  while (req.message[msg_len] != '\0') msg_len++;
  strncpy(req.message + msg_len, pid_str, sizeof(req.message) - msg_len - 1);

  printf("Sending notification: %s\n", req.message);
  int result = sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
  if (result != 0) {
    printf("sceKernelSendNotificationRequest failed: %d\n", result);
  } else {
    printf("Notification sent successfully\n");
  }

  return pid;
}