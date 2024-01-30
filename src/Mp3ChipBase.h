/*-------------------------------------------------------------------------
Mp3ChipBase - base class to T_CHIP_VARIANT template features

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/DFMiniMp3)

-------------------------------------------------------------------------
This file is part of the Makuna/DFMiniMp3 library.

DFMiniMp3 is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

DFMiniMp3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with DFMiniMp3.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once
#include <stdint.h>
#include <vector>
#include "DfMp3Types.h"


// 7E FF 06 0F 00 01 01 xx xx EF
// 0	->	7E is start code
// 1	->	FF is version
// 2	->	06 is length
// 3	->	0F is command
// 4	->	00 is no receive
// 5~6	->	01 01 is argument
// 7~8	->	checksum = 0 - ( FF+06+0F+00+01+01 )
// 9	->	EF is end code
struct Mp3_Packet_WithCheckSum
{
    uint8_t startCode;
    uint8_t version;
    uint8_t length;
    uint8_t command;
    uint8_t requestAck;
    uint8_t hiByteArgument;
    uint8_t lowByteArgument;
    uint8_t hiByteCheckSum;
    uint8_t lowByteCheckSum;
    uint8_t endCode;
};

// 7E FF 06 0F 00 01 01 EF
// 0	->	7E is start code
// 1	->	FF is version
// 2	->	06 is length
// 3	->	0F is command
// 4	->	00 is no receive
// 5~6	->	01 01 is argument
// 7	->	EF is end code
struct Mp3_Packet_WithoutCheckSum
{
    uint8_t startCode;
    uint8_t version;
    uint8_t length;
    uint8_t command;
    uint8_t requestAck;
    uint8_t hiByteArgument;
    uint8_t lowByteArgument;
    uint8_t endCode;
};

using DFPlayerData = std::vector<uint8_t>;

class DFPlayerPacket {
    std::vector<uint8_t> _msg;

    uint16_t _calcChecksum();

public:
    explicit DFPlayerPacket(bool with_chksum = true);

    bool setChecksum();

    bool validateChecksum();

    bool validate();

    size_t size() const { return _msg.size(); };

    DFPlayerData& getData(){ return _msg; };

    /**
     * @brief Get commad value in packet
     * 
     * @return uint8_t 
     */
    uint8_t getCmd() const { return _msg.at(3); };

    /**
     * @brief Get command argument value
     * 
     * @return uint16_t 
     */
    uint16_t getArg() const { return ((_msg.at(5) << 8) | _msg.at(6)); };
};


class Mp3ChipBase
{

public:
    Mp3ChipBase(bool txChkSum = true, bool rxChkSum = true) : sendCheckSum(txChkSum), recvCheckSum(rxChkSum) {};
    virtual ~Mp3ChipBase(){};

    const bool sendCheckSum;
    const bool recvCheckSum;

    /**
     * @brief  returns if player supports command acknowledge 
     * pure virtual method, to be overriden from derrived class
     * @param command 
     * @return true 
     * @return false 
     */
    virtual bool commandSupportsAck( uint8_t command) = 0;

    virtual DFPlayerPacket makeTXPacket(uint8_t command, uint16_t arg, bool requestAck = false);

    virtual DFPlayerPacket makeRXPacket(){ return DFPlayerPacket(recvCheckSum); };

};

class DFPlayerOriginal : public Mp3ChipBase
{
public:
    DFPlayerOriginal() : Mp3ChipBase(){}

    //typedef Mp3_Packet_WithCheckSum SendPacket;
    //typedef Mp3_Packet_WithCheckSum ReceptionPacket;

    bool commandSupportsAck([[maybe_unused]] uint8_t command) override { return true; }
};

class Mp3ChipIncongruousNoAck : public Mp3ChipBase
{
public:
    Mp3ChipIncongruousNoAck() : Mp3ChipBase(){}

    //typedef Mp3_Packet_WithCheckSum SendPacket;
    //typedef Mp3_Packet_WithCheckSum ReceptionPacket;

    bool commandSupportsAck(uint8_t command) override
    {
        return (command > Mp3_Commands_Requests);
    }
};

class Mp3ChipMH2024K16SS : public Mp3ChipBase
{
public:
    Mp3ChipMH2024K16SS() : Mp3ChipBase(false){}
    //typedef Mp3_Packet_WithoutCheckSum SendPacket;
    //typedef Mp3_Packet_WithCheckSum ReceptionPacket;

    bool commandSupportsAck(uint8_t command) override { return true; }
};
