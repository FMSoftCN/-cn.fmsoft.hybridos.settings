## 编译wpa_supplicant过程

对WiFi的操作，使用了wpa_suppliant。因此本篇讲述如何编译wpa_supplicant和wifimanager。

```c
------------------------------------------------------------
|        APP1        |        APP2        |       APP3       |
 ------------------------------------------------------------
|                      hiBus Server(hiBusd)                  |
 ------------------------------------------------------------ 
|              cn.fmsoft.hybridos.settings/inetd             |
 ------------------------------------------------------------
|                         libwifi.so                         |
 ------------------------------------------------------------
|                      libwifimanager.so                     |
 ------------------------------------------------------------
|          wpa_suppliant       |      libwpa_client.so       |
 ------------------------------------------------------------
|                      Linux Kernel/Drivers                  |
 ------------------------------------------------------------
```

- inetd：提供对网络管理的框架；
- libwifi.so：提供对WiFi设备的操作；
- libwifimanager.so：提供与wpa_suppliant的通信机制，发送命令并接收结果；
- libwpa_client.so：对具体网络设备的设备级操作，诸如：打开、关闭等；
- wpa_suppliant：完成对具体网络设备功能级的操作，诸如搜索、连接。

因此必须首先编译出：libwpa_client.so和libwifimanager.so。

libwifimanager.so代码来源于全智平台SDK中的wifimanager，libwpa_client.so来源于wpa_suppliant代码。



##### 1、编译libnl-1.1.4

```c
# wget http://www.infradead.org/~tgr/libnl/files/libnl-3.2.25.tar.gz
# tar zxf libnl-3.2.25.tar.gz
# cd libnl-3.2.25
# ./configure --prefix=/usr
# make
# sudo make install
```

##### 2、编译openssl-1.1

```c
# wget https://www.openssl.org/source/openssl-1.1.1e.tar.gz
# tar zxf openssl-1.1.1e.tar.gz
# cd openssl-1.1.1e
# ./config no-asm shared no-async --prefix=/usr
# make
# sudo make install
```

##### 3、编译wpa_supplicant

```c
# wget https://w1.fi/releases/wpa_supplicant-2.6.tar.gz
# tar zxf wpa_supplicant-2.6.tar.gz
# cd wpa_supplicant-2.6/wpa_supplicant
# cp defconfig .config
# vim .config
	CONFIG_LIBNL32=y
# vim Makefile
	在文件最开始添加：
        CONFIG_BUILD_WPA_CLIENT_SO=1
		LIBS += -lpthread -lm -pthread
		STATIC_LIB += libnl-3.a
		STATIC_LIB += libnl-genl--3.a
		STATIC_LIB += libnl-route-3.a
	修改：
        $(Q)$(LDO) $(LDFLAGS) -o wpa_supplicant $(OBJS) $(LIBS) $(EXTRALIBS)
    为：
        $(Q)$(LDO) $(LDFLAGS) --static -o wpa_supplicant $(OBJS) $(LIBS) $(EXTRALIBS)
# make
```

获得：libwpa_client.so