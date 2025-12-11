/*
 * Geargrafx - PC Engine / TurboGrafx Emulator
 * Copyright (C) 2024  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include "geargrafx.h"
#include "application.h"

int main(int argc, char* argv[])
{
    char* rom_file = NULL;
    char* symbol_file = NULL;
    bool show_usage = false;
    bool force_fullscreen = false;
    bool force_windowed = false;
    int mcp_mode = -1; // -1 = disabled, 0 = stdio, 1 = tcp
    int mcp_tcp_port = 7777;
    int ret = 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-?") == 0) ||
                (strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "/?") == 0))
            {
                show_usage = true;
                ret = 0;
            }
            else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0))
            {
                printf("%s\n", GG_TITLE_ASCII);
                printf("Build: %s\n", GG_VERSION);
                printf("Author: Ignacio Sánchez (drhelius)\n");
                return 0;
            }
            else if ((strcmp(argv[i], "-f") == 0) || (strcmp(argv[i], "--fullscreen") == 0))
            {
                force_fullscreen = true;
            }
            else if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--windowed") == 0))
            {
                force_windowed = true;
            }
            else if (strcmp(argv[i], "--mcp-stdio") == 0)
            {
                mcp_mode = 0;
            }
            else if (strcmp(argv[i], "--mcp-http") == 0)
            {
                mcp_mode = 1;
            }
            else if (strcmp(argv[i], "--mcp-http-port") == 0)
            {
                if (i + 1 < argc)
                {
                    mcp_tcp_port = atoi(argv[++i]);
                    if (mcp_tcp_port <= 0 || mcp_tcp_port > 65535)
                    {
                        printf("Invalid port number: %d\n", mcp_tcp_port);
                        mcp_tcp_port = 7777;
                    }
                }
            }
            else
            {
                printf("Unknown option: %s\n", argv[i]);
                show_usage = true;
                ret = -1;
            }
        }
    }

    int non_option_count = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            if (non_option_count == 0)
                rom_file = argv[i];
            else if (non_option_count == 1)
                symbol_file = argv[i];
            
            non_option_count++;
            
            if (non_option_count > 2)
            {
                show_usage = true;
                ret = -1;
                break;
            }
        }
    }

    if (show_usage)
    {
        printf("Usage: %s [options] [game_file] [symbol_file]\n", argv[0]);
        printf("  [game_file]         Game file: accepts ROMs (.pce, .sgx, .hes), CUE (.cue) or ZIP (.zip)\n");
        printf("\nOptions:\n");
        printf("  -f, --fullscreen      Start in fullscreen mode\n");
        printf("  -w, --windowed        Start in windowed mode with menu visible\n");
        printf("      --mcp-stdio       Auto-start MCP server with stdio transport\n");
        printf("      --mcp-http        Auto-start MCP server with HTTP transport\n");
        printf("      --mcp-http-port N HTTP port for MCP server (default: 7777)\n");
        printf("  -v, --version         Display version information\n");
        printf("  -h, --help            Display this help message\n");
        return ret;
    }

    if (force_fullscreen && force_windowed)
        force_fullscreen = false;

    ret = application_init(rom_file, symbol_file, force_fullscreen, force_windowed, mcp_mode, mcp_tcp_port);

    if (ret == 0)
        application_mainloop();

    application_destroy();

    return ret;
}