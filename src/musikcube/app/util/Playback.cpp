//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2004-2023 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include <stdafx.h>
#include "Playback.h"

#include <musikcore/sdk/constants.h>

using namespace musik::core;
using namespace musik::core::audio;
using namespace musik::core::sdk;
using namespace musik::cube;

static const size_t NO_SELECTION = (size_t) -1;

static size_t getSelectedIndex(TrackListView& trackList) {
    auto tracks = trackList.GetTrackList();
    size_t index = 0;

    if (tracks && tracks->Count()) {
        if (trackList.IsFocused()) {
            index = trackList.GetSelectedTrackIndex();
            if (index == cursespp::ListWindow::NO_SELECTION) {
                return NO_SELECTION;
            }
        }
    }

    return index;
}

namespace musik {
    namespace cube {
        namespace playback {
            void PlaySelected(TrackListView& trackList, PlaybackService& playback) {
                auto index = getSelectedIndex(trackList);
                if (index != NO_SELECTION) {
                    auto tracks = trackList.GetTrackList();
                    playback.Play(*tracks, index);
                }
            }

            void PlayFromTop(TrackListView& trackList, PlaybackService& playback) {
                auto tracks = trackList.GetTrackList();
                if (tracks && tracks->Count()) {
                    playback.Play(*tracks, 0);
                }
            }

            bool HotSwap(TrackListView& trackList, PlaybackService& playback) {
                auto index = getSelectedIndex(trackList);
                if (index != NO_SELECTION) {
                    auto tracks = trackList.GetTrackList();
                    return playback.HotSwap(*tracks, index);
                }
                return false;
            }
        }
    }
}
