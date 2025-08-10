#include "html_parser.h"

std::vector<std::string> HtmlParser::extract_links(const std::string& html, const std::string& base_url) {
    std::vector<std::string> links;
    GumboOutput* output = gumbo_parse(html.c_str());
    if (!output) {
        return links;
    }
    extract_links(output->root, base_url, links);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return links;
}

void HtmlParser::extract_links(GumboNode* node, const std::string& base_url, std::vector<std::string>& links) {
    if (node->type != GUMBO_NODE_ELEMENT) return;
    if (node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) {
            std::string url = normalize_url(href->value, base_url);
            if (!url.empty()) {
                links.push_back(url);
            }
        }
    }
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        extract_links(static_cast<GumboNode*>(children->data[i]), base_url, links);
    }
}

std::string HtmlParser::extract_plain_text(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    if (!output) {
        return "";
    }
    std::string text = extract_text(output->root);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return text;
}

std::string HtmlParser::extract_text(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        if (node->v.element.tag == GUMBO_TAG_SCRIPT || node->v.element.tag == GUMBO_TAG_STYLE) {
            return "";
        }
        std::string text;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            text += extract_text(static_cast<GumboNode*>(children->data[i]));
        }
        return text;
    }
    return "";
}