/*  
 * InfiniPaint (fork)
 * Image insert preprocessing: freistellen + compression options.
 */

#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

class GlobalConfig;

namespace ImagePreprocess {

enum class CompressFormat : uint8_t {
    KEEP = 0,
    WEBP,
    PNG
};

enum class FreistellenMode : uint8_t {
    OFF = 0,
    WHITE_KEY,       // Near-white → transparent (always available)
    REMBG_SCHNELL,   // rembg -m silueta (Tower freistellen/schnell)
    REMBG_ALLGEMEIN  // rembg -m bria-rmbg (Tower freistellen/allgemein)
};

struct Options {
    bool compressOnInsert = true;
    CompressFormat compressFormat = CompressFormat::WEBP;
    int webpQuality = 80; // 0–100, lossy
    bool freistellenOnInsert = false;
    FreistellenMode freistellenMode = FreistellenMode::WHITE_KEY;
    int whiteKeyThreshold = 245; // 0–255, RGB channels all >= threshold → transparent
};

// Mutates name + data in place when preprocessing applies. Returns true if changed.
bool preprocess_resource(std::string& name, std::shared_ptr<std::string>& data, const Options& options);

Options options_from_config(const GlobalConfig& conf);

bool looks_like_raster_image(std::string_view name, std::string_view data);

}
