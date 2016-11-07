#pragma once

#include <ESP8266WiFi.h>

enum PandoraClientState
{
    PandoraClientState_WifiConnecting,
    PandoraClientState_WifiConnected,

    PandoraClientState_ClientConnecting,
    PandoraClientState_ClientConnected,

    PandoraClientState_DeviceReset,
};

struct PandoraClientStatus
{
    PandoraClientState state;
};

// define ClientStatusCallback to be a function like
// void OnClientStatusChange(const PandoraClientStatus& status) {}
typedef void(*ClientStatusCallback)(const PandoraClientStatus& status);

class PandoraClient
{
public:
    PandoraClient()
    {
    }

    void begin(const char* ssid, const char* password)
    {
        WiFi.setAutoConnect(false);
        WiFi.disconnect(true);
        delay(500);

        WiFi.setPhyMode(WIFI_PHY_MODE_11G);
        WiFi.setOutputPower(8); // low power
        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);
        _state = PandoraClientState_WifiConnecting;
    }

    void loop(ClientStatusCallback callback)
    {
        if (WiFi.isConnected())
        {
            if (_state == PandoraClientState_WifiConnecting)
            {
                _state = PandoraClientState_WifiConnected;
                PandoraClientStatus status;
                status.state = _state;
                callback(status);
            }

			if (!_client)
			{
                _state = PandoraClientState_ClientConnecting;
				ConnectClient(callback);
			}
            else
            {
                // heart beat required to keep client alive
                uint32_t time = millis();
                if (time - _heartBeatTime > 2000)
                {
                    _heartBeatTime = time;

                    const uint16_t heartBeat = 0xffff;
                    _client.write((uint8_t*)&heartBeat, sizeof(heartBeat));
                }
            }
		}
        else
        {
            _state = PandoraClientState_WifiConnecting;

            PandoraClientStatus status;
            status.state = _state;
            callback(status);

            delay(500);
        }
    }

    bool sendCommand(const BookCommand& command)
    {
        bool sent = true;
        if (_client)
        {
            _client.write((uint8_t*)&command, sizeof(BookCommand));
        }
        else
        {
            sent = false;
        }
        return sent;
    }

private:
    WiFiClient _client;
    uint32_t _heartBeatTime;
    PandoraClientState _state;

    void ConnectClient(ClientStatusCallback callback)
    {
        IPAddress ipGateway = WiFi.gatewayIP();

        for (int attempts = 0; attempts < 3; ++attempts)
        {
            if (_client.connect(ipGateway, c_PandoraPort))
            {
                break;
            }

            PandoraClientStatus status;
            status.state = _state;
            callback(status);

            delay(1000);
        }

        if (_client)
        {
            _state = PandoraClientState_ClientConnected;

            PandoraClientStatus status;
            status.state = _state;
            callback(status);

            _heartBeatTime = millis();
        }
        else
        {
            PandoraClientStatus status;
            status.state = PandoraClientState_DeviceReset;
            callback(status);


            ESP.reset();
            delay(1000);
        }
    }
};

