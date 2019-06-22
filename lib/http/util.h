#ifndef util_h
#define util_h

#include <memory.h>
#include <cstring>
#include <sstream>
//#include <zlib.h>

struct packet_info {
    std::string src_addr;
    std::string dst_addr;
    bool        is_fin;
    std::string body;
};

extern bool is_atty;

bool is_plain_text(const std::string &s);

void get_join_addr(const std::string &src_addr, const std::string &dst_addr, std::string &ret);

std::string timeval2tr(const struct timeval *ts);

bool gzip_decompress(std::string &src, std::string &dst);

std::string urlencode(const std::string &s);

#endif