// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/logging/backend.h"

#include "libretro.h"

class LibRetroLogger : public Log::Backend {
public:
    explicit LibRetroLogger(retro_log_printf_t callback);

    const char *GetName() const override;

    void Write(const Log::Entry &entry) override;

private:
    retro_log_printf_t callback;
};
