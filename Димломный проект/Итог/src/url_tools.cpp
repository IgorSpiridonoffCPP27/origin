#include "url_tools.h"
#include <vector>
#include <algorithm>
#include <iostream> // Added for std::cout

std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto c : value) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || uc == '-' || uc == '_' || uc == '.' || uc == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(uc);
        }
    }
    return escaped.str();
}


// Resolve dot segments per RFC 3986
static std::string resolve_dot_segments(const std::string& input_path) {
	std::vector<std::string> segments;
	segments.reserve(16);
	std::string segment;
	for (size_t i = 0; i <= input_path.size(); ++i) {
		char ch = (i < input_path.size() ? input_path[i] : '/');
		if (ch == '/') {
			if (segment == "..") {
				if (!segments.empty()) segments.pop_back();
			} else if (!segment.empty() && segment != ".") {
				segments.push_back(segment);
			}
			segment.clear();
		} else {
			segment.push_back(ch);
		}
	}
	std::string result = "/";
	for (size_t i = 0; i < segments.size(); ++i) {
		if (i) result += '/';
		result += segments[i];
	}
	return result;
}

std::string normalize_url(const std::string& url, const std::string& base_url) {
	if (url.empty() || url[0] == '#' || url.rfind("javascript:", 0) == 0) {
		return "";
	}
	// Absolute URL
	if (url.find("://") != std::string::npos) {
		return url;
	}
	// Protocol-relative
	if (url.rfind("//", 0) == 0) {
		size_t proto_pos = base_url.find("://");
		if (proto_pos != std::string::npos) {
			return base_url.substr(0, proto_pos) + ":" + url;
		}
		return "";
	}

	// Parse base: scheme://authority/path?query#frag
	size_t proto_pos = base_url.find("://");
	if (proto_pos == std::string::npos) {
		return "";
	}
	std::string scheme = base_url.substr(0, proto_pos);
	size_t auth_start = proto_pos + 3;
	size_t path_start = base_url.find('/', auth_start);
	std::string authority = base_url.substr(auth_start, (path_start == std::string::npos ? base_url.size() : path_start) - auth_start);
	std::string base_path;
	if (path_start != std::string::npos) {
		size_t q_pos = base_url.find('?', path_start);
		size_t h_pos = base_url.find('#', path_start);
		size_t end = std::min(q_pos == std::string::npos ? base_url.size() : q_pos,
							h_pos == std::string::npos ? base_url.size() : h_pos);
		base_path = base_url.substr(path_start, end - path_start);
	} else {
		base_path = "/";
	}

	// Query-only href
	if (!url.empty() && url[0] == '?') {
		return scheme + "://" + authority + base_path + url;
	}

	// Build candidate path for relative URL
	std::string ref = url;
	// Keep query/fragment if present in ref
	size_t q_pos_ref = ref.find('?');
	size_t h_pos_ref = ref.find('#');
	size_t cut_pos = std::min(q_pos_ref == std::string::npos ? ref.size() : q_pos_ref,
							h_pos_ref == std::string::npos ? ref.size() : h_pos_ref);
	std::string ref_path_part = ref.substr(0, cut_pos);
	std::string ref_tail = ref.substr(cut_pos); // includes '?' or '#'

	std::string joined_path;
	if (!ref_path_part.empty() && ref_path_part[0] == '/') {
		joined_path = ref_path_part; // root-relative
	} else {
		// directory of base
		size_t last_slash = base_path.rfind('/');
		std::string base_dir = (last_slash == std::string::npos) ? "/" : base_path.substr(0, last_slash + 1);
		joined_path = base_dir + ref_path_part;
	}
	std::string resolved = resolve_dot_segments(joined_path);
	return scheme + "://" + authority + resolved + ref_tail;
}

std::string get_host(const std::string& url) {
	size_t host_start = std::string::npos;
	if (url.rfind("//", 0) == 0) {
		host_start = 2;
	} else {
		size_t protocol_pos = url.find("://");
		if (protocol_pos == std::string::npos) {
			return "";
		}
		host_start = protocol_pos + 3;
	}
	size_t host_end = url.find_first_of("/#?", host_start);
	if (host_end == std::string::npos) host_end = url.size();
	return url.substr(host_start, host_end - host_start);
}

// Кодирует только не-ASCII и пробелы в %HH, оставляя / ? & = : и др. разделители
std::string encode_http_target(const std::string& target) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char uc : target) {
        if (uc <= 0x20 || uc >= 0x7F) {
            escaped << '%' << std::uppercase << std::setw(2) << int(uc) << std::nouppercase;
        } else {
            switch (uc) {
                case ' ': escaped << "%20"; break;
                default: escaped << static_cast<char>(uc); break;
            }
        }
    }
    return escaped.str();
}