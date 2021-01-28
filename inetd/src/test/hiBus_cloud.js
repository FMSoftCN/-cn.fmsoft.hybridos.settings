{
    function hiBus(appName, runnerName, port, callback)
    {
        // hiBus status
        hiBus.WSUNCONNECT = 0;
        hiBus.WSCONNECT = 1;
        hiBus.WSAUTHENRITY = 2;
        hiBus.WSWORKING = 3;

        // hiBus error
        hiBus.ERRNO = 0;
        hiBus.ERRCONNECT = 1;
        hiBus.ERRAUTHENRITY = 2;

        var ret = true;
        var connectCallback = callback;

        // step 1: check validation
        if (typeof(appName) != "string")
            ret = false;
        else
            this.appName = appName;

        if (typeof(runnerName) != "string")
            ret = false;
        else
            this.runnerName = runnerName;

        if (port === "" || port == null)
            this.port = 7700;
        else {
            if (isNaN(port))
                this.port = 7700;
            else
                this.port = port;
        }

        eventArray = new Array();
        procedureArray = new Array();

        this.WSStatus = hiBus.WSUNCONNECT; 
        webSocket = new WebSocket("ws://dev.fmsoft.cn:" + this.port);

        // generate UUID
        this.genID = function(length) {
            return Number(Math.random().toString().substr(3, length) + Date.now()).toString(36);
        }

        // websocket onopen
        webSocket.onopen = function() {
           WSStatus = hiBus.WSCONNECT;
        };

        // websocket onclose
        webSocket.onclose = function(e) {
            if(WSStatus == hiBus.WSCONNECT) {
                if(typeof(connectCallback) == "function") 
                    connectCallback(hiBus.ERRCONNECT);
            }
            else if(WSStatus == hiBus.WSAUTHENRITY) {
                if(typeof(connectCallback) == "function") 
                    connectCallback(hiBus.ERRAUTHENRITY);
            }
            else if(WSStatus == hiBus.WSWORKING) {
            }
            else if(WSStatus == hiBus.WSUNCONNECT) {
            }

            WSStatus = hiBus.WSUNCONNECT;
        }

        // websocket onerror
        webSocket.onerror = function(e) {
            if(WSStatus == hiBus.WSCONNECT) {
            }
            else if(WSStatus == hiBus.WSAUTHENRITY) {
            }
            else if(WSStatus == hiBus.WSWORKING) {
            }
            else if(WSStatus == hiBus.WSUNCONNECT) {
            }
        }

        // websocket onmessage
        webSocket.onmessage = function(e) {
            console.log(e.data);

            var packetJson;

            try {
                packetJson = JSON.parse(e.data);
            } catch (e) {
                console.log(e.message);
                return;
            }

            if(WSStatus == hiBus.WSCONNECT) {
                if (packetJson.packetType == "auth" && packetJson.protocolName == "HIBUS") {
                    var version = packetJson.protocolVersion;
                    var chCode = packetJson.challengeCode;
                    var sendJson = {
                        "packetType":"auth",
                        "protocolName":"HIBUS",
                        "protocolVersion":version,
                        "hostName":"localhost",
                        "appName":appName,
                        "runnerName":runnerName,
                        "signature":"RFVNQg==",
                        "encodedIn":"base64"
                    };
                    this.send(JSON.stringify(sendJson));
                    WSStatus = hiBus.WSAUTHENRITY;
                }
            }
            else if(WSStatus == hiBus.WSAUTHENRITY) {
                if (packetJson.packetType == "authPassed") {
                    if(typeof(connectCallback) == "function") 
                        connectCallback(hiBus.ERRNO);
                    WSStatus = hiBus.WSWORKING;
                }
                else if (packetJson.packetType == "authFailed") {
                    this.close();
                }
            }
            else if(WSStatus == hiBus.WSWORKING) {
                if (packetJson.packetType == "event") {
                    // check whether in eventArray
                    for (var i = 0, len = eventArray.length; i < len; i++) {
                        if(!eventArray[i])
                            continue;

                        // If find the node, do not break the loop.
                        // Because runner can subscribe one event with different functions.
                        if(eventArray[i].endpointName == packetJson.fromEndpoint &&
                                eventArray[i].eventName == packetJson.fromBubble) {
                            if(typeof(eventArray[i].callback) == "function")
                                eventArray[i].callback(packetJson.bubbleData);
                        }
                    }
                }
                else if (packetJson.packetType == "result") {
                    // submit to dest runner
                    if (packetJson.retCode == 202) {
                    }
                    else
                    {
                        // check whether in eventArray
                        for (var i = 0, len = procedureArray.length; i < len; i++) {
                            if (!procedureArray[i])
                                continue;
                            if (procedureArray[i].callId == packetJson.callId && procedureArray[i].toEndpoint == packetJson.fromEndpoint &&
                                    procedureArray[i].toMethod == packetJson.fromMethod) {
                                if(typeof(procedureArray[i].callback) == "function")
                                {
                                    if(packetJson.fromEndpoint == "@localhost/cn.fmsoft.hybridos.hibus/builtin" &&
                                                (packetJson.fromMethod == "subscribeevent" || packetJson.fromMethod == "unsubscribeevent"))
                                        procedureArray[i].callback(packetJson.retCode);
                                    else
                                        procedureArray[i].callback(packetJson.retValue);
                                }
                                delete procedureArray[i];
                            }
                        }
                    }
                }
                else if (packetJson.packetType == "error") {
                    // check whether in eventArray
                    for (var i = 0, len = procedureArray.length; i < len; i++) {
                        if (!procedureArray[i])
                            continue;
                        if (procedureArray[i].callId == packetJson.causedId && packetJson.causedBy == "call") {
                            if(typeof(procedureArray[i].callback) == "function")
                                procedureArray[i].callback(packetJson.retCode);
                            delete procedureArray[i];
                        }
                    }
                }
            }
            else if(WSStatus == hiBus.WSUNCONNECT) {
            }
        }

        this.callProcedure = function(endpoint, methodName, param, timeout, callback)
        {
            var emptyPosition;
            var callId = this.genID(36);

            // step 1: check validation
            if (typeof(endpoint) != "string")
                return false;

            if (typeof(methodName) != "string")
                return false;

            if (timeout === "" || timeout == null)
                timeout = 0;
            if (isNaN(timeout))
                timeout = 0;

            if (callback == null)
            {
                // send request to hihus directly, do not push it to procedure array. 
            }
            else
            {
                // check empty position in eventArray
                for (var i = 0, len = procedureArray.length; i < len; i++)
                {
                    if (!procedureArray[i])
                    {
                        emptyPosition = i;
                        break;
                    }
                }

                if (typeof(emptyPosition) == "undefined")
                    procedureArray.push({
                            "callId":callId,
                            "toEndpoint":endpoint.toLocaleLowerCase(),
                            "toMethod":methodName.toLocaleLowerCase(),
                            "expectedTime":timeout,
                            "callback":callback
                            });
                else
                    procedureArray[emptyPosition] = {
                        "callId":callId,
                        "toEndpoint":endpoint.toLocaleLowerCase(),
                        "toMethod":methodName.toLocaleLowerCase(),
                        "expectedTime":timeout,
                        "callback":callback
                    };
            }

            // send to hiBus
            var sendJson = {
                "packetType":"call",
                "callId":callId,
                "toEndpoint":endpoint,
                "toMethod":methodName,
                "expectedTime":timeout,
                "authenInfo":{},
                "parameter":param
            };
            webSocket.send(JSON.stringify(sendJson));

            return true;
        }

        this.subscribeEvent = function(endpointName, eventName, callback, subscribeCallback)
        {
            var findEvent = false;
            var emptyPosition;

            var eventId = this.genID(36);

            // step 1: check validation
            if (typeof(endpointName) != "string")
                return false;

            if (typeof(eventName) != "string")
                return false;

            if (typeof(callback) != "function") 
                return false;

            // check whether in eventArray
            for (var i = 0, len = eventArray.length; i < len; i++)
            {
                if (!eventArray[i])
                {
                    emptyPosition = i;
                    continue;
                }

                // delete it. if find, do not break the loop.
                // because runner can subscribe one event with different functions
                if (eventArray[i].endpointName == endpointName &&
                        eventArray[i].eventName == eventName &&
                        eventArray[i].callback == callback)
                {
                    findEvent = true;
                    break;
                }
            }

            // set in eventArray
            if(!findEvent)
            {
                if (typeof(emptyPosition) == "undefined")
                    eventArray.push({
                            "endpointName":endpointName, 
                            "eventName":eventName, 
                            "callback":callback
                            });
                else
                    eventArray[emptyPosition] = {
                        "endpointName":endpointName,
                        "eventName":eventName, 
                        "callback":callback
                    };
            }

            this.callProcedure("@localhost/cn.fmsoft.hybridos.hibus/builtin", "subscribeEvent", 
                            "{\"endpointName\":\"" + endpointName + "\", \"bubbleName\":\"" + eventName + "\"}", 30000, subscribeCallback);
            return true;
        }

        hiBus.prototype.unsubscribeEvent = function(endpointName, eventName, unsubscribeCallback)
        {
            // step 1: check validation
            if (typeof(endpointName) != "string")
                return false;

            if (typeof(eventName) != "string")
                return false;

            // check whether in eventArray
            for (var i = 0, len = eventArray.length; i < len; i++)
            {
                if(!eventArray[i])
                    continue;

                // If find the node, do not break the loop.
                // Because runner can subscribe one event with different functions.
                if(eventArray[i].endpointName == endpointName &&
                        eventArray[i].eventName == eventName) {
                    delete eventArray[i];
                }
            }

            this.callProcedure("@localhost/cn.fmsoft.hybridos.hibus/builtin", "unsubscribeEvent", 
                            "{\"endpointName\":\"" + endpointName + "\", \"bubbleName\":\"" + eventName + "\"}", 30000, unsubscribeCallback);
            return true;
        }

    }
}
