// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/scm_rev.h"

namespace Common {

const char g_scm_rev[]      = GIT_REV;
const char g_scm_branch[]   = GIT_BRANCH;
const char g_scm_desc[]     = GIT_DESC;
const char g_build_name[]   = BUILD_NAME;
const char g_build_date[]   = BUILD_DATE;
const char g_build_fullname[] = BUILD_FULLNAME;
const char g_build_version[]  = BUILD_VERSION;
const char g_shader_cache_version[] = SHADER_CACHE_VERSION;

} // namespace

