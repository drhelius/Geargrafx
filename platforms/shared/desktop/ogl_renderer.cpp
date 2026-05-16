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

#if defined(__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #define GLAD_GL_IMPLEMENTATION
    #include <glad.h>
#endif

#include <string>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "emu.h"
#include "config.h"
#include "geargrafx.h"

#define OGL_RENDERER_IMPORT
#include "ogl_renderer.h"
#include "ogl_shader_chain.h"

static uint32_t system_texture;
static uint32_t frame_buffer_object;
static GG_Runtime_Info current_runtime;
static OglRendererScreenGeometry screen_geometry;

static uint32_t quad_shader_program = 0;
static uint32_t quad_vao = 0;
static uint32_t quad_vbo = 0;
static int quad_uniform_texture = -1;
static int quad_uniform_color = -1;
static int quad_uniform_tex_scale = -1;

static void init_ogl_gui(void);
static bool init_ogl_emu(void);
static void init_ogl_debug(void);
static void init_ogl_savestates(void);
static bool init_shaders(void);
static void render_gui(void);
static bool should_use_internal_shader_chain(void);
static void render_internal_shader_chain(void);
static void render_external_shader_chain(void);
static void render_internal_shader_chain_feedback(void);
static void render_emu_normal(void);
static void update_emu_texture(void);
static void render_quad(uint32_t program, uint32_t texture, int viewport_width, int viewport_height, float tex_h, float tex_v, float red, float green, float blue, float alpha);
static void render_quad_preset(int pass_index, uint32_t program, uint32_t texture, int input_width, int input_height, int viewport_width, int viewport_height);
static void update_system_texture(void);
static void update_debug_textures(void);
static void update_savestates_texture(void);
static void load_configured_shader_preset(void);
static void apply_shader_parameter_config(void);
static float get_original_aspect(void);
static const char* get_glsl_version(void);
static uint32_t compile_shader(uint32_t shader_type, const char* shader_name, const char** sources, int source_count);
static uint32_t link_program(uint32_t vertex_shader, uint32_t fragment_shader, const char* program_name);
static void configure_texture_2d(bool filter_linear);
static void create_texture_2d(uint32_t* texture, int width, int height, int internal_format, uint32_t format, uint32_t type, const void* pixels, bool filter_linear);
static void resize_texture_2d(uint32_t texture, int width, int height, int internal_format, uint32_t format, uint32_t type, const void* pixels, bool filter_linear);
static bool check_framebuffer_complete(const char* name);

bool ogl_renderer_init(void)
{
#if !defined(__APPLE__)
    int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);

    if (version == 0)
    {
        Error("GLAD: Failed to initialize OpenGL context");
        return false;
    }

    Log("GLAD: OpenGL %d.%d", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
#endif

    ogl_renderer_opengl_version = (const char*)glGetString(GL_VERSION);
    Log("Starting OpenGL %s", ogl_renderer_opengl_version);

    glDisable(GL_FRAMEBUFFER_SRGB);

    if (!init_shaders())
        return false;

    init_ogl_gui();

    if (!init_ogl_emu())
        return false;

    if (!ogl_shader_chain_init("internal shader-chain framebuffer"))
        return false;

    load_configured_shader_preset();

    init_ogl_debug();
    init_ogl_savestates();

    return true;
}

void ogl_renderer_destroy(void)
{
    glDeleteFramebuffers(1, &frame_buffer_object); 
    glDeleteTextures(1, &ogl_renderer_emu_texture);
    glDeleteTextures(1, &system_texture);
    ogl_shader_chain_destroy();

    glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_background[0]);
    glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_background[1]);
    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
        {
            glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_sprites[i][s]);
        }
    glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_tiles[0]);
    glDeleteTextures(1, &ogl_renderer_emu_debug_huc6270_tiles[1]);
    glDeleteTextures(1, &ogl_renderer_emu_savestates);

    if (quad_shader_program)
        glDeleteProgram(quad_shader_program);
    if (quad_vao)
        glDeleteVertexArrays(1, &quad_vao);
    if (quad_vbo)
        glDeleteBuffers(1, &quad_vbo);

    quad_shader_program = 0;
    quad_vao = 0;
    quad_vbo = 0;

    ImGui_ImplOpenGL3_Shutdown();
}

void ogl_renderer_begin_render(void)
{
    ImGui_ImplOpenGL3_NewFrame();
}

void ogl_renderer_render(void)
{
    emu_get_runtime(current_runtime);

    if (config_debug.debug)
    {
        update_debug_textures();
    }

    update_savestates_texture();

    bool use_internal_shader_chain = should_use_internal_shader_chain();

    if (use_internal_shader_chain)
        render_internal_shader_chain();
    else
        render_emu_normal();

    update_emu_texture();

    ImVec4 clear_color = ImVec4(config_video.background_color[0], config_video.background_color[1], config_video.background_color[2], 1.00f);

    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);

    glDisable(GL_FRAMEBUFFER_SRGB);

    glViewport(0, 0, fb_width, fb_height);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    render_gui();
}

void ogl_renderer_end_render(void)
{
#if defined(__APPLE__) || defined(_WIN32)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif
}

void ogl_renderer_set_screen_geometry(const OglRendererScreenGeometry* geometry)
{
    if (!geometry)
        return;

    screen_geometry = *geometry;
}

uint32_t ogl_renderer_get_screen_texture(void)
{
    if (should_use_internal_shader_chain() && ogl_shader_chain_get_pass_texture() != 0)
        return ogl_shader_chain_get_pass_texture();

    return ogl_renderer_emu_texture;
}

void ogl_renderer_get_screen_uv(float* u, float* v)
{
    if (should_use_internal_shader_chain())
    {
        if (u)
            *u = 1.0f;
        if (v)
            *v = 1.0f;
        return;
    }

    GG_Runtime_Info runtime;
    emu_get_runtime(runtime);

    if (u)
        *u = (float)runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    if (v)
        *v = (float)runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;
}

bool ogl_renderer_load_shader_preset(const char* path)
{
    bool same_path = path && config_video.shader_preset_path.compare(path) == 0;

    if (!ogl_shader_chain_load_preset(path))
        return false;

    config_video.shader_mode = config_ShaderMode_External;

    if (same_path)
        apply_shader_parameter_config();

    config_video.shader_preset_path.assign(path ? path : "");
    ogl_renderer_save_shader_parameter_config();
    return true;
}

bool ogl_renderer_reload_shader_preset(void)
{
    if (!ogl_shader_chain_reload_preset())
        return false;

    config_video.shader_mode = config_ShaderMode_External;
    config_video.shader_preset_path.assign(ogl_shader_chain_get_preset_path());
    ogl_renderer_save_shader_parameter_config();
    return true;
}

void ogl_renderer_unload_shader_preset(void)
{
    ogl_shader_chain_unload_preset();
    config_video.shader_mode = config_ShaderMode_Off;
    config_video.shader_parameter_count = 0;
}

void ogl_renderer_save_shader_parameter_config(void)
{
    int count = ogl_shader_chain_get_parameter_count();
    count = CLAMP(count, 0, config_shader_parameter_count);
    config_video.shader_parameter_count = count;

    for (int i = 0; i < count; i++)
    {
        const ShaderPresetParameter* parameter = ogl_shader_chain_get_parameter(i);
        if (!parameter)
            continue;

        config_video.shader_parameter_name[i].assign(parameter->name);
        config_video.shader_parameter_value[i] = parameter->value;
    }
}

static void init_ogl_gui(void)
{
#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif
}

static bool init_ogl_emu(void)
{
    glGenFramebuffers(1, &frame_buffer_object);
    create_texture_2d(&ogl_renderer_emu_texture, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, NULL, false);
    create_texture_2d(&system_texture, SYSTEM_TEXTURE_WIDTH, SYSTEM_TEXTURE_HEIGHT, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_frame_buffer, false);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ogl_renderer_emu_texture, 0);

    bool complete = check_framebuffer_complete("emulator framebuffer");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return complete;
}

static void init_ogl_debug(void)
{
    create_texture_2d(&ogl_renderer_emu_debug_huc6270_background[0], HUC6270_MAX_BACKGROUND_WIDTH, HUC6270_MAX_BACKGROUND_HEIGHT, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_background_buffer[0], false);
    create_texture_2d(&ogl_renderer_emu_debug_huc6270_background[1], HUC6270_MAX_BACKGROUND_WIDTH, HUC6270_MAX_BACKGROUND_HEIGHT, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_background_buffer[1], false);

    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
        {
            create_texture_2d(&ogl_renderer_emu_debug_huc6270_sprites[i][s], 32, 64, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_sprite_buffers[i][s], false);
        }

    for (int i = 0; i < 2; i++)
    {
        create_texture_2d(&ogl_renderer_emu_debug_huc6270_tiles[i], 32 * 8, 64 * 8, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)emu_debug_tiles_buffer[i], false);
    }
}

static void init_ogl_savestates(void)
{
    create_texture_2d(&ogl_renderer_emu_savestates, 2048, 256, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
}

static void render_gui(void)
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static bool should_use_internal_shader_chain(void)
{
    return ogl_shader_chain_is_initialized() &&
            !config_debug.debug &&
            screen_geometry.physical_width > 0 &&
            screen_geometry.physical_height > 0;
}

static void render_internal_shader_chain(void)
{
    bool has_preset = ogl_shader_chain_has_preset();
    bool filter_linear = has_preset ? ogl_shader_chain_get_preset_filter_linear() : false;

    if (!ogl_shader_chain_update_source_texture(current_runtime.screen_width, current_runtime.screen_height, emu_frame_buffer, filter_linear))
    {
        render_emu_normal();
        return;
    }

    if (has_preset)
    {
        render_external_shader_chain();
        return;
    }

    if (!ogl_shader_chain_resize_pass_texture(screen_geometry.physical_width, screen_geometry.physical_height, false))
    {
        render_emu_normal();
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ogl_shader_chain_get_pass_framebuffer());
    glDisable(GL_BLEND);

    render_quad(quad_shader_program, ogl_shader_chain_get_source_texture(),
            ogl_shader_chain_get_pass_width(), ogl_shader_chain_get_pass_height(),
            1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void render_external_shader_chain(void)
{
    glDisable(GL_BLEND);

    int pass_count = ogl_shader_chain_get_preset_pass_count();
    if (pass_count <= 0)
        return;

    int original_width = current_runtime.screen_width;
    int original_height = current_runtime.screen_height;
    int input_width = original_width;
    int input_height = original_height;
    uint32_t input_texture = ogl_shader_chain_get_source_texture();

    for (int i = 0; i < pass_count; i++)
    {
        int output_width = screen_geometry.physical_width;
        int output_height = screen_geometry.physical_height;
        bool final_pass = i == pass_count - 1;

        ogl_shader_chain_get_preset_pass_output_size(i, original_width, original_height, input_width, input_height,
                screen_geometry.physical_width, screen_geometry.physical_height, &output_width, &output_height);

        uint32_t output_fbo = 0;
        uint32_t output_texture = 0;
        if (final_pass)
        {
            if (!ogl_shader_chain_resize_pass_texture(output_width, output_height, false))
                return;
            output_fbo = ogl_shader_chain_get_pass_framebuffer();
            output_texture = ogl_shader_chain_get_pass_texture();
        }
        else
        {
            if (!ogl_shader_chain_resize_intermediate_texture(i, output_width, output_height, ogl_shader_chain_get_preset_pass_filter_linear(i + 1)))
                return;
            output_fbo = ogl_shader_chain_get_intermediate_framebuffer(i);
            output_texture = ogl_shader_chain_get_intermediate_texture(i);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, output_fbo);
        render_quad_preset(i, ogl_shader_chain_get_preset_pass_program(i), input_texture, input_width, input_height, output_width, output_height);

        input_texture = output_texture;
        input_width = output_width;
        input_height = output_height;
    }


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (ogl_shader_chain_preset_uses_feedback())
        render_internal_shader_chain_feedback();
}

static void render_internal_shader_chain_feedback(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, ogl_shader_chain_get_feedback_framebuffer());
    glDisable(GL_BLEND);

    render_quad(quad_shader_program, ogl_shader_chain_get_pass_texture(),
            ogl_shader_chain_get_pass_width(), ogl_shader_chain_get_pass_height(),
            1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void render_emu_normal(void)
{
    float tex_h = (float)current_runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    float tex_v = (float)current_runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_object);

    glDisable(GL_BLEND);

    update_system_texture();

    int viewport_width = current_runtime.screen_width;
    int viewport_height = current_runtime.screen_height * FRAME_BUFFER_SCALE;

    render_quad(quad_shader_program, system_texture, viewport_width, viewport_height, tex_h, tex_v, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void update_system_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, system_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, current_runtime.screen_width, current_runtime.screen_height,
            GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_frame_buffer);

    configure_texture_2d(false);
}

static void update_debug_textures(void)
{
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_huc6270_background[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, emu_debug_background_buffer_width[0], emu_debug_background_buffer_height[0],
            GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_debug_background_buffer[0]);
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_huc6270_background[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, emu_debug_background_buffer_width[1], emu_debug_background_buffer_height[1],
            GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_debug_background_buffer[1]);

    for (int i = 0; i < 2; i++)
        for (int s = 0; s < 64; s++)
        {
            glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_huc6270_sprites[i][s]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, emu_debug_sprite_widths[i][s], emu_debug_sprite_heights[i][s],
                    GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_debug_sprite_buffers[i][s]);
        }

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_debug_huc6270_tiles[i]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 32 * 8, 64 * 8,
                GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_debug_tiles_buffer[i]);
    }
}

static void update_savestates_texture(void)
{
    int i = config_emulator.save_slot;

    if (IsValidPointer(emu_savestates_screenshots[i].data))
    {
        int width = emu_savestates_screenshots[i].width;
        int height = emu_savestates_screenshots[i].height;
        glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_savestates);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) emu_savestates_screenshots[i].data);
    }
}

static void update_emu_texture(void)
{
    glBindTexture(GL_TEXTURE_2D, ogl_renderer_emu_texture);

    configure_texture_2d(false);
}

static void render_quad(uint32_t program, uint32_t texture, int viewport_width, int viewport_height, float tex_h, float tex_v, float red, float green, float blue, float alpha)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glUseProgram(program);
    glUniform2f(quad_uniform_tex_scale, tex_h, tex_v);
    glUniform4f(quad_uniform_color, red, green, blue, alpha);

    glViewport(0, 0, viewport_width, viewport_height);

    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
}

static void render_quad_preset(int pass_index, uint32_t program, uint32_t texture, int input_width, int input_height, int viewport_width, int viewport_height)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ogl_shader_chain_get_source_texture());

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ogl_shader_chain_get_feedback_texture());

    glUseProgram(program);
    ogl_shader_chain_apply_preset_uniforms(pass_index, current_runtime.screen_width, current_runtime.screen_height,
            input_width, input_height, viewport_width, viewport_height,
            screen_geometry.physical_width, screen_geometry.physical_height, get_original_aspect());

    glViewport(0, 0, viewport_width, viewport_height);

    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
}

static void load_configured_shader_preset(void)
{
    if (config_video.shader_mode != config_ShaderMode_External || config_video.shader_preset_path.empty())
        return;

    if (!ogl_shader_chain_load_preset(config_video.shader_preset_path.c_str()))
    {
        Error("Unable to load configured shader preset: %s", ogl_shader_chain_get_last_error());
        return;
    }

    apply_shader_parameter_config();
}

static void apply_shader_parameter_config(void)
{
    int count = CLAMP(config_video.shader_parameter_count, 0, config_shader_parameter_count);
    int parameter_count = ogl_shader_chain_get_parameter_count();

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < parameter_count; j++)
        {
            const ShaderPresetParameter* parameter = ogl_shader_chain_get_parameter(j);
            if (!parameter)
                continue;

            if (config_video.shader_parameter_name[i].compare(parameter->name) == 0)
            {
                ogl_shader_chain_set_parameter(j, config_video.shader_parameter_value[i]);
                break;
            }
        }
    }
}

static float get_original_aspect(void)
{
    if (current_runtime.screen_height <= 0)
        return 1.0f;

    int width_scale = current_runtime.width_scale > 0 ? current_runtime.width_scale : 1;
    float logical_width = (float)current_runtime.screen_width / (float)width_scale;
    return logical_width / (float)current_runtime.screen_height;
}

static const char* get_glsl_version(void)
{
#if defined(__APPLE__)
    return "#version 150\n";
#else
    return "#version 130\n";
#endif
}

static uint32_t compile_shader(uint32_t shader_type, const char* shader_name, const char** sources, int source_count)
{
    uint32_t shader = glCreateShader(shader_type);
    glShaderSource(shader, source_count, sources, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar info[2048];
        glGetShaderInfoLog(shader, sizeof(info), NULL, info);
        Error("%s shader compile error: %s", shader_name, info);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static uint32_t link_program(uint32_t vertex_shader, uint32_t fragment_shader, const char* program_name)
{
    uint32_t program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glBindAttribLocation(program, 0, "aPos");
    glBindAttribLocation(program, 1, "aTexCoord");
    glLinkProgram(program);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar info[2048];
        glGetProgramInfoLog(program, sizeof(info), NULL, info);
        Error("%s program link error: %s", program_name, info);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static void configure_texture_2d(bool filter_linear)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_linear ? GL_LINEAR : GL_NEAREST);
}

static void create_texture_2d(uint32_t* texture, int width, int height, int internal_format, uint32_t format, uint32_t type, const void* pixels, bool filter_linear)
{
    glGenTextures(1, texture);
    resize_texture_2d(*texture, width, height, internal_format, format, type, pixels, filter_linear);
}

static void resize_texture_2d(uint32_t texture, int width, int height, int internal_format, uint32_t format, uint32_t type, const void* pixels, bool filter_linear)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, pixels);
    configure_texture_2d(filter_linear);
}

static bool check_framebuffer_complete(const char* name)
{
    uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
        return true;

    Error("OpenGL framebuffer incomplete (%s): 0x%04X", name, status);
    return false;
}

static bool init_shaders(void)
{
    const char* version = get_glsl_version();

    const char* vs_body =
        "in vec2 aPos;\n"
        "in vec2 aTexCoord;\n"
        "out vec2 vTexCoord;\n"
        "uniform vec2 uTexScale;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    vTexCoord = aTexCoord * uTexScale;\n"
        "}\n";

    const char* fs_body =
        "in vec2 vTexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uTexture;\n"
        "uniform vec4 uColor;\n"
        "uniform vec2 uTexScale;\n"
        "void main() {\n"
        "    FragColor = texture(uTexture, vTexCoord) * uColor;\n"
        "}\n";

    const char* vs_sources[2] = { version, vs_body };
    const char* fs_sources[2] = { version, fs_body };

    uint32_t vs = compile_shader(GL_VERTEX_SHADER, "Quad vertex", vs_sources, 2);
    if (!vs)
        return false;

    uint32_t fs = compile_shader(GL_FRAGMENT_SHADER, "Quad fragment", fs_sources, 2);
    if (!fs)
    {
        glDeleteShader(vs);
        return false;
    }

    quad_shader_program = link_program(vs, fs, "Quad shader");

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!quad_shader_program)
        return false;

    quad_uniform_tex_scale = glGetUniformLocation(quad_shader_program, "uTexScale");
    quad_uniform_texture = glGetUniformLocation(quad_shader_program, "uTexture");
    quad_uniform_color = glGetUniformLocation(quad_shader_program, "uColor");

    glUseProgram(quad_shader_program);
    glUniform1i(quad_uniform_texture, 0);
    glUseProgram(0);

    float quad_vertices[] = {
        -1.0f, -1.0f,  0.0f,  0.0f,
         3.0f, -1.0f,  2.0f,  0.0f,
        -1.0f,  3.0f,  0.0f,  2.0f,
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    GLint pos_attrib = glGetAttribLocation(quad_shader_program, "aPos");
    GLint tex_attrib = glGetAttribLocation(quad_shader_program, "aTexCoord");

    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(tex_attrib);
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Debug("Quad shader initialized (program=%u, vao=%u, vbo=%u)", quad_shader_program, quad_vao, quad_vbo);
    return true;
}