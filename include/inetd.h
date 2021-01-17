#ifndef __INETD__H__
#define __INETD__H__

// only for develop
#define INETD_CONFIG_FILE   "/home/projects/inetd/bin/inetd.cfg"
#define INETD_LIBRARY_PATH  "/home/projects/inetd/lib"

// ================ For WiFi <<<<  =========================================================
#define HOTSPOT_STRING_LENGTH 64 

struct _wifi_context
{
    const aw_wifi_interface_t * p_wifi_interface;
    int event_label;
};
typedef struct _wifi_context wifi_context;

typedef struct _wifi_hotspot
{
    char bssid[HOTSPOT_STRING_LENGTH];
    unsigned char ssid[HOTSPOT_STRING_LENGTH];
    char frenquency[HOTSPOT_STRING_LENGTH];
    char capabilities[HOTSPOT_STRING_LENGTH];
    int  signal_strength;
    bool isConnect;
    struct _wifi_hotspot * next;
} wifi_hotspot;

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

#define APP_NAME_SETTINGS               "cn.fmsoft.hybridos.settings"
#define RUNNER_NAME_INETD               "inetd"
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
#define DEFAULT_SCAN_TIME               30

// for error
#define ERR_NO                          0
#define ERR_LIBRARY_OPERATION           -1
#define ERR_NONE_DEVICE_LIST            -2
#define ERR_WRONG_PROCEDURE             -3
#define ERR_WRONG_JSON                  -4
#define ERR_NO_DEVICE_NAME_IN_PARAM     -5
#define ERR_NO_DEVICE_IN_SYSTEM         -6
#define ERR_DEVICE_TYPE                 -7
#define ERR_LOAD_LIBRARY                -8
#define ERR_NOT_WIFI_DEVICE             -9
#define ERR_DEVICE_NOT_OPENNED          -10
#define ERR_OPEN_WIFI_DEVICE            -11
#define ERR_CLOSE_WIFI_DEVICE           -12
#define ERR_OPEN_ETHERNET_DEVICE        -13
#define ERR_CLOSE_ETHERNET_DEVICE       -14
#define ERR_OPEN_MOBILE_DEVICE          -15
#define ERR_CLOSE_MOBILE_DEVICE         -16
#define ERR_DEVICE_NOT_CONNECT          -17

// for network changed
#define NETWORK_CHANGED_NAME            ((0x01) << 0)
#define NETWORK_CHANGED_TYPE            ((0x01) << 1)
#define NETWORK_CHANGED_STATUS          ((0x01) << 2)
#define NETWORK_CHANGED_MAC             ((0x01) << 3)
#define NETWORK_CHANGED_IP              ((0x01) << 4)
#define NETWORK_CHANGED_BROADCAST       ((0x01) << 5)
#define NETWORK_CHANGED_SUBNETMASK      ((0x01) << 6)

#define NETWORK_DEVICE_NAME_LENGTH  32
#define NETWORK_ADDRESS_LENGTH      32
#define WIFI_SSID_LENGTH            32

typedef struct _WiFi_device
{
    struct _hiWiFiDeviceOps * wifi_device_Ops;
    wifi_context * context;
    int scan_time;
    int signal;
    char ssid[WIFI_SSID_LENGTH];
    pthread_mutex_t list_mutex;             // wifi列表读写同步
    wifi_hotspot *first_hotspot;            // 扫描wifi列表
} WiFi_device;

typedef struct _network_device
{
    char ifname[NETWORK_DEVICE_NAME_LENGTH];
    int type;
    int status;
    int priority;
    void * device;
    void * lib_handle;
    char mac[NETWORK_ADDRESS_LENGTH];
    char ip[NETWORK_ADDRESS_LENGTH];
    char broadAddr[NETWORK_ADDRESS_LENGTH];
    char subnetMask[NETWORK_ADDRESS_LENGTH];
    int speed;
    hibus_conn* conn;
} network_device;

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
    int (*get_cur_net_info)(wifi_context * context, char * reply, int reply_length);                // 获得当前网络连接的各项参数，包括ssid，IP等
    int (*set_scan_interval)(wifi_context * context, int interval);                                 // 设置scan间隔
    void (* report_wifi_scan_info)(network_device * device, wifi_hotspot * hotspots, int number);   // 回调函数，上报scan结果
    network_device * device;                                                                        // 该结构对应的device结构
} hiWiFiDeviceOps;

// for test
#define AGENT_NAME          "cn.fmsoft.hybridos.sysmgr"
#define AGENT_RUNNER_NAME   "gui"

#endif  // __INETD__H__
