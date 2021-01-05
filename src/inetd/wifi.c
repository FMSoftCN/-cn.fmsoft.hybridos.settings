#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <net/if.h>
#include <sys/timerfd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <arpa/inet.h>
#include <errno.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <hibus.h>
#include <hibox/json.h>

#include "inetd.h"
#include "wifi.h"

static const char *op_errors[] = {
    "success",
    "can not get devices list.",
    "wrong procedure name.",
    "wrong Json format.",
    "can not find device name in param.",
    "can not find device in system.",
    "device is not WiFi device.",
    "can not get operation functions.",
    "an error ocurs in open wifi device.",
    "an error ocurs in close wifi device."
};


char * openDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = ERR_NO;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    // get device array
    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = ERR_NONE_DEVICE_LIST;
        goto failed;
    }

    // get procedure name
    if(strncasecmp(to_method, METHOD_WIFI_OPEN_DEVICE, strlen(METHOD_WIFI_OPEN_DEVICE)))
    {
        ret_code = ERR_WRONG_PROCEDURE;
        goto failed;
    }

    // analyze json
    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    // get device name
    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    // device does exist?
    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = ERR_NO_DEVICE_IN_SYSTEM;
        goto failed;
    }

    // device is WiFi device
    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
    
    if((device[index].status == DEVICE_STATUS_DOWN) || (device[index].status == DEVICE_STATUS_UNCERTAIN))
    {
        wifi_device = (WiFi_device *)device[index].device;
        if(wifi_device == NULL)
        {
            ret_code = ERR_NO_OPERATION_LIST;
            goto failed;
        }
        else
            ret_code = wifi_device->wifi_device_Ops->open(device_name, &(wifi_device->context));
    }

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 128);
    sprintf(ret_string, "{\"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;

}

char * closeDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = ERR_NONE_DEVICE_LIST;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_CLOSE_DEVICE, strlen(METHOD_WIFI_CLOSE_DEVICE)))
    {
        ret_code = ERR_WRONG_PROCEDURE;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = ERR_NO_DEVICE_IN_SYSTEM;
        goto failed;
    }

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
        
    if((device[index].status != DEVICE_STATUS_DOWN) && (device[index].status != DEVICE_STATUS_UNCERTAIN))
    {
        wifi_device = (WiFi_device *)device[index].device;
        if(wifi_device == NULL)
        {
            ret_code = ERR_NO_OPERATION_LIST;
            goto failed;
        }
        else
            ret_code = wifi_device->wifi_device_Ops->close(wifi_device->context);
    }

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 128);
    sprintf(ret_string, "{\"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * getNetworkDevicesStatus(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    return NULL;
}

char * wifiStartScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = -1;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_START_SCAN, strlen(METHOD_WIFI_START_SCAN)))
    {
        ret_code = -2;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = -3;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = -3;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = -4;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = -5;
        goto failed;
    }

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = -6;
        goto failed;
    }
        
failed:
    if(jo)
        json_object_put (jo);

#ifdef gengyue
    wifi_device_Ops->start_scan(context);
        fprintf(stderr, "WIFI DAEMON: end scan. %d.\n", ret_code);
    wifi_device_Ops->get_hotspots(context, NULL);
#endif
    memset(ret_string, 0, 128);
    sprintf(ret_string, "{\"data\":\"\", \"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * wifiStopScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = -1;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_STOP_SCAN, strlen(METHOD_WIFI_STOP_SCAN)))
    {
        ret_code = -2;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = -3;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = -3;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = -4;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = -5;
        goto failed;
    }

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = -6;
        goto failed;
    }
        
failed:
    if(jo)
        json_object_put (jo);

    if(ret_code)
    {
        memset(ret_string, 0, 128);
        sprintf(ret_string, "{\"data\":\"\", \"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
        return ret_string;
    }

    return strdup("{\"data\":\"\", \"errCode\":\"0\", \"errMsg\":\"OK\"}");
}

char * wifiConnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = -1;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_CONNECT_AP, strlen(METHOD_WIFI_CONNECT_AP)))
    {
        ret_code = -2;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = -3;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = -3;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = -4;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = -5;
        goto failed;
    }

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = -6;
        goto failed;
    }
        
failed:
    if(jo)
        json_object_put (jo);

    if(ret_code)
    {
        memset(ret_string, 0, 128);
        sprintf(ret_string, "{\"data\":\"\", \"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
        return ret_string;
    }

    return strdup("{\"data\":\"\", \"errCode\":\"0\", \"errMsg\":\"OK\"}");
}

char * wifiDisconnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    int index = -1;
    int ret_code = 0;
    char * ret_string = malloc(4096);
    WiFi_device * wifi_device = NULL;

    network_device * device = hibus_conn_get_user_data(conn);
    if(device == NULL)
    {
        ret_code = -1;
        goto failed;
    }

    if(strncasecmp(to_method, METHOD_WIFI_DISCONNECT_AP, strlen(METHOD_WIFI_DISCONNECT_AP)))
    {
        ret_code = -2;
        goto failed;
    }

    jo = hibus_json_object_from_string(method_param, strlen(method_param), 2);
    if(jo == NULL)
    {
        ret_code = -3;
        goto failed;
    }

    if(json_object_object_get_ex(jo, "device", &jo_tmp) == 0)
    {
        ret_code = -3;
        goto failed;
    }

    device_name = json_object_get_string(jo_tmp);
    if(device_name && strlen(device_name) == 0)
    {
        ret_code = -4;
        goto failed;
    }

    index = get_device_index(device, device_name);
    if(index == -1)
    {
        ret_code = -5;
        goto failed;
    }

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = -6;
        goto failed;
    }
        
failed:
    if(jo)
        json_object_put (jo);

    if(ret_code)
    {
        memset(ret_string, 0, 128);
        sprintf(ret_string, "{\"data\":\"\", \"errCode\":\"%d\", \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
        return ret_string;
    }

    return strdup("{\"data\":\"\", \"errCode\":\"0\", \"errMsg\":\"OK\"}");
}

char * wifiGetNetworkInfo(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    return NULL;
}

void wifi_register(hibus_conn * hibus_context_inetd)
{
    int ret_code = 0;

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_OPEN_DEVICE, NULL, NULL, openDevice);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_OPEN_DEVICE, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_CLOSE_DEVICE, NULL, NULL, closeDevice);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_CLOSE_DEVICE, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_GET_DEVICES_STATUS, NULL, NULL, getNetworkDevicesStatus);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_GET_DEVICES_STATUS, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_START_SCAN, NULL, NULL, wifiStartScanHotspots);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_START_SCAN, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_STOP_SCAN, NULL, NULL, wifiStopScanHotspots);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_STOP_SCAN, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_CONNECT_AP, NULL, NULL, wifiConnect);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_CONNECT_AP, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_DISCONNECT_AP, NULL, NULL, wifiDisconnect);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_DISCONNECT_AP, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_procedure(hibus_context_inetd, METHOD_WIFI_GET_NETWORK_INFO, NULL, NULL, wifiGetNetworkInfo);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register procedure %s, %s.\n", METHOD_WIFI_GET_NETWORK_INFO, hibus_get_err_message(ret_code));
        return;
    }

    // step 5: register an event
    ret_code = hibus_register_event(hibus_context_inetd, NETWORKDEVICECHANGED, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register event %s, %s.\n", NETWORKDEVICECHANGED, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_event(hibus_context_inetd, WIFINEWHOTSPOTS, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register event %s, %s.\n", WIFINEWHOTSPOTS, hibus_get_err_message(ret_code));
        return;
    }

    ret_code = hibus_register_event(hibus_context_inetd, WIFISIGNALSTRENGTHCHANGED, NULL, NULL);
    if(ret_code)
    {
        fprintf(stderr, "WIFI DAEMON: Error for register event %s, %s.\n", WIFISIGNALSTRENGTHCHANGED, hibus_get_err_message(ret_code));
        return;
    }
}

void wifi_revoke(hibus_conn * hibus_context_inetd)
{
    hibus_revoke_event(hibus_context_inetd, WIFISIGNALSTRENGTHCHANGED);
    hibus_revoke_event(hibus_context_inetd, WIFINEWHOTSPOTS);
    hibus_revoke_event(hibus_context_inetd, NETWORKDEVICECHANGED);

    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_OPEN_DEVICE);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_CLOSE_DEVICE);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_GET_DEVICES_STATUS);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_START_SCAN);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_STOP_SCAN);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_CONNECT_AP);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_DISCONNECT_AP);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_GET_NETWORK_INFO);
}

