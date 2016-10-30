#pragma once

#include <ESP8266WiFi.h>

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

		_ssid = ssid;
		_password = password;
        WiFi.begin(ssid, password);

		while (WiFi.status() != WL_CONNECTED)
		{
			delay(100);
			Serial.println(".");
		}

		ConnectClient();
		delay(1000);
    }

    void loop()
    {
 /*       if (WiFi.status() != WL_CONNECTED)
        {
			Serial.println("WiFi lost connection");
			WiFi.begin(_ssid, _password);
        }
		else*/
		{
			if (!_client)
			{
				Serial.println("WiFi lost connection");
				ConnectClient();
			}
		}
    }

    bool sendCommand(const BookCommand& command)
    {
        if (_client)
        {
            _client.write((uint8_t*)&command, sizeof(BookCommand));

            Serial.print(" > ");
            Serial.print(command.command);
            Serial.print(" ");
            Serial.println(command.param);
            return true;
        }
        else
        {
            Serial.println("send without a client");
            return false;
        }
    }

private:
    WiFiClient _client;
	const char* _ssid;
	const char* _password;

    void ConnectClient()
    {
        IPAddress ipGateway = WiFi.gatewayIP();
        Serial.println("Gateway IP address:");
        Serial.println(ipGateway);

        for (int attempts = 0; attempts < 3; ++attempts)
        {
            if (_client.connect(ipGateway, c_PandoraPort))
            {
                break;
            }

            Serial.print(".");
            delay(1000);
        }

        if (_client)
        {
            Serial.println("Client connected");

            Serial.println("Remote IP address:");
            Serial.println(_client.remoteIP());
            Serial.println();
            WiFi.printDiag(Serial);
        }
        else
        {
            Serial.println("RESETTING");
            Serial.flush();

            ESP.reset();
            delay(1000);
        }
    }
};

