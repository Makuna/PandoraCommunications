#pragma once

#include <ESP8266WiFi.h>
enum PandoraServerState
{
    PandoraServerState_ClientConnected,

    PandoraServerState_CommandReadError,
};

struct PandoraServerStatus
{
    PandoraServerState state;
};

// define ServerStatusCallback to be a function like
// void OnServerStatusChange(const PandoraServerStatus& status) {}
typedef void(*ServerStatusCallback)(const PandoraServerStatus& status);


// define ReceiveCommandCallback to be a function like
// void OnReceiveCallBack(const BookCommand& param) {}
typedef void(*ReceiveCommandCallback)(const BookCommand& param);

class PandoraServer
{
public:
    PandoraServer() :
        _server(c_PandoraPort)
    {
    }

    void begin(const char* ssid, const char* password)
    {
        WiFi.setAutoConnect(false);
        WiFi.disconnect(true);
        WiFi.softAPdisconnect(false);

        delay(500);

        uint8_t channel = _scanForOpenChannel(11);

        WiFi.setPhyMode(WIFI_PHY_MODE_11G);
        WiFi.setOutputPower(8); // low power
        WiFi.mode(WIFI_AP);

        WiFi.softAP(ssid, password, channel);

        delay(1000);

        IPAddress myIP = WiFi.softAPIP();

        // Start the server
        _server.begin();
    }

    void loop(ReceiveCommandCallback callback, ServerStatusCallback statusCallback)
    {
        WiFiClient newClient = _server.available();
        if (newClient)
        {
            PandoraServerStatus status;
            status.state = PandoraServerState_ClientConnected;
            statusCallback(status);
            
            _client.stop();
            _client = newClient;
        }

        if (_client)
        {
            // wait for a complete command
            if (sizeof(BookCommand) <= _client.available())
            {
                BookCommand command;
                int read = _client.read((uint8_t*)(&command), sizeof(BookCommand));
                if (sizeof(BookCommand) == read)
                {
                    if (command.command == 0xff && command.param == 0xff)
                    {
                        // ignore the heartbeat
                    }
                    else
                    {
                        callback(command);
                    }
                }
                else
                {
                    PandoraServerStatus status;
                    status.state = PandoraServerState_CommandReadError;
                    statusCallback(status);
                }
            }
        }
    }

private:
    WiFiServer _server;
    WiFiClient _client;

    uint8_t _scanForOpenChannel(const uint8_t maxChannel)
    {
        // scan networks to pick a clear channel
        //
        int8_t countNetworks = WiFi.scanNetworks();
        uint16_t countUsingChannel[13 + 2] = { 0 }; // include space for -1 and +1, and use full support of 13 channels

        for (int8_t indexNetwork = 0; indexNetwork < countNetworks; ++indexNetwork)
        {
            int32_t rssiScan = WiFi.RSSI(indexNetwork);
            int8_t channelScan = WiFi.channel(indexNetwork); // 1-13 inclusive)
            uint8_t rankScan = (rssiScan + 100) / 5; // worse than -95db we ignore

            if (channelScan <= maxChannel)
            {
                countUsingChannel[channelScan] += rankScan;

                // higher ranked channels also bleed over to nearby channels so
                // make the nearby less desirable
                if (rankScan > 2)
                {
                    countUsingChannel[channelScan - 1] += 1;
                    countUsingChannel[channelScan + 1] += 1;
                }
            }

            //Serial.print(WiFi.SSID(indexNetwork));
            //Serial.print(" (");
            //Serial.print(channelScan);
            //Serial.print(") ");
            //Serial.print(rssiScan);
            //Serial.print(" [");
            //Serial.print(rankScan);
            //Serial.println("]");
        }

        //Serial.println();

        // find an open channel 
        // skip channel 1 completely, too many things default to it
        //
        uint8_t channel = 2;
        uint16_t channelRank = countUsingChannel[channel];
        for (uint8_t indexChannel = channel + 1; indexChannel <= maxChannel; ++indexChannel)
        {
            uint16_t testRank = countUsingChannel[indexChannel];
            if (channelRank >= testRank)
            {
                channelRank = testRank;
                channel = indexChannel;
            }
        }
        return channel;
    }


};

