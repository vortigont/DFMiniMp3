/*-------------------------------------------------------------------------
DfMp3Types - API exposed support enums and structs

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

#define Mp3_PacketStartCode 0x7e
#define Mp3_PacketVersion 0xff
#define Mp3_PacketEndCode 0xef


enum DfMp3_Error
{
    //                              alternative meanings depending on chip
    // from device                
    DfMp3_Error_Busy = 1,         //  busy                  busy
    DfMp3_Error_Sleeping,         //  frame not received    sleep
    DfMp3_Error_SerialWrongStack, //  verification error    frame not received
    DfMp3_Error_CheckSumNotMatch, //                        checksum
    DfMp3_Error_FileIndexOut,     //  folder out of scope   track out of scope
    DfMp3_Error_FileMismatch,     //  folder not found      track not found
    DfMp3_Error_Advertise,        //                        only allowed while playing     advertisement not allowed
    DfMp3_Error_SdReadFail,       //                        SD card failed
    DfMp3_Error_FlashReadFail,    //                        Flash mem failed
    DfMp3_Error_EnteredSleep = 10, //                        entered sleep    
    // from library
    DfMp3_Error_RxTimeout = 0x81,
    DfMp3_Error_PacketSize,
    DfMp3_Error_PacketHeader,
    DfMp3_Error_PacketChecksum,
    DfMp3_Error_General = 0xff
};

enum DfMp3_PlaybackMode
{
    DfMp3_PlaybackMode_Repeat,
    DfMp3_PlaybackMode_FolderRepeat,
    DfMp3_PlaybackMode_SingleRepeat,
    DfMp3_PlaybackMode_Random
};

enum DfMp3_Eq
{
    DfMp3_Eq_Normal,
    DfMp3_Eq_Pop,
    DfMp3_Eq_Rock,
    DfMp3_Eq_Jazz,
    DfMp3_Eq_Classic,
    DfMp3_Eq_Bass
};

enum DfMp3_PlaySource // value - only one can be set
{
    DfMp3_PlaySource_Usb = 1,
    DfMp3_PlaySource_Sd,
    DfMp3_PlaySource_Aux,
    DfMp3_PlaySource_Sleep,
    DfMp3_PlaySource_Flash
};

enum DfMp3_PlaySources // bitfield - more than one can be set
{
    DfMp3_PlaySources_Usb = 0x01,
    DfMp3_PlaySources_Sd = 0x02,
    DfMp3_PlaySources_Pc = 0x04,
    DfMp3_PlaySources_Flash = 0x08,
};

enum DfMp3_StatusState
{
    DfMp3_StatusState_Idle = 0x00,
    DfMp3_StatusState_Playing = 0x01,
    DfMp3_StatusState_Paused = 0x02,
    DfMp3_StatusState_Sleep = 0x08, // note, some chips use DfMp3_StatusSource_Sleep
    DfMp3_StatusState_Shuffling = 0x11, // not documented, but discovered
};

enum DfMp3_StatusSource
{
    DfMp3_StatusSource_General = 0x00,
    DfMp3_StatusSource_Usb = 0x01,
    DfMp3_StatusSource_Sd = 0x02,
    DfMp3_StatusSource_Sleep = 0x10,
};

struct DfMp3_Status
{
    DfMp3_StatusSource source;
    DfMp3_StatusState state;
};

enum class DfMp3_SourceEvent {
    removed,
    inserted,
    online
};

enum class DfMp3Type {
    origin,
    incongruousNoAck,
    nochksum,
    unknown
};

enum Mp3_Commands
{
    Mp3_Commands_None = 0x00,
    Mp3_Commands_PlayNextTrack = 0x01,
    Mp3_Commands_PlayPrevTrack = 0x02,
    Mp3_Commands_PlayGlobalTrack = 0x03,
    Mp3_Commands_IncVolume = 0x04,
    Mp3_Commands_DecVolume = 0x05,
    Mp3_Commands_SetVolume = 0x06,
    Mp3_Commands_SetEq = 0x07,
    Mp3_Commands_LoopGlobalTrack = 0x08,
    Mp3_Commands_SetPlaybackMode = 0x08,
    Mp3_Commands_SetPlaybackSource = 0x09,
    Mp3_Commands_Sleep = 0x0a,
    Mp3_Commands_Awake = 0x0b,
    Mp3_Commands_Reset = 0x0c,
    Mp3_Commands_Start = 0x0d,
    Mp3_Commands_Pause = 0x0e,
    Mp3_Commands_PlayFolderTrack = 0x0f,
    Mp3_Commands_RepeatPlayInRoot = 0x11,
    Mp3_Commands_PlayMp3FolderTrack = 0x12,
    Mp3_Commands_PlayAdvertTrack = 0x13,
    Mp3_Commands_PlayFolderTrack16 = 0x14,
    Mp3_Commands_StopAdvert = 0x15,
    Mp3_Commands_Stop = 0x16,
    Mp3_Commands_LoopInFolder = 0x17,
    Mp3_Commands_PlayRandmomGlobalTrack = 0x18,
    Mp3_Commands_RepeatPlayCurrentTrack = 0x19,
    Mp3_Commands_SetDacInactive = 0x1a,
    Mp3_Commands_Requests = 0x30, // after this is all request, before all actions
    Mp3_Commands_GetPlaySources = 0x3f, // deprecated due to conflict with replies
    Mp3_Commands_GetStatus = 0x42,
    Mp3_Commands_GetVolume = 0x43,
    Mp3_Commands_GetEq = 0x44,
    Mp3_Commands_GetPlaybackMode = 0x45,
    Mp3_Commands_GetSoftwareVersion = 0x46,
    Mp3_Commands_GetUsbTrackCount = 0x47,
    Mp3_Commands_GetSdTrackCount = 0x48,
    Mp3_Commands_GetFlashTrackCount = 0x49,
    Mp3_Commands_GetUsbCurrentTrack = 0x4b,
    Mp3_Commands_GetSdCurrentTrack = 0x4c,
    Mp3_Commands_GetFlashCurrentTrack = 0x4d,
    Mp3_Commands_GetFolderTrackCount = 0x4e,
    Mp3_Commands_GetTotalFolderCount = 0x4f,
};


enum Mp3_Replies
{
    Mp3_Replies_PlaySource_Inserted = 0x3a,
    Mp3_Replies_PlaySource_Removed = 0x3b,
    Mp3_Replies_TrackFinished_Usb = 0x3c,
    Mp3_Replies_TrackFinished_Sd = 0x3d,
    Mp3_Replies_TrackFinished_Flash = 0x3e,
    Mp3_Replies_PlaySource_Online = 0x3f,
    Mp3_Replies_Error = 0x40,
    Mp3_Replies_Ack = 0x41,
};
