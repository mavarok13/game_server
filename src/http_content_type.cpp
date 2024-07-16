#include "http_content_type.h"

namespace http_content_type {

std::string_view GetContentTypeByExtension(std::string_view extension) {

    if(extension == ".txt") {
        return http_content_type::TEXT; 
    }
    else if (extension == ".htm" || extension == ".html") {
        return http_content_type::HTML; 
    }
    else if (extension == ".css") {
        return http_content_type::CSS; 
    } 
    else if (extension == ".js") {
        return http_content_type::JS; 
    } 
    else if (extension == ".json") {
        return http_content_type::JSON; 
    } 
    else if (extension == ".xml") {
        return http_content_type::XML; 
    } 
    else if (extension == ".png") {
        return http_content_type::PNG; 
    } 
    else if (extension == ".jpg" || extension == ".jpe" || extension == ".jpeg") {
        return http_content_type::JPEG; 
    }
    else if (extension == ".gif") {
        return http_content_type::GIF; 
    }
    else if (extension == ".bmp") {
        return http_content_type::BMP; 
    }
    else if (extension == ".ico") {
        return http_content_type::ICO;
    }
    else if (extension == ".tiff" || extension == ".tif") {
        return http_content_type::TIFF; 
    }
    else if (extension == ".svg") {
        return http_content_type::SVG; 
    }
    else if (extension == ".mp3") {
        return http_content_type::MP3; 
    }
    else {
        return http_content_type::EMPTY; 
    }
}
} // namespace http_content_type