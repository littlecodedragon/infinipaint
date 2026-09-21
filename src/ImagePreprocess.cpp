/*  
 * InfiniPaint (fork)
 * Image insert preprocessing: freistellen + compression options.
 */

#include "ImagePreprocess.hpp"
#include "GlobalConfig.hpp"
#include <Helpers/Logger.hpp>

#include <include/codec/SkBmpDecoder.h>
#include <include/codec/SkCodec.h>
#include <include/codec/SkGifDecoder.h>
#include <include/codec/SkIcoDecoder.h>
#include <include/codec/SkJpegDecoder.h>
#include <include/codec/SkPngDecoder.h>
#include <include/codec/SkWbmpDecoder.h>
#include <include/codec/SkWebpDecoder.h>
#include <include/core/SkBitmap.h>
#include <include/core/SkData.h>
#include <include/core/SkPixmap.h>
#include <include/core/SkStream.h>
#include <include/encode/SkPngEncoder.h>
#include <include/encode/SkWebpEncoder.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <vector>

#ifndef __EMSCRIPTEN__
#include <cstdlib>
#include <array>
#endif

namespace ImagePreprocess {
namespace {

std::array<const SkCodecs::Decoder, 7> imageDecoders = {
    SkBmpDecoder::Decoder(),
    SkGifDecoder::Decoder(),
    SkIcoDecoder::Decoder(),
    SkJpegDecoder::Decoder(),
    SkPngDecoder::Decoder(),
    SkWbmpDecoder::Decoder(),
    SkWebpDecoder::Decoder()
};

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string replace_extension(std::string name, const char* newExt) {
    auto slash = name.find_last_of("/\\");
    auto baseStart = slash == std::string::npos ? 0 : slash + 1;
    auto dot = name.find_last_of('.');
    if(dot != std::string::npos && dot > baseStart)
        name = name.substr(0, dot);
    name += newExt;
    return name;
}

bool decode_first_frame(const std::string& data, SkBitmap& outBitmap) {
    sk_sp<SkData> skData = SkData::MakeWithoutCopy(data.data(), data.size());
    std::unique_ptr<SkCodec> codec = SkCodec::MakeFromData(skData, imageDecoders);
    if(!codec)
        return false;
    SkImageInfo info = codec->getInfo().makeColorType(kRGBA_8888_SkColorType).makeAlphaType(kUnpremul_SkAlphaType);
    if(!outBitmap.tryAllocPixels(info))
        return false;
    SkCodec::Options opts;
    opts.fFrameIndex = 0;
    if(codec->getPixels(info, outBitmap.getPixels(), outBitmap.rowBytes(), &opts) != SkCodec::Result::kSuccess)
        return false;
    return true;
}

void apply_white_key(SkBitmap& bitmap, int threshold) {
    threshold = std::clamp(threshold, 0, 255);
    SkPixmap pm;
    if(!bitmap.peekPixels(&pm))
        return;
    const int w = pm.width();
    const int h = pm.height();
    for(int y = 0; y < h; y++) {
        auto* row = reinterpret_cast<uint8_t*>(pm.writable_addr(0, y));
        for(int x = 0; x < w; x++) {
            uint8_t* p = row + x * 4;
            if(p[0] >= threshold && p[1] >= threshold && p[2] >= threshold) {
                p[3] = 0;
            }
            else if(p[0] > threshold - 25 && p[1] > threshold - 25 && p[2] > threshold - 25) {
                // Soft edge: fade alpha near threshold
                int minC = std::min({p[0], p[1], p[2]});
                int fade = threshold - minC;
                int alpha = (fade * 255) / 25;
                p[3] = static_cast<uint8_t>(std::clamp(std::min<int>(p[3], alpha), 0, 255));
            }
        }
    }
}

std::optional<std::string> encode_bitmap(const SkBitmap& bitmap, CompressFormat format, int webpQuality) {
    SkPixmap pm;
    if(!bitmap.peekPixels(&pm))
        return std::nullopt;
    SkDynamicMemoryWStream out;
    bool ok = false;
    if(format == CompressFormat::WEBP) {
        SkWebpEncoder::Options opts;
        opts.fCompression = SkWebpEncoder::Compression::kLossy;
        opts.fQuality = static_cast<float>(std::clamp(webpQuality, 0, 100));
        ok = SkWebpEncoder::Encode(&out, pm, opts);
    }
    else {
        ok = SkPngEncoder::Encode(&out, pm, {});
    }
    if(!ok)
        return std::nullopt;
    out.flush();
    auto skData = out.detachAsData();
    if(!skData)
        return std::nullopt;
    return std::string(reinterpret_cast<const char*>(skData->bytes()), skData->size());
}

#ifndef __EMSCRIPTEN__
std::optional<std::string> find_rembg_bin() {
    const char* envBin = std::getenv("TOWER_REMBG_BIN");
    if(envBin && envBin[0] && std::filesystem::is_regular_file(envBin))
        return std::string(envBin);
    const char* home = std::getenv("HOME");
    if(home) {
        std::filesystem::path venv = std::filesystem::path(home) / ".local/share/tower-rembg/bin/rembg";
        if(std::filesystem::is_regular_file(venv))
            return venv.string();
        std::filesystem::path local = std::filesystem::path(home) / ".local/bin/rembg";
        if(std::filesystem::is_regular_file(local))
            return local.string();
    }
    // PATH lookup via `command -v`
    FILE* pipe = popen("command -v rembg 2>/dev/null", "r");
    if(!pipe)
        return std::nullopt;
    char buf[512];
    std::string path;
    while(fgets(buf, sizeof(buf), pipe))
        path += buf;
    pclose(pipe);
    while(!path.empty() && (path.back() == '\n' || path.back() == '\r'))
        path.pop_back();
    if(!path.empty() && std::filesystem::is_regular_file(path))
        return path;
    return std::nullopt;
}

std::optional<std::string> run_rembg(const std::string& inputData, const std::string& model) {
    auto rembg = find_rembg_bin();
    if(!rembg) {
        Logger::get().log(Logger::LogType::INFO, "[ImagePreprocess] rembg not found — falling back to white-key");
        return std::nullopt;
    }
    std::error_code ec;
    auto tmpDir = std::filesystem::temp_directory_path(ec);
    if(ec)
        return std::nullopt;
    auto inPath = tmpDir / ("infinipaint-rembg-in-" + std::to_string(std::random_device{}()) + ".png");
    auto outPath = tmpDir / ("infinipaint-rembg-out-" + std::to_string(std::random_device{}()) + ".png");

    {
        std::ofstream out(inPath, std::ios::binary);
        if(!out)
            return std::nullopt;
        out.write(inputData.data(), static_cast<std::streamsize>(inputData.size()));
    }

    std::string cmd = "\"" + *rembg + "\" i -m " + model + " \"" + inPath.string() + "\" \"" + outPath.string() + "\" 2>/dev/null";
    int rc = std::system(cmd.c_str());
    std::optional<std::string> result;
    if(rc == 0 && std::filesystem::is_regular_file(outPath)) {
        std::ifstream in(outPath, std::ios::binary);
        if(in) {
            result = std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            if(result->empty())
                result.reset();
        }
    }
    std::filesystem::remove(inPath, ec);
    std::filesystem::remove(outPath, ec);
    return result;
}
#endif

} // namespace

bool looks_like_raster_image(std::string_view name, std::string_view data) {
    std::string lower = to_lower(std::string(name));
    auto hasExt = [&](const char* ext) {
        return lower.size() >= std::strlen(ext) && lower.compare(lower.size() - std::strlen(ext), std::strlen(ext), ext) == 0;
    };
    if(hasExt(".png") || hasExt(".jpg") || hasExt(".jpeg") || hasExt(".webp") || hasExt(".bmp") || hasExt(".gif") || hasExt(".ico"))
        return true;
    if(data.size() >= 8) {
        // PNG / JPEG / RIFF WEBP / GIF / BMP magic
        const auto* b = reinterpret_cast<const unsigned char*>(data.data());
        if(b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G')
            return true;
        if(b[0] == 0xFF && b[1] == 0xD8)
            return true;
        if(b[0] == 'R' && b[1] == 'I' && b[2] == 'F' && b[3] == 'F')
            return true;
        if(b[0] == 'G' && b[1] == 'I' && b[2] == 'F')
            return true;
        if(b[0] == 'B' && b[1] == 'M')
            return true;
    }
    return false;
}

Options options_from_config(const GlobalConfig& conf) {
    Options o;
    o.compressOnInsert = conf.imageCompressOnInsert;
    o.compressFormat = conf.imageCompressFormat;
    o.webpQuality = conf.imageWebpQuality;
    o.freistellenOnInsert = conf.imageFreistellenOnInsert;
    o.freistellenMode = conf.imageFreistellenMode;
    o.whiteKeyThreshold = conf.imageWhiteKeyThreshold;
    return o;
}

bool preprocess_resource(std::string& name, std::shared_ptr<std::string>& data, const Options& options) {
    if(!data || data->empty())
        return false;
    if(!looks_like_raster_image(name, *data))
        return false;

    bool wantFreistellen = options.freistellenOnInsert && options.freistellenMode != FreistellenMode::OFF;
    bool wantCompress = options.compressOnInsert && options.compressFormat != CompressFormat::KEEP;
    if(!wantFreistellen && !wantCompress)
        return false;

    std::string working = *data;
    bool changed = false;

    if(wantFreistellen) {
#ifndef __EMSCRIPTEN__
        if(options.freistellenMode == FreistellenMode::REMBG_SCHNELL || options.freistellenMode == FreistellenMode::REMBG_ALLGEMEIN) {
            const char* model = options.freistellenMode == FreistellenMode::REMBG_SCHNELL ? "silueta" : "bria-rmbg";
            // rembg wants an image file; feed current bytes (encode as PNG first if needed)
            SkBitmap bmp;
            std::string rembgInput = working;
            if(decode_first_frame(working, bmp)) {
                auto png = encode_bitmap(bmp, CompressFormat::PNG, 100);
                if(png)
                    rembgInput = *png;
            }
            if(auto rembgOut = run_rembg(rembgInput, model)) {
                working = *rembgOut;
                changed = true;
                name = replace_extension(name, ".png");
            }
            else {
                // Fallback white-key
                if(decode_first_frame(working, bmp)) {
                    apply_white_key(bmp, options.whiteKeyThreshold);
                    if(auto enc = encode_bitmap(bmp, CompressFormat::PNG, 100)) {
                        working = *enc;
                        changed = true;
                        name = replace_extension(name, ".png");
                    }
                }
            }
        }
        else
#endif
        {
            SkBitmap bmp;
            if(decode_first_frame(working, bmp)) {
                apply_white_key(bmp, options.whiteKeyThreshold);
                if(auto enc = encode_bitmap(bmp, CompressFormat::PNG, 100)) {
                    working = *enc;
                    changed = true;
                    name = replace_extension(name, ".png");
                }
            }
        }
    }

    if(wantCompress) {
        SkBitmap bmp;
        if(decode_first_frame(working, bmp)) {
            CompressFormat fmt = options.compressFormat;
            if(auto enc = encode_bitmap(bmp, fmt, options.webpQuality)) {
                // Keep result if smaller, or if freistellen required alpha-friendly format
                bool preferNew = enc->size() < working.size() || wantFreistellen || fmt == CompressFormat::WEBP;
                if(preferNew) {
                    working = *enc;
                    changed = true;
                    name = replace_extension(name, fmt == CompressFormat::WEBP ? ".webp" : ".png");
                }
            }
        }
    }

    if(changed)
        data = std::make_shared<std::string>(std::move(working));
    return changed;
}

}
