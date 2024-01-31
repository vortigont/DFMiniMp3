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

#include "DFMiniMp3.h"
#ifdef DfMiniMp3Debug
#include "Arduino.h"        // needed for Serial
#endif

DFMiniMp3::DFMiniMp3(Stream& serial, DfMp3Type type, uint32_t ackTimeout) :
    _serial(serial),
    _c_AckTimeout(ackTimeout),
    _comRetries(3), // default to three retries
    _isOnline(false)
#ifdef DfMiniMp3Debug
    ,_inTransaction(0)
#endif
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

DFMiniMp3::DFMiniMp3(Stream& serial, DfMp3Type type, std::unique_ptr<Mp3ChipBase> mp3chip, uint32_t ackTimeout) :
    _serial(serial),
    _c_AckTimeout(ackTimeout),
    _comRetries(3), // default to three retries
    _isOnline(false),
#ifdef DfMiniMp3Debug
    _inTransaction(0),
#endif
    _player(std::move(mp3chip)) {}

void DFMiniMp3::loop(){
    // process only one notification from the q at a time
    abateNotification();

    size_t pending;

    // consume all messages from Stream buffer if any
    do {
        pending = _queueNotifications.size();
        listenForReply(Mp3_Commands_None, false);
    } while (pending != _queueNotifications.size());
}

uint16_t DFMiniMp3::getCurrentTrack(DfMp3_PlaySource source){
    uint8_t command;

    switch (source)
    {
    case DfMp3_PlaySource_Usb:
        command = Mp3_Commands_GetUsbCurrentTrack;
        break;
    case DfMp3_PlaySource_Sd:
        command = Mp3_Commands_GetSdCurrentTrack;
        break;
    case DfMp3_PlaySource_Flash:
        command = Mp3_Commands_GetFlashCurrentTrack;
        break;
    default:
        command = Mp3_Commands_GetSdCurrentTrack;
        break;
    }

    return getCommand(command).arg;
}

DfMp3_Status DFMiniMp3::getStatus(){
    uint16_t reply = getCommand(Mp3_Commands_GetStatus).arg;

    DfMp3_Status status;
    status.source = static_cast<DfMp3_StatusSource>(reply >> 8);
    status.state = static_cast<DfMp3_StatusState>(reply & 0xff);

    return status;
}

uint16_t DFMiniMp3::getTotalTrackCount(DfMp3_PlaySource source){
    uint8_t command;

    switch (source)
    {
    case DfMp3_PlaySource_Usb:
        command = Mp3_Commands_GetUsbTrackCount;
        break;
    case DfMp3_PlaySource_Sd:
        command = Mp3_Commands_GetSdTrackCount;
        break;
    case DfMp3_PlaySource_Flash:
        command = Mp3_Commands_GetFlashTrackCount;
        break;
    default:
        command = Mp3_Commands_GetSdTrackCount;
        break;
    }

    return getCommand(command).arg;
}

void DFMiniMp3::abateNotification(){
    if (_queueNotifications.size()){
        callNotification(_queueNotifications.front());
        _queueNotifications.pop_front();
    }
}

void DFMiniMp3::callNotification(reply_t reply){
    switch (reply.command)
    {
    case Mp3_Replies_TrackFinished_Usb: // usb
        if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Usb, reply.arg);
        break;

    case Mp3_Replies_TrackFinished_Sd: // micro sd
        if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Sd, reply.arg);
        break;

    case Mp3_Replies_TrackFinished_Flash: // flash
        if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Flash, reply.arg);
        break;

    case Mp3_Replies_PlaySource_Online:
    case Mp3_Replies_PlaySource_Inserted:
    case Mp3_Replies_PlaySource_Removed:
        if (_cb_OnPlaySourceEvent) _cb_OnPlaySourceEvent(static_cast<DfMp3_SourceEvent>(reply.command), static_cast<DfMp3_PlaySources>(reply.arg));
        break;

    case Mp3_Replies_Error: // error
        if (_cb_OnErr) _cb_OnErr(reply.arg);
        break;

    default:
#ifdef DfMiniMp3Debug
        DfMiniMp3Debug.print("DFP: INVALID NOTIFICATION: ");
        reply.printReply();
        DfMiniMp3Debug.println();
#endif
        break;
    }
}

void DFMiniMp3::sendPacket(uint8_t command, uint16_t arg, bool requestAck){
    //typename T_CHIP_VARIANT::SendPacket packet = T_CHIP_VARIANT::generatePacket(command, arg, requestAck);
    DFPlayerPacket packet = _player->makeTXPacket(command, arg, requestAck);

#ifdef DfMiniMp3Debug
    DfMiniMp3Debug.print("OUT ");
    printRawPacket(packet.getData());
    //printRawPacket(reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    DfMiniMp3Debug.println();
#endif

    //_serial.write(reinterpret_cast<uint8_t*>(&packet), sizeof(packet));
    _serial.write(packet.getData().data(), packet.size());
}

bool DFMiniMp3::readPacket(reply_t* reply, bool wait4data){
    //typename T_CHIP_VARIANT::ReceptionPacket in;
    DFPlayerPacket inpkt = _player->makeRXPacket();

    // do not block on read if not required
    if (_serial.available() < inpkt.getData().size() && !wait4data ){
        return false;
    }

    // init our out args always
    *reply = {};

    size_t readlen = _serial.readBytes(inpkt.getData().data(), inpkt.size());

#ifdef DfMiniMp3Debug
    DfMiniMp3Debug.print("IN ");
    printRawPacket(inpkt.getData());
    //printRawPacket(reinterpret_cast<const uint8_t*>(&in), read);
    DfMiniMp3Debug.println();
#endif

    if (readlen < inpkt.size())
    {
#ifdef DfMiniMp3Debug
        DfMiniMp3Debug.println("DF: bad size pkt");
#endif
        // not enough bytes, corrupted packet
        reply->arg = DfMp3_Error_PacketSize;
        return false;
    }

    if (!inpkt.validate())
    {
#ifdef DfMiniMp3Debug
        DfMiniMp3Debug.println("DF: invalid pkt");
#endif
        // invalid version or corrupted packet
        reply->arg = DfMp3_Error_PacketHeader;
        // either we received garbage or lost/missed packet header, won't look for it, just flush stream and retry
        _serial.flush();
        return false;
    }

    if (!inpkt.validateChecksum())
    {
#ifdef DfMiniMp3Debug
        DfMiniMp3Debug.print("DF: bad csum");
#endif
        // checksum failed, corrupted packet
        reply->arg = DfMp3_Error_PacketChecksum;
        return false;
    }

    reply->command = inpkt.getCmd();
    reply->arg = inpkt.getArg();

    return true;
}

DFMiniMp3::reply_t DFMiniMp3::retryCommand(uint8_t command, uint8_t expectedCommand, uint16_t arg, bool requestAck){
    reply_t reply;
    uint8_t retries = _comRetries;


#ifdef DfMiniMp3Debug
    if (_inTransaction != 0)
    {
        DfMiniMp3Debug.print("Rentrant? _inTransaction ");
        DfMiniMp3Debug.print(_inTransaction);
    }
    else
#endif
    {
        loop();
    }

#ifdef DfMiniMp3Debug
    _inTransaction++;
#endif
    if (_player->commandSupportsAck(command))
    {
        // with ack support, 
        // we may retry if we don't get what we expected
        //
        _serial.setTimeout(_c_AckTimeout);
        do
        {
            sendPacket(command, arg, requestAck);
            reply = listenForReply(expectedCommand);
            retries--;
        } while (reply.command != expectedCommand && retries);
    }
    else
    {
        // without ack support, 
        // we may retry only if we get an error
        //
        _serial.setTimeout(c_NoAckTimeout);
        do
        {
            sendPacket(command, arg, requestAck);
            reply = listenForReply(expectedCommand);
            retries--;
        } while (reply.command == Mp3_Replies_Error && retries);
    }
#ifdef DfMiniMp3Debug
    _inTransaction--;
#endif

    if (reply.command == Mp3_Replies_Error)
    {
        if (_cb_OnErr) _cb_OnErr(reply.arg);
        reply = {};
    }
    
    return reply;
}

DFMiniMp3::reply_t DFMiniMp3::listenForReply(uint8_t command, bool wait4data){
    reply_t reply;

    while (readPacket(&reply, wait4data))
    {
        switch (reply.command)
        {
        case Mp3_Replies_PlaySource_Online: // play source online
        case Mp3_Replies_PlaySource_Inserted: // play source inserted
        case Mp3_Replies_PlaySource_Removed: // play source removed
            _isOnline = true;
            _queueNotifications.push_back(reply);
            break;

        case Mp3_Replies_TrackFinished_Usb: // usb
        case Mp3_Replies_TrackFinished_Sd: // micro sd
        case Mp3_Replies_TrackFinished_Flash: // flash
            _queueNotifications.push_back(reply);
            break;

        case Mp3_Replies_Error: // error
            if (command == Mp3_Commands_None)
                _queueNotifications.push_back(reply);
            else
                return reply;
            break;

        case Mp3_Replies_Ack: // ack
        default:
            if (command != Mp3_Commands_None)
            {
                return reply;
            }
            break;
        }

        // for not specific listen, only drain
        // one message at a time
        if (command == Mp3_Commands_None)
        {
            break;
        }
    }


    return {};
}


#ifdef DfMiniMp3Debug
void DFMiniMp3::printRawPacket(const DFPlayerData& data)
{
    for (auto &c : data){
        DfMiniMp3Debug.printf("%02X ", c);
    }
}

void DFMiniMp3::reply_t::printReply() const {
    DfMiniMp3Debug.printf(" %02x", command);
    char formated[8];
    snprintf(formated, 8, " %04x", arg);
    DfMiniMp3Debug.print(formated);
}
#endif





