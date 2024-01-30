/*-------------------------------------------------------------------------
DFMiniMp3 library

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
/*
#include "DFMiniMp3.h"

DFMiniMp3::DFMiniMp3(Stream& serial, DfMp3Type type, uint32_t ackTimeout) :
    _serial(serial),
    _c_AckTimeout(ackTimeout),
    _comRetries(3), // default to three retries
    _isOnline(false),
#ifdef DfMiniMp3Debug
    _inTransaction(0),
#endif
    _queueNotifications(4) // default to 4 notifications in queue
{
    switch (type){
        case DfMp3Type::nochksum :
            _player = std::make_unique<Mp3ChipMH2024K16SS>();
            break;
        case DfMp3Type::incongruousNoAck :
            _player = std::make_unique<Mp3ChipIncongruousNoAck>();
            break;
        default :
            _player = std::make_unique<DFPlayerOriginal>();
    }        
}

DFMiniMp3::DFMiniMp3(Stream& serial, std::unique_ptr<Mp3ChipBase> mp3chip, uint32_t ackTimeout = DF_ACK_TIMEOUT) :
    _serial(serial),
    _c_AckTimeout(ackTimeout),
    _comRetries(3), // default to three retries
    _isOnline(false),
#ifdef DfMiniMp3Debug
    _inTransaction(0),
#endif
    _queueNotifications(4), // default to 4 notifications in queue
    _player(std::move(mp3chip)) {}

*/