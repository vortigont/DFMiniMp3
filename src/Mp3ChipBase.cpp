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

#include "Mp3ChipBase.h"

#define DF_PKT_W_CSUM_SIZE      10     // size of packet with checksum
#define DF_PKT_WO_CSUM_SIZE     8      // size of packet with out checksum
#define DF_CSUM_DATALEN_SIZE    6      // size of data where checksum is applied
#define CSUM_DATA_OFFSET        1      // offset where to start calculating checksum

// c-tor
DFPlayerPacket::DFPlayerPacket(bool with_chksum) {
    if (with_chksum)
        _msg = {0x7E, 0xFF, 0x06, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
    else
        _msg = {0x7E, 0xFF, 0x06, 0x0F, 0x00, 0x00, 0x00, 0xEF};
}

uint16_t DFPlayerPacket::_calcChecksum(){
    uint16_t sum = 0xFFFF;
    for (int idx = 0; idx != DF_CSUM_DATALEN_SIZE; ++idx){
        sum -= _msg.at(idx+CSUM_DATA_OFFSET);
    }
    return sum + 1;
}

bool DFPlayerPacket::setChecksum(){
    // validate packets only with proper size
    if (_msg.size() != DF_PKT_W_CSUM_SIZE) return false;

    uint16_t sum = _calcChecksum();
    Mp3_Packet_WithCheckSum *out = reinterpret_cast<Mp3_Packet_WithCheckSum*>(_msg.data());
    out->hiByteCheckSum = (sum >> 8);
    out->lowByteCheckSum = (sum & 0xff);
    return true;
}

bool DFPlayerPacket::validateChecksum(){
    // validate packets only with proper size, otherwise return 'true' as we assume that packets w/o checksum are always valid
    if (_msg.size() == DF_PKT_WO_CSUM_SIZE) return true;

    Mp3_Packet_WithCheckSum *in = reinterpret_cast<Mp3_Packet_WithCheckSum*>(_msg.data());

    return (_calcChecksum() == ((in->hiByteCheckSum) << 8) | in->lowByteCheckSum);
}

bool DFPlayerPacket::validate(){
    return (_msg.at(0) == Mp3_PacketStartCode &&
            _msg.at(1) == Mp3_PacketVersion &&
            _msg.at(2) == DF_CSUM_DATALEN_SIZE &&               // not sure if this is valid for packet's w/o checksum, looks pretty strange
            _msg.at(_msg.size()-1) == Mp3_PacketEndCode
    );
}

DFPlayerPacket Mp3ChipBase::makeTXPacket(uint8_t command, uint16_t arg, bool requestAck){
    DFPlayerPacket pkt(sendCheckSum);
    Mp3_Packet_WithCheckSum *out = reinterpret_cast<Mp3_Packet_WithCheckSum*>(pkt.getData().data());
    out->command = command;
    out->requestAck = requestAck;
    out->hiByteArgument = static_cast<uint8_t>(arg >> 8);
    out->lowByteArgument = static_cast<uint8_t>(arg & 0xff);
    pkt.setChecksum();
    return pkt;
}

