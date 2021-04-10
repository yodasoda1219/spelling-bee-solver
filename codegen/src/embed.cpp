#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <cassert>
#include "actions.h"
static size_t write_callback(char* pointer, size_t size, size_t nmemb, void* userdata) {
    std::stringstream& output = *(std::stringstream*)userdata;
    output << std::string(pointer, nmemb);
    return nmemb;
}
static CURLcode pull(CURL* c, const std::string& url, std::string& output) {
    std::stringstream data;
    CURLcode code;
    code = curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    code = curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, false);
    code = curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_callback);
    code = curl_easy_setopt(c, CURLOPT_WRITEDATA, &data);
    code = curl_easy_perform(c);
    output = data.str();
    return code;
}
template<typename T> static bool __equals__(const T& a, const T& b) {
    return a == b;
}
template<typename Container, typename Element> static bool contains(const Container& container, const Element& element, std::function<bool(const Element&, const Element&)> equals = __equals__<Element>) {
    for (const Element& e : container) {
        if (equals(e, element)) return true;
    }
    return false;
}
static std::string parse_filename(const std::string& path) {
    size_t pos = path.find_last_of('/');
#ifdef SYSTEM_WINDOWS
    if (pos == std::string::npos) {
        pos = path.find_last_of('\\');
    }
#endif
    std::string local_filename;
    if (pos != std::string::npos) {
        local_filename = path.substr(pos + 1);
    } else {
        local_filename = path;
    }
    pos = local_filename.find_last_of('.');
    if (pos != std::string::npos) {
        return local_filename.substr(0, pos);
    } else {
        return local_filename;
    }
}
int embed(const std::string& url, const std::string& filepath) {
    std::string data;
    CURL* c = curl_easy_init();
    CURLcode code = pull(c, url, data);
    if (code != CURLE_OK) return (int)code;
    curl_easy_cleanup(c);
    std::cout << "Successfully pulled data from: " << url << std::endl;
    std::cout << "Parsing dictionary data..." << std::endl;
    std::string double_endl = "\r\n\r\n";
    size_t pos = data.find(double_endl);
    if (pos != std::string::npos) {
        data = data.substr(pos + double_endl.length());
    }
    std::string word = "";
    std::vector<std::string> dict;
    for (size_t i = 0; i < data.length(); i++) {
        char c = data[i];
        if (c == '\n') {
            dict.push_back(word);
            word.clear();
        } else if (c != '\r') {
            word.push_back(data[i]);
        }
    }
    if (!contains(dict, word)) {
        dict.push_back(word);
    }
    std::cout << "Embedding data into C source/header file: " << filepath << std::endl;
    std::string symbol_prefix = parse_filename(filepath);
    std::string data_footer = "const char* " + symbol_prefix + "_data[] = {\n";
    std::ofstream output(filepath);
    output << data_footer;
    for (const auto& w : dict) {
        output << "\t\"" << w << "\",\n";
    }
    output << "};\n" << std::flush;
    output << "size_t " << symbol_prefix << "_data_size = " << dict.size() << ";\n" << std::flush;
    output.close();
    return 0;
}