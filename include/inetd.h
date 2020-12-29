#ifndef __INETD__H__
#define __INETD__H__


#define MAX_DEVICE_NUM      20
#define WORKING_DIRECTORY   "/home/gengyue"
#define INETD_CONFIG_FILE   "inetd.cfg"

#define APP_INETD_NAME      "cn.fmsoft.hybridos.inetd"
#define SOCKET_PATH         "/var/tmp/hibus.sock"
#define RUNNER_WIFI_NAME    "wifi"

// method
#define METHOD_WIFI_OPEN_DEVICE         "openDevice"
#define METHOD_WIFI_CLOSE_DEVICE        "closeDevice"
#define METHOD_WIFI_GET_DEVICES_STATUS  "getNetworkDevicesStatus"
#define METHOD_WIFI_START_SCAN          "wifiStartScanHotspots"
#define METHOD_WIFI_STOP_SCAN           "wifiStopScanHotspots"
#define METHOD_WIFI_CONNECT_AP          "wifiConnect"
#define METHOD_WIFI_DISCONNECT_AP       "wifiDisconnect"
#define METHOD_WIFI_GET_NETWORK_INFO    "wifiGetNetworkInfo"

// event
#define NETWORKDEVICECHANGED    "NETWORKDEVICECHANGED"
#define WIFINEWHOTSPOTS         "WIFINEWHOTSPOTS"
#define WIFISIGNALSTRENGTHCHANGED   "WIFISIGNALSTRENGTHCHANGED"

// for test
#define AGENT_NAME          "cn.fmsoft.hybridos.sysmgr"
#define AGENT_RUNNER_NAME   "gui"

typedef char * (* hibus_method_handler)(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);

struct _wifi_context;
typedef struct _wifi_context wifi_context;

#define HOTSPOT_STRING_LENGTH 40
typedef struct _wifi_hotspot
{
    char bssid[HOTSPOT_STRING_LENGTH];
    unsigned char ssid[HOTSPOT_STRING_LENGTH];
    char frenquency[HOTSPOT_STRING_LENGTH];
    char capabilities[HOTSPOT_STRING_LENGTH];
    int  signal_strength;
    bool isConnect;
} wifi_hotspot;

typedef struct _hiWiFiDeviceOps
{
    int (* open) (const char * device_name, wifi_context ** context);
    int (* close) (wifi_context * context);
    int (* connect) (wifi_context * context, const char * ssid, const char *password);
    int (* disconnect) (wifi_context * context);
    int (* get_signal_strength) (wifi_context * context);
    int (* start_scan) (wifi_context * context);
    int (* stop_scan) (wifi_context * context);
    unsigned int (* get_hotspots) (wifi_context * context, wifi_hotspot ** hotspots);
} hiWiFiDeviceOps;


#endif  // __INETD__H__
