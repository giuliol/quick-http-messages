//
// Created by Giulio Luzzati on 23/07/18.
//

#ifndef NEWCORE_DEBUG_H
#define NEWCORE_DEBUG_H

#include <cstdint>
#include <string>
#include <ostream>
#include <sstream>
#include <iostream>

typedef int64_t c_time_t;

c_time_t time_now(void);
std::string time_string();

static const std::string ANSI_COLOR_RED   =  "\x1b[31m";
static const std::string ANSI_COLOR_GREEN  = "\x1b[32m";
static const std::string ANSI_COLOR_YELLOW = "\x1b[33m";
static const std::string ANSI_COLOR_BLUE   = "\x1b[34m";
static const std::string ANSI_COLOR_MAGENTA = "\x1b[35m";
static const std::string ANSI_COLOR_CYAN  =  "\x1b[36m";
static const std::string ANSI_COLOR_RESET  = "\x1b[0m";
static const std::string STYLE_WARN     =    "\x1b[33m\x1b[1m";
static const std::string STYLE_ERR      =    "\x1b[31m\x1b[1m";
static const std::string STYLE_OK        =   "\x1b[34m";

static char logline_buffer[16384];

#define stringy(arg) #arg


class Logger {
public:
    Logger(const std::string& s):style(&s) {}
    Logger(const std::string& s, const std::string& t):style(&s),logtag(t) {}
    std::ostream& operator<<(const std::string& st){ s << st; return s; }
    ~Logger() {
        if(style)  std::cout << *style;
        std::cout << "[" << time_string() << "] ";
        if(!logtag.empty()) std::cout << "[" << logtag << "] ";
        std::cout << s.str();
        if(style) std::cout << ANSI_COLOR_RESET;
        std::cout << std::endl;
    }
private:
    const std::string* style = nullptr;
    const std::string logtag;
    std::stringstream s;
};

#define core_log              Logger(ANSI_COLOR_RESET)
#define core_ok               Logger(STYLE_OK)
#define core_err              Logger(STYLE_ERR)
#define core_warn             Logger(STYLE_WARN)

#define core_log_tag(arg)     Logger(ANSI_COLOR_RESET, arg)
#define core_ok_tag(arg)      Logger(STYLE_OK, arg)
#define core_err_tag(arg)     Logger(STYLE_ERR, arg)
#define core_warn_tag(arg)    Logger(STYLE_WARN, arg)


#define core_assert(cond, expr) \
if ( ! ( cond ) )   \
 { core_err << "assert fail:\t! ( " << #cond << " )   [" << __FILE__ << ":" << __LINE__ << "]" ; expr; }

#define core_try(expr, failure) \
try { expr; } catch (const std::exception& e) { core_err << e.what(); failure; }

#endif //NEWCORE_DEBUG_H
