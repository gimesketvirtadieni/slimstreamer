#pragma once

#include <g3log/g3log.hpp>
#include <g3log/loglevels.hpp>

// introducing ERROR level
const LEVELS ERROR {WARNING.value + 1, {"ERROR"}};
