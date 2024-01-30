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
#pragma once

#include <functional>
#include <memory>
#include "Mp3ChipBase.h"
#include "Stream.h"
#include "internal/queueSimple.h"

#define DF_ACK_TIMEOUT   900

using DF_OnPlayFinished = std::function< void (DfMp3_PlaySources source, uint16_t track)>;
using DF_OnPlaySourceEvent = std::function< void (DfMp3_SourceEvent event, DfMp3_PlaySources source)>;
using DF_OnError = std::function< void (uint16_t errorCode)>;


class DFMiniMp3
{
public:
    explicit DFMiniMp3(Stream& serial, DfMp3Type type = DfMp3Type::origin, uint32_t ackTimeout = DF_ACK_TIMEOUT) :
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

    /**
     * @brief Construct a new DFMiniMp3 object with user-provided Mp3Chip handler class
     * all arguments are mandatory to avoid overload with know-types
     * @param serial - serial port stream object
     * @param type - should be DfMp3Type::unknown
     * @param mp3chip - unique pointer to the pobject derived from Mp3ChipBase, constructor will take the ownership of the object
     * @param ackTimeout - aknowledge timemout, could be DF_ACK_TIMEOUT by default
     */
    explicit DFMiniMp3(Stream& serial, DfMp3Type type, std::unique_ptr<Mp3ChipBase> mp3chip, uint32_t ackTimeout) :
            _serial(serial),
            _c_AckTimeout(ackTimeout),
            _comRetries(3), // default to three retries
            _isOnline(false),
        #ifdef DfMiniMp3Debug
            _inTransaction(0),
        #endif
            _queueNotifications(4), // default to 4 notifications in queue
            _player(std::move(mp3chip)) {}


    DFMiniMp3(Stream& serial, std::unique_ptr<Mp3ChipBase> mp3chip, uint32_t ackTimeout = DF_ACK_TIMEOUT);

    void setComRetries(uint8_t retries)
    {
        _comRetries = retries;
    }

    void loop()
    {
        // call all outstanding notifications
        while (abateNotification());

        // check for any new notifications in comms
        uint8_t maxDrains = 6;

        while (maxDrains)
        {
            listenForReply(Mp3_Commands_None, false);
            maxDrains--;
        }
    }

    // Does not work with all models.
    // 0x3f reply overlaps the play source online notification
    // so this is why I suspect this has been deprecated and should
    // be avoided as there is not a way to tell a notification from a value return
    // YX5200-24SS - sends reply
    // MH2024K-24SS - sends NO reply --> results in error notification
/*
    [[deprecated("Command in conflict with notification with no valid solution.")]]
    DfMp3_PlaySources getPlaySources()
    {
        return getCommand(Mp3_Commands_GetPlaySources).arg;
    }
*/
    uint16_t getSoftwareVersion()
    {
        return getCommand(Mp3_Commands_GetSoftwareVersion).arg;
    }

    // the track as enumerated across all folders
    void playGlobalTrack(uint16_t track = 0)
    {
        setCommand(Mp3_Commands_PlayGlobalTrack, track);
    }

    // sd:/mp3/####track name
    void playMp3FolderTrack(uint16_t track)
    {
        setCommand(Mp3_Commands_PlayMp3FolderTrack, track);
    }

    // older devices: sd:/###/###track name
    // newer devices: sd:/##/###track name
    // folder and track numbers are zero padded
    void playFolderTrack(uint8_t folder, uint8_t track)
    {
        uint16_t arg = (folder << 8) | track;
        setCommand(Mp3_Commands_PlayFolderTrack, arg);
    }

    // sd:/##/####track name
    // track number must be four digits, zero padded
    void playFolderTrack16(uint8_t folder, uint16_t track)
    {
        uint16_t arg = (static_cast<uint16_t>(folder) << 12) | track;
        setCommand(Mp3_Commands_PlayFolderTrack16, arg);
    }

    void playRandomTrackFromAll()
    {
        setCommand(Mp3_Commands_PlayRandmomGlobalTrack);
    }

    void nextTrack()
    {
        setCommand(Mp3_Commands_PlayNextTrack);
    }

    void prevTrack()
    {
        setCommand(Mp3_Commands_PlayPrevTrack);
    }

    uint16_t getCurrentTrack(DfMp3_PlaySource source = DfMp3_PlaySource_Sd)
    {
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

    // 0- 30
    void setVolume(uint8_t volume)
    {
        setCommand(Mp3_Commands_SetVolume, volume);
    }

    uint8_t getVolume()
    {
        return getCommand(Mp3_Commands_GetVolume).arg;
    }

    void increaseVolume()
    {
        setCommand(Mp3_Commands_IncVolume);
    }

    void decreaseVolume()
    {
        setCommand(Mp3_Commands_DecVolume);
    }

    // useless, removed
    // 0-31
    /*
    void setVolume(bool mute, uint8_t volume)
    {
        uint16_t arg = (!mute << 8) | volume;
        setCommand(0x10, arg);
    }
    */

    void loopGlobalTrack(uint16_t globalTrack)
    {
        setCommand(Mp3_Commands_LoopGlobalTrack, globalTrack);
    }

    // sd:/##/*
    // 0-99
    void loopFolder(uint8_t folder)
    {
        setCommand(Mp3_Commands_LoopInFolder, folder);
    }

    // not well supported, use at your own risk
    void setPlaybackMode(DfMp3_PlaybackMode mode)
    {
        setCommand(Mp3_Commands_SetPlaybackMode, mode);
    }

    DfMp3_PlaybackMode getPlaybackMode()
    {
        return static_cast<DfMp3_PlaybackMode>(getCommand(Mp3_Commands_GetPlaybackMode).arg);
    }

    void setRepeatPlayAllInRoot(bool repeat)
    {
        setCommand(Mp3_Commands_RepeatPlayInRoot, !!repeat);
    }

    void setRepeatPlayCurrentTrack(bool repeat)
    {
        setCommand(Mp3_Commands_RepeatPlayCurrentTrack, !repeat);
    }

    void setEq(DfMp3_Eq eq)
    {
        setCommand(Mp3_Commands_SetEq, eq);
    }

    DfMp3_Eq getEq()
    {
        return static_cast<DfMp3_Eq>(getCommand(Mp3_Commands_GetEq).arg);
    }

    void setPlaybackSource(DfMp3_PlaySource source)
    {
        setCommand(Mp3_Commands_SetPlaybackSource, source);
    }

    void sleep()
    {
        setCommand(Mp3_Commands_Sleep);
    }

    void awake()
    {
        setCommand(Mp3_Commands_Awake);
    }

    /**
     * @brief send reset command to player
     * this call will return immidiately. Checking when (if) player will get back on-line should be done via event callbacks
     */
    void reset()
    {
        _isOnline = false;
        setCommand(Mp3_Commands_Reset);
    }

    void start()
    {
        setCommand(Mp3_Commands_Start);
    }

    void pause()
    {
        setCommand(Mp3_Commands_Pause);
    }

    void stop()
    {
        setCommand(Mp3_Commands_Stop);
    }

    DfMp3_Status getStatus()
    {
        uint16_t reply = getCommand(Mp3_Commands_GetStatus).arg;

        DfMp3_Status status;
        status.source = static_cast<DfMp3_StatusSource>(reply >> 8);
        status.state = static_cast<DfMp3_StatusState>(reply & 0xff);

        return status;
    }

    uint16_t getFolderTrackCount(uint16_t folder)
    {
        return getCommand(Mp3_Commands_GetFolderTrackCount, folder).arg;
    }

    uint16_t getTotalTrackCount(DfMp3_PlaySource source = DfMp3_PlaySource_Sd)
    {
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

    uint16_t getTotalFolderCount()
    {
        return getCommand(Mp3_Commands_GetTotalFolderCount).arg;
    }

    // sd:/advert/####track name
    void playAdvertisement(uint16_t track)
    {
        setCommand(Mp3_Commands_PlayAdvertTrack, track);
    }

    void stopAdvertisement()
    {
        setCommand(Mp3_Commands_StopAdvert);
    }

    void enableDac()
    {
        setCommand(Mp3_Commands_SetDacInactive, 0x00);
    }

    void disableDac()
    {
        setCommand(Mp3_Commands_SetDacInactive, 0x01);
    }

    bool isOnline() const
    {
        return _isOnline;
    }

    /**
     * @brief set functional call-back for PlayFinished event
     * 
     * @param cb - callback function void (DfMp3_PlaySources source, uint16_t track)
     * @return * void 
     */
    void onPlayFinished(DF_OnPlayFinished cb){ _cb_OnPlayFinished = cb; }

    /**
     * @brief set functional call-back for PlaySource event
     * 
     * @param cb - callback function DfMp3_PlaySources source, DfMp3_SourceEvent event
     * @return * void 
     */
    void onPlaySource(DF_OnPlaySourceEvent cb){ _cb_OnPlaySourceEvent = cb; }

    /**
     * @brief set functional call-back for PlayFinished event
     * 
     * @param cb - void (uint16_t errorCode)
     * @return * void 
     */
    void onError(DF_OnError cb){ _cb_OnErr = cb; }


private:
    struct reply_t
    {
        uint8_t command = 0;
        uint16_t arg = 0;

        bool isUndefined()
        {
            return (command == Mp3_Commands_None);
        }

#ifdef DfMiniMp3Debug
        void printReply() const
        {
            char formated[8];

            sprintf(formated, " %02x", command);
            DfMiniMp3Debug.print(formated);
            sprintf(formated, " %04x", arg);
            DfMiniMp3Debug.print(formated);
        }
#endif
    };

    // Serial port object for communicating with player
    Stream& _serial;
    const uint32_t _c_AckTimeout;
    const uint32_t c_NoAckTimeout = 50; // 30ms observerd, added a little overhead

    uint8_t _comRetries;
    volatile bool _isOnline;
#ifdef DfMiniMp3Debug
    int8_t _inTransaction;
#endif
    queueSimple_t<reply_t> _queueNotifications;

    // Chip handler instance
    std::unique_ptr<Mp3ChipBase> _player;

    // func callback pointers
    DF_OnPlayFinished _cb_OnPlayFinished = nullptr;
    DF_OnPlaySourceEvent _cb_OnPlaySourceEvent = nullptr;
    DF_OnError _cb_OnErr = nullptr;

    void appendNotification(reply_t reply)
    {
        // store the notification for later calling so
        // current comms transactions can be finished
        // without interruption
        _queueNotifications.Enqueue(reply);
    }

    bool abateNotification()
    {
        // remove the first notification and call it
        reply_t reply;
        bool wasAbated = false;
        if (_queueNotifications.Dequeue(&reply))
        {
            callNotification(reply);
            wasAbated = true;
        }
        return wasAbated;
    }

    void callNotification(reply_t reply)
    {
        switch (reply.command)
        {
        case Mp3_Replies_TrackFinished_Usb: // usb
            if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Usb, reply.arg);
            break;

        case Mp3_Replies_TrackFinished_Sd: // micro sd
            if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Sd, reply.arg);
            //T_NOTIFICATION_METHOD::OnPlayFinished(*this, DfMp3_PlaySources_Sd, reply.arg);
            break;

        case Mp3_Replies_TrackFinished_Flash: // flash
            if (_cb_OnPlayFinished) _cb_OnPlayFinished(DfMp3_PlaySources_Flash, reply.arg);
            //T_NOTIFICATION_METHOD::OnPlayFinished(*this, DfMp3_PlaySources_Flash, reply.arg);
            break;

        case Mp3_Replies_PlaySource_Online:
        case Mp3_Replies_PlaySource_Inserted:
        case Mp3_Replies_PlaySource_Removed:
            if (_cb_OnPlaySourceEvent) _cb_OnPlaySourceEvent(static_cast<DfMp3_SourceEvent>(reply.command), static_cast<DfMp3_PlaySources>(reply.arg));
            //T_NOTIFICATION_METHOD::OnPlaySourceOnline(*this, static_cast<DfMp3_PlaySources>(reply.arg));
            break;

        case Mp3_Replies_Error: // error
            if (_cb_OnErr) _cb_OnErr(reply.arg);
            //T_NOTIFICATION_METHOD::OnError(*this, reply.arg);
            break;

        default:
#ifdef DfMiniMp3Debug
            DfMiniMp3Debug.print("INVALID NOTIFICATION: ");
            reply.printReply();
            DfMiniMp3Debug.println();
#endif
            break;
        }
    }

    void drainResponses()
    {
        loop();
    }

    void sendPacket(uint8_t command, uint16_t arg = 0, bool requestAck = false)
    {
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

    bool readPacket(reply_t* reply, bool wait4data = true)
    {
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
            DfMiniMp3Debug.println("DF: bad size pkt");
            // not enough bytes, corrupted packet
            reply->arg = DfMp3_Error_PacketSize;
            return false;
        }

        if (!inpkt.validate())
        {
            DfMiniMp3Debug.println("DF: invalid pkt");
            // invalid version or corrupted packet
            reply->arg = DfMp3_Error_PacketHeader;
            // either we received garbage or lost/missed packet header, won't look for it, just flush stream and retry
            _serial.flush();
            return false;
        }

        if (!inpkt.validateChecksum())
        {
            DfMiniMp3Debug.print("DF: bad csum");
            // checksum failed, corrupted packet
            reply->arg = DfMp3_Error_PacketChecksum;
            return false;
        }

        reply->command = inpkt.getCmd();
        reply->arg = inpkt.getArg();

        return true;
    }

    reply_t retryCommand(uint8_t command, 
            uint8_t expectedCommand, 
            uint16_t arg = 0, 
            bool requestAck = false)
    {
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
            drainResponses();
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

    reply_t getCommand(uint8_t command, uint16_t arg = 0)
    {
        return retryCommand(command, command, arg);
    }

    void setCommand(uint8_t command, uint16_t arg = 0)
    {
        retryCommand(command, Mp3_Replies_Ack, arg, true);
    }

    reply_t listenForReply(uint8_t command, bool wait4data = true)
    {
        reply_t reply;

        while (readPacket(&reply, wait4data))
        {
            switch (reply.command)
            {
            case Mp3_Replies_PlaySource_Online: // play source online
            case Mp3_Replies_PlaySource_Inserted: // play source inserted
            case Mp3_Replies_PlaySource_Removed: // play source removed
                _isOnline = true;
                appendNotification(reply);
                break;

            case Mp3_Replies_TrackFinished_Usb: // usb
            case Mp3_Replies_TrackFinished_Sd: // micro sd
            case Mp3_Replies_TrackFinished_Flash: // flash
                appendNotification(reply);
                break;

            case Mp3_Replies_Error: // error
                if (command == Mp3_Commands_None)
                {
                    appendNotification(reply);
                }
                else
                {
                    return reply;
                }
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
    void printRawPacket(const DFPlayerData& data)
    {
        for (auto &c : data){
            DfMiniMp3Debug.printf("%02X ", c);
        }
    }

#endif
};
