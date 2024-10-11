#pragma once
#include "Encoder.h"


#include <Windows.h>
#include <Vfw.h>

class AVIEncoder final : public Encoder
{
public:
    bool start(Params params) override;
    bool stop() override;
    bool append_video(uint8_t* image) override;
    bool append_audio(uint8_t* audio, size_t length) override;

private:
    static constexpr auto MAX_AVI_SIZE = 0x7B9ACA00;

    bool save_options();
    bool load_options();

    Params m_params{};
    AVICOMPRESSOPTIONS* m_avi_options = new AVICOMPRESSOPTIONS();

    bool m_splitting = false;
    size_t m_splits = 0;

    size_t m_frame = 0;
    size_t m_sample = 0;

    BITMAPINFOHEADER m_info_hdr{};
    PAVIFILE m_avi_file{};

    AVISTREAMINFO m_video_stream_hdr{};
    PAVISTREAM m_video_stream{};
    PAVISTREAM m_compressed_video_stream{};

    AVISTREAMINFO m_sound_stream_hdr{};
    PAVISTREAM m_sound_stream{};
    WAVEFORMATEX m_sound_format{};

    size_t m_avi_file_size = 0;
};
