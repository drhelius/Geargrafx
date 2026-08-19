/*
 * Geargrafx - PC Engine / TurboGrafx Emulator
 * Copyright (C) 2026  Ignacio Sanchez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef TRACE_LOGGER_FORMATTER_H
#define TRACE_LOGGER_FORMATTER_H

#include "trace_logger.h"

#define GG_TRACE_FORMAT_BUFFER_SIZE 512

struct GG_Trace_Format_Options
{
    bool bank;
    bool registers;
    bool flags;
    bool bytes;
    bool cycles;
    bool previous_cycle_valid;
    u64 previous_cycle;
};

void trace_log_format_cpu_bytes(const GG_Trace_Entry& entry, char* buffer, size_t buffer_size);
void trace_log_format_cycle_prefix(const GG_Trace_Entry& entry, bool previous_cycle_valid,
    u64 previous_cycle, char* buffer, size_t buffer_size);
void trace_logger_format_entry(const GG_Trace_Entry& entry,
    const GG_Trace_Format_Options& options, char* buffer, size_t buffer_size);

#endif /* TRACE_LOGGER_FORMATTER_H */
