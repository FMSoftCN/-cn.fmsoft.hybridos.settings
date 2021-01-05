#ifndef __INETD__H__
#define __INETD__H__

// only for develop
#define INETD_CONFIG_FILE   "/home/projects/hibusdaemon/bin/inetd.cfg"
#define INETD_LIBRARY_PATH  "/home/projects/hibusdaemon/lib"

// ================ For WiFi <<<<  =========================================================
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

// method for WiFi
#define METHOD_WIFI_OPEN_DEVICE         "openDevice"
#define METHOD_WIFI_CLOSE_DEVICE        "closeDevice"
#define METHOD_WIFI_GET_DEVICES_STATUS  "getNetworkDevicesStatus"
#define METHOD_WIFI_START_SCAN          "wifiStartScanHotspots"
#define METHOD_WIFI_STOP_SCAN           "wifiStopScanHotspots"
#define METHOD_WIFI_CONNECT_AP          "wifiConnect"
#define METHOD_WIFI_DISCONNECT_AP       "wifiDisconnect"
#define METHOD_WIFI_GET_NETWORK_INFO    "wifiGetNetworkInfo"

// event for WiFi
#define NETWORKDEVICECHANGED            "NETWORKDEVICECHANGED"
#define WIFINEWHOTSPOTS                 "WIFINEWHOTSPOTS"
#define WIFISIGNALSTRENGTHCHANGED       "WIFISIGNALSTRENGTHCHANGED"
// ================ >>>> For WiFi  =========================================================


// ================ For Ethernet  <<<<  ====================================================
// ================ >>>> For Ethernet  =====================================================


// ================ For Mobile  <<<<  ======================================================
// ================ >>>> For Mobile  =======================================================


#define WORKING_DIRECTORY               "/home/gengyue"                     // for daemon

#define APP_INETD_NAME                  "cn.fmsoft.hybridos.inetd"
#define RUNNER_WIFI_NAME                "wifi"
#define RUNNER_ETHERNET_NAME            "ethernet"
#define RUNNER_MOBILE_NAME              "mobile"
#define SOCKET_PATH                     "/var/tmp/hibus.sock"

#define MAX_DEVICE_NUM                  10

// device type
#define DEVICE_TYPE_UNKONWN             0
#define DEVICE_TYPE_ETHERNET            1
#define DEVICE_TYPE_WIFI                2
#define DEVICE_TYPE_MOBILE              3
#define DEVICE_TYPE_LO                  4
#define DEVICE_TYPE_DEFAULT             DEVICE_TYPE_ETHERNET

// device status
#define DEVICE_STATUS_UNCERTAIN         0
#define DEVICE_STATUS_DOWN              1
#define DEVICE_STATUS_UP                2
#define DEVICE_STATUS_LINK              3
#define DEVICE_STATUS_UNLINK            4

// for time period
#define DEFAULT_SCAN_TIME               120
#define DEFAULT_SIGNAL_TIME             30

typedef struct _WiFi_device
{
    hiWiFiDeviceOps * wifi_device_Ops;
    wifi_context * context;
    int scan_time;
    int signal_time;
} WiFi_device;

#define NETWORK_DEVICE_NAME_LENGTH  32
typedef struct _network_device
{
    char ifname[NETWORK_DEVICE_NAME_LENGTH];
    int type;
    int status;
    int priority;
    void * device;
    void * lib_handle;
    char mac[16];
    char ip[32];
    char broadAddr[32];
    char subnetMask[32];
} network_device;

int get_device_index(const network_device * device, const char * ifname);

// for test
#define AGENT_NAME          "cn.fmsoft.hybridos.sysmgr"
#define AGENT_RUNNER_NAME   "gui"

#endif  // __INETD__H__
