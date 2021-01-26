#ifndef __INETD_WIFI__
#define __INETD_WIFI__

char * openDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * closeDevice(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * getNetworkDevicesStatus(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * wifiStartScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * wifiStopScanHotspots(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * wifiConnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * wifiDisconnect(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);
char * wifiGetNetworkInfo(hibus_conn* conn, const char* from_endpoint, const char* to_method, const char* method_param, int *err_code);

void wifi_register(hibus_conn * hibus_context_inetd);
void wifi_revoke(hibus_conn * hibus_context_inetd);

#endif  // __INETD_WIFI__
