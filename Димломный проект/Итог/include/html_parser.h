#include <vector>
#include <string>
#include <gumbo.h>
#include "url_tools.h"
class HtmlParser {
public:
    static std::vector<std::string> extract_links(const std::string& html, const std::string& base_url);
    static std::string extract_plain_text(const std::string& html);

private:
    static void extract_links(GumboNode* node, const std::string& base_url, std::vector<std::string>& links);
    static std::string extract_text(GumboNode* node);
};