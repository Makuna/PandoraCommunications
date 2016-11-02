#pragma once

#include <ESP8266WiFi.h>

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
        Serial.print("using channel ");
        Serial.println(channel);

        WiFi.setPhyMode(WIFI_PHY_MODE_11G);
        WiFi.setOutputPower(8); // low power
        WiFi.mode(WIFI_AP);

        WiFi.softAP(ssid, password, channel);

        delay(1000);

        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());

        // Start the server
        _server.begin();
        Serial.println("Server started");

        WiFi.printDiag(Serial);
    }

    void loop(ReceiveCommandCallback callback)
    {
        WiFiClient newClient = _server.available();
        if (newClient)
        {
            Serial.println("client connected");
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
                    Serial.println("read command error");
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

                // higher ranked channels also bleed over to nearby channels 
                if (rankScan > 2)
                {
                    countUsingChannel[channelScan - 1] += 1;
                    countUsingChannel[channelScan + 1] += 1;
                }
            }

            Serial.print(WiFi.SSID(indexNetwork));
            Serial.print(" (");
            Serial.print(channelScan);
            Serial.print(") ");
            Serial.print(rssiScan);
            Serial.print(" [");
            Serial.print(rankScan);
            Serial.println("]");
        }

        Serial.println();

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

