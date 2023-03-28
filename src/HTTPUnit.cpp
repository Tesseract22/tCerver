#include "HTTPResponse.hpp"
#include <HTTPUnit.hpp>

HTTPUnit::HTTPUnit() { url_map_.insert({"/", &HTTP::defaultPage}); }