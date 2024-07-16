#include "http_path_utils.h"

namespace http_path_utils {

int PathBased(fs::path target_path, fs::path base) {

    target_path = fs::weakly_canonical(target_path);
    base = fs::weakly_canonical(base);

    int after = -1;

    auto tp = target_path.begin();
    auto b = base.begin();

    auto tp_end = target_path.end();
    auto b_end = base.end();
// *    FIX THIS: The last element of "api/v1/" ---> *tp = "" <---- BUT "api/v1" ----> *tp = target_path.end() <----- 
    if (*(--tp_end) != "") ++tp_end;
    if (*(--b_end) != "") ++b_end;
// *    ========
    for (; b != b_end; ++b, ++tp) {

        if (tp == tp_end || *tp != *b) {

            return after;
        }
    }

    after = std::distance(tp, tp_end);

    return after;
}

bool MatchPaths(fs::path target_path, fs::path path) {

    target_path = fs::weakly_canonical(target_path);
    path = fs::weakly_canonical(path);

    if ( std::distance(target_path.end(), target_path.begin()) != std::distance(path.end(), path.begin()) ) {

        return false;
    }

    for (auto tp = target_path.begin(), p = path.begin(); tp != target_path.end(); ++tp, ++p) {

        if (*tp != *p) {

            return false;
        }
    }

    return true;
}

std::string UrlUncode(std::string_view encoded_target) {

    std::string encoded_target_s{encoded_target.data(), encoded_target.size()};

    size_t pos = std::string::npos;

    char symbol = '\0';

    while ((pos = encoded_target_s.find('%')) != std::string::npos /* && pos < encoded_target_s.size()-2 */) {

        std::string code{encoded_target_s.substr(pos+1, 2).c_str()};

        if (code[0] >= 'A' && code[0] <= 'F') {

            symbol = (code[0]-'A'+10) * 16;
        } else if (code[0] >= '0' && code[0] <= '9') {

            symbol = (code[0]-'0') * 16;
        }

        if (code[1] >= 'A' && code[1] <= 'F') {

            symbol += code[1]-'A'+10;
        } else if (code[1] >= '0' && code[1] <= '9') {

            symbol += code[1]-'0';
        }

        encoded_target_s.replace(pos, 3, 1, symbol);

        symbol = '\0';
    }

    while ((pos = encoded_target_s.find('+')) != std::string::npos) {

        encoded_target_s.replace(pos, 1, 1, ' ');
    }

    return encoded_target_s;
}
} // namespace http_path_utils