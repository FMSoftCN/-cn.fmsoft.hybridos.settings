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

#include "wifi_intf.h"
#include "inetd.h"
#include "tools.h"
#include "wifi.h"

extern const char *op_errors[];

char * wifiStartScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
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
    if(strncasecmp(to_method, METHOD_WIFI_START_SCAN, strlen(METHOD_WIFI_START_SCAN)))
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

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
        
    // whether library has been loaded
    wifi_device = (WiFi_device *)device[index].device;
    if(wifi_device == NULL)
    {
        ret_code = ERR_LOAD_LIBRARY;
        goto failed;
    }
    else
    {
        if(wifi_device->context == NULL)
        {
            ret_code = ERR_DEVICE_NOT_OPENNED;
            goto failed;
        }
    }

    if((device[index].status == DEVICE_STATUS_DOWN) || (device[index].status == DEVICE_STATUS_UNCERTAIN))
    {
        ret_code = ERR_OPEN_WIFI_DEVICE;
        if(ifconfig_helper(device_name, 1))
            goto failed;
    }

    ret_code = wifi_device->wifi_device_Ops->start_scan(wifi_device->context);

    wifi_device->wifi_device_Ops->get_hotspots(wifi_device->context, &(wifi_device->first_hotspot));
{
    wifi_hotspot * spot = wifi_device->first_hotspot;
    while(spot)
    {
        printf("========== %s, %s, %d\n", spot->bssid, spot->ssid, spot->signal_strength);
        spot = spot->next;
    }
}

failed:
    if(jo)
        json_object_put (jo);


    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;

}

char * wifiStopScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
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
    if(strncasecmp(to_method, METHOD_WIFI_STOP_SCAN, strlen(METHOD_WIFI_STOP_SCAN)))
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

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
        
    wifi_device = (WiFi_device *)device[index].device;
    if(wifi_device == NULL)
    {
        ret_code = ERR_LOAD_LIBRARY;
        goto failed;
    }
    else
    {
        if(wifi_device->context == NULL)
        {
            ret_code = ERR_DEVICE_NOT_OPENNED;
            goto failed;
        }
    }

    if((device[index].status == DEVICE_STATUS_DOWN) || (device[index].status == DEVICE_STATUS_UNCERTAIN))
    {
        ret_code = ERR_OPEN_WIFI_DEVICE;
        if(ifconfig_helper(device_name, 1))
            goto failed;
    }

    ret_code = wifi_device->wifi_device_Ops->stop_scan(wifi_device->context);

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * wifiConnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    const char * ssid = NULL;
    const char * password = NULL;
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
    if(strncasecmp(to_method, METHOD_WIFI_CONNECT_AP, strlen(METHOD_WIFI_CONNECT_AP)))
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

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
    
    // get WiFi ssid 
    if(json_object_object_get_ex(jo, "ssid", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    ssid = json_object_get_string(jo_tmp);
    if(ssid && strlen(ssid) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    // get WiFi password 
    if(json_object_object_get_ex(jo, "password", &jo_tmp) == 0)
    {
        ret_code = ERR_WRONG_JSON;
        goto failed;
    }

    password = json_object_get_string(jo_tmp);
    if(password && strlen(password) == 0)
    {
        ret_code = ERR_NO_DEVICE_NAME_IN_PARAM;
        goto failed;
    }

    wifi_device = (WiFi_device *)device[index].device;
    if(wifi_device == NULL)
    {
        ret_code = ERR_LOAD_LIBRARY;
        goto failed;
    }
    else
    {
        if(wifi_device->context == NULL)
        {
            ret_code = ERR_DEVICE_NOT_OPENNED;
            goto failed;
        }
    }

    if((device[index].status == DEVICE_STATUS_DOWN) || (device[index].status == DEVICE_STATUS_UNCERTAIN))
    {
        ret_code = ERR_OPEN_WIFI_DEVICE;
        if(ifconfig_helper(device_name, 1))
            goto failed;
    }

    ret_code = wifi_device->wifi_device_Ops->connect(wifi_device->context, ssid, password);

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * wifiDisconnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
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
    if(strncasecmp(to_method, METHOD_WIFI_DISCONNECT_AP, strlen(METHOD_WIFI_CONNECT_AP)))
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

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
    
    wifi_device = (WiFi_device *)device[index].device;
    if(wifi_device == NULL)
    {
        ret_code = ERR_LOAD_LIBRARY;
        goto failed;
    }
    else
    {
        if(wifi_device->context == NULL)
        {
            ret_code = ERR_DEVICE_NOT_OPENNED;
            goto failed;
        }
    }

    if((device[index].status != DEVICE_STATUS_DOWN) && (device[index].status != DEVICE_STATUS_UNCERTAIN))
        ret_code = wifi_device->wifi_device_Ops->disconnect(wifi_device->context);

failed:
    if(jo)
        json_object_put (jo);

    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);
    return ret_string;
}

char * wifiGetNetworkInfo(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code)
{
    hibus_json *jo = NULL;
    hibus_json *jo_tmp = NULL;
    const char * device_name = NULL;
    char reply[512];
    int reply_length = 512;
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
    if(strncasecmp(to_method, METHOD_WIFI_GET_NETWORK_INFO, strlen(METHOD_WIFI_GET_NETWORK_INFO)))
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

    if(device[index].type != DEVICE_TYPE_WIFI)
    {
        ret_code = ERR_NOT_WIFI_DEVICE;
        goto failed;
    }
    
    wifi_device = (WiFi_device *)device[index].device;
    if(wifi_device == NULL)
    {
        ret_code = ERR_LOAD_LIBRARY;
        goto failed;
    }
    else
    {
        if(wifi_device->context == NULL)
        {
            ret_code = ERR_DEVICE_NOT_OPENNED;
            goto failed;
        }
    }

    if(device[index].status != DEVICE_STATUS_LINK)
    {
        ret_code = ERR_DEVICE_NOT_CONNECT;
        goto failed;
    }

    ret_code = wifi_device->wifi_device_Ops->get_cur_net_info(wifi_device->context, reply, reply_length);

    memset(ret_string, 0, 4096);
    sprintf(ret_string, "{\"data\":{");
    if(ret_code == 0)
    {
        char * tempstart = NULL;
        char * tempend = NULL;
        char content[64];

        // device name
        sprintf(ret_string + strlen(ret_string), "\"device\":\"%s\",", device_name);

        // bssid
        memset(content, 0, 64);
        tempstart = strstr(reply, "bssid=");
        if(tempstart)
        {
            tempstart += strlen("bssid=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"bssid\":\"%s\",", content);

        // frenquency 
        memset(content, 0, 64);
        tempstart = strstr(tempend, "freq=");
        if(tempstart)
        {
            tempstart += strlen("freq=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"frenquency\":\"%s MHz\",", content);

        // ssid
        memset(content, 0, 64);
        tempstart = strstr(tempend, "ssid=");
        if(tempstart)
        {
            tempstart += strlen("ssid=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"ssid\":\"%s\",", content);

        // encryptionType
        memset(content, 0, 64);
        tempstart = strstr(tempend, "key_mgmt=");
        if(tempstart)
        {
            tempstart += strlen("key_mgmt=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"encryptionType\":\"%s\",", content);

        // ip
        memset(content, 0, 64);
        tempstart = strstr(tempend, "ip_address=");
        if(tempstart)
        {
            tempstart += strlen("ip_address=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"ip\":\"%s\",", content);

        // mac
        memset(content, 0, 64);
        tempstart = strstr(tempend, "address=");
        if(tempstart)
        {
            tempstart += strlen("address=");
            tempend = strstr(tempstart, "\n");
            if(tempend)
                memcpy(content, tempstart, tempend - tempstart);
        }
        sprintf(ret_string + strlen(ret_string), "\"mac\":\"%s\",", content);

        // speed
        sprintf(ret_string + strlen(ret_string), "\"speed\":\"%d Mbps\"", device[index].speed);

        // gate way

        // singal

    }
    sprintf(ret_string + strlen(ret_string), "},");

failed:
    if(jo)
        json_object_put (jo);

    sprintf(ret_string + strlen(ret_string), "\"errCode\":%d, \"errMsg\":\"%s\"}", ret_code, op_errors[-1 * ret_code]);

    return ret_string;
}

void wifi_register(hibus_conn * hibus_context_inetd)
{
    int ret_code = 0;

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

    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_START_SCAN);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_STOP_SCAN);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_CONNECT_AP);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_DISCONNECT_AP);
    hibus_revoke_procedure(hibus_context_inetd, METHOD_WIFI_GET_NETWORK_INFO);
}

