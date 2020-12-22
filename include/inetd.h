#ifndef __INETD__H__
#define __INETD__H__


#define MAX_DEVICE_NUM      20
#define WORKING_DIRECTORY   "/home/gengyue"
#define INETD_CONFIG_FILE   "inetd.cfg"

#define APP_INETD_NAME      "cn.fmsoft.hybridos.inetd"
#define SOCKET_PATH         "/var/tmp/hibus.sock"


// runner name
#define RUNNER_WIFI_NAME    "wifi"

// method
#define METHOD_WIFI_SCAN    "wifi_method_scan"
#define METHOD_WIFI_CONNECT "wifi_method_connect"
#define METHOD_WIFI_DISCONN "wifi_method_disconnect"


// event
#define EVENT_WIFI_SIGNAL   "wifi_event_signal"
#define EVENT_WIFI_SCAN     "wifi_event_scan"
#define EVENT_WIFI_BROKEN   "wifi_event_broken"
#define EVENT_WIFI_CONNECT  "wifi_event_connect"
#define EVENT_WIFI_RESUME   "wifi_event_resume"

// for test
#define AGENT_NAME          "cn.fmsoft.hybridos.sysmgr"
#define AGENT_RUNNER_NAME   "gui"

typedef char * (* hibus_method_handler)(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);

typedef struct _HibusInvokeOps 
{
    int (* open_device) (int * fd_device);
    int (* close_device) (int fd_device);
    void (* device_read) (void);
    void (* wifi_scan) (void);
    void (* wifi_signal) (void);
    hibus_method_handler wifi_scan_handler;
    hibus_method_handler wifi_connect_handler;
    hibus_method_handler wifi_disconnect_handler;
} HibusInvokeOps;

#endif  // __INETD__H__
