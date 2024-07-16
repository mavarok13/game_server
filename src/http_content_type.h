#pragma once

#include <string_view>

namespace http_content_type {

constexpr static char HTML[] = "text/html";
constexpr static char CSS[] = "text/css";
constexpr static char TEXT[] = "text/plain";
constexpr static char JS[] = "text/javascript";
constexpr static char JSON[] = "application/json";
constexpr static char XML[] = "application/xml";
constexpr static char PNG[] = "image/png";
constexpr static char JPEG[] = "image/jpeg";
constexpr static char GIF[] = "image/gif";
constexpr static char BMP[] = "image/bmp";
constexpr static char ICO[] = "image/vnd.microsoft.icon";
constexpr static char TIFF[] = "image/tiff";
constexpr static char SVG[] = "image/svg+xml";
constexpr static char MP3[] = "audio/mpeg";
constexpr static char EMPTY[] = "application/octet-stream";

std::string_view GetContentTypeByExtension(std::string_view type);

} //namespace http_content_type