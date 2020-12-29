#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wifi_intf.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <hibus.h>

#include "inetd.h"

#define CONFIG_CTRL_IFACE_DIR "/var/run/wpa_supplicant"
typedef struct _wifi_context
{
    int a;
} wifi_context;

static hiWiFiDeviceOps wifiOps;

#ifdef gengyue
static const char *ctrl_iface_dir = CONFIG_CTRL_IFACE_DIR;
static int event = WIFIMG_NETWORK_DISCONNECTED;

static void wifi_event_handle(tWIFI_EVENT wifi_event, void *buf, int event_label)
{
    printf("============================== event_label 0x%x\n", event_label);

    switch(wifi_event)
    {
        case WIFIMG_WIFI_ON_SUCCESS:
        {
            printf("WiFi on success!\n");
            event = WIFIMG_WIFI_ON_SUCCESS;
            break;
        }

        case WIFIMG_WIFI_ON_FAILED:
        {
            printf("WiFi on failed!\n");
            event = WIFIMG_WIFI_ON_FAILED;
            break;
        }

        case WIFIMG_WIFI_OFF_FAILED:
        {
            printf("wifi off failed!\n");
            event = WIFIMG_WIFI_OFF_FAILED;
            break;
        }

        case WIFIMG_WIFI_OFF_SUCCESS:
        {
            printf("wifi off success!\n");
            event = WIFIMG_WIFI_OFF_SUCCESS;
            break;
        }

        case WIFIMG_NETWORK_CONNECTED:
        {
            printf("WiFi connected ap!\n");
            event = WIFIMG_NETWORK_CONNECTED;
            break;
        }

        case WIFIMG_NETWORK_DISCONNECTED:
        {
            printf("WiFi disconnected!\n");
            event = WIFIMG_NETWORK_DISCONNECTED;
            break;
        }

        case WIFIMG_PASSWORD_FAILED:
        {
            printf("Password authentication failed!\n");
            event = WIFIMG_PASSWORD_FAILED;
            break;
        }

        case WIFIMG_CONNECT_TIMEOUT:
        {
            printf("Connected timeout!\n");
            event = WIFIMG_CONNECT_TIMEOUT;
            break;
        }

        case WIFIMG_NO_NETWORK_CONNECTING:
        {
            printf("It has no wifi auto connect when wifi on!\n");
            event = WIFIMG_NO_NETWORK_CONNECTING;
            break;
        }

        case WIFIMG_CMD_OR_PARAMS_ERROR:
        {
            printf("cmd or params error!\n");
            event = WIFIMG_CMD_OR_PARAMS_ERROR;
            break;
        }

        case WIFIMG_KEY_MGMT_NOT_SUPPORT:
        {
            printf("key mgmt is not supported!\n");
            event = WIFIMG_KEY_MGMT_NOT_SUPPORT;
            break;
        }

        case WIFIMG_OPT_NO_USE_EVENT:
        {
            printf("operation no use!\n");
            event = WIFIMG_OPT_NO_USE_EVENT;
            break;
        }

        case WIFIMG_NETWORK_NOT_EXIST:
        {
            printf("network not exist!\n");
            event = WIFIMG_NETWORK_NOT_EXIST;
            break;
        }

        case WIFIMG_DEV_BUSING_EVENT:
        {
            printf("wifi device busing!\n");
            event = WIFIMG_DEV_BUSING_EVENT;
            break;
        }

        default:
        {
            printf("Other event, no care!\n");
        }
    }
}

static char * get_default_ifname(void)
{
    char * ifname = NULL;
    struct dirent *dent = NULL;

    DIR * dir = opendir(ctrl_iface_dir);

    if(!dir) 
        return NULL;

    while((dent = readdir(dir))) 
    {
        if (dent->d_type != DT_SOCK && dent->d_type != DT_UNKNOWN)
            continue;
        if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;
        ifname = strdup(dent->d_name);
        break;
    }
    closedir(dir);

    return ifname;
}

int main(int argv, char *argc[])
{
    int ret = 0;
    int len = 0;
    int event_label = 0;
    char results[4096] = {0};
    const aw_wifi_interface_t *p_wifi_interface = NULL;
    char * ctrl_ifname = NULL;

    // get the default WiFi device
    ctrl_ifname = get_default_ifname();
    if(ctrl_ifname)
    {
        memset(results, 0, 4096);
        sprintf(results, "%s/%s", ctrl_iface_dir, ctrl_ifname);
        free(ctrl_ifname);
    }
    printf("Selected interface '%s'\n", results);


    // connect to WiFi device
    event_label = rand();
    printf("============================== event_label 0x%x\n", event_label);
    p_wifi_interface = aw_wifi_on(wifi_event_handle, event_label, results);
    if(p_wifi_interface == NULL)
    {
        printf("wifi on failed event 0x%x\n", event);
        return -1;
    }

    while(aw_wifi_get_wifi_state() == WIFIMG_WIFI_BUSING)
    {
        printf("wifi state busing,waiting\n");
        usleep(2000000);
    }


    event_label++;
    printf("============================== event_label 0x%x\n", event_label);

    printf("\n*********************************\n");
    printf("***Start scan!***\n");
    printf("*********************************\n");


    ret = p_wifi_interface->start_scan(event_label);
    printf("ret of scan is %d\n", ret);

    if(ret == 0)
    {
        printf("******************************\n");
        printf("Wifi scan: Success!\n");
        printf("******************************\n");
    }
    else
    {
        printf("start scan failed!\n");

    }


    len = 4096;
    ret = p_wifi_interface->get_scan_results(results, &len);

    printf("ret of get_scan_results is %d\n", ret);
    if(ret == 0)
    {
        printf("%s\n", results);
        printf("******************************\n");
        printf("Wifi get_scan_results: Success!\n");
        printf("******************************\n");
    }
    else
    {
        printf("Get_scan_results failed!\n");
    }

    return 0;
}
#endif


int open_device(const char * device_name, wifi_context ** context)
{
    wifi_context * con = malloc(sizeof(wifi_context));
    memset(con, 0, sizeof(wifi_context));
    * context = con;
printf("======================================================= wifi: open.\n");
    return 0;
}

int close_device(wifi_context * context)
{
    if(context)
        free(context);

printf("======================================================= wifi: close.\n");
    return 0;
}

int connect(wifi_context * context, const char * ssid, const char *password)
{
    int ret_code = 0;
printf("======================================================= wifi: connect.\n");
    return ret_code;
}

int disconnect(wifi_context * context)
{
    int ret_code = 0;
printf("======================================================= wifi: disconnect.\n");
    return ret_code;
}

int get_signal_strength(wifi_context * context)
{
    int ret_code = 0;
printf("======================================================= wifi: get_signal_strength.\n");
    return ret_code;
}

int start_scan(wifi_context * context)
{
    int ret_code = 0;
printf("======================================================= wifi: start_scan.\n");
    return ret_code;
}

int stop_scan(wifi_context * context)
{
    int ret_code = 0;
printf("======================================================= wifi: stop_scan.\n");
    return ret_code;
}

unsigned int get_hotspots(wifi_context * context, wifi_hotspot ** hotspots)
{
    unsigned int ret_code = 0;
printf("======================================================= wifi: get_hotspots.\n");
    return ret_code;
}


// initialize device.
hiWiFiDeviceOps * __wifi_device_ops_get(void)
{
    // initialize wifiOps 
    wifiOps.open = open_device;
    wifiOps.close = close_device;
    wifiOps.connect = connect;
    wifiOps.disconnect = disconnect;
    wifiOps.get_signal_strength = get_signal_strength;
    wifiOps.stop_scan = stop_scan;
    wifiOps.start_scan = start_scan;
    wifiOps.get_hotspots = get_hotspots;

    return &wifiOps;
}
