#pragma once

const uint16_t c_PandoraPort = 777;

struct BookCommand
{
    uint8_t command;
    uint8_t param;
};

#include "PandoraClient.h"
#include "PandoraServer.h"
