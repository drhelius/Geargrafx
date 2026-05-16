#if defined(__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#else
    #include <glad.h>
#endif

#include <stddef.h>
#include <string.h>
#include <string>
#include "geargrafx.h"
#include "ogl_shader_program.h"
#include "shader_preset.h"

#define OGL_SHADER_CHAIN_IMPORT
#include "ogl_shader_chain.h"

static uint32_t source_texture = 0;
static uint32_t pass_texture = 0;
static uint32_t pass_fbo = 0;
static uint32_t feedback_texture = 0;
static uint32_t feedback_fbo = 0;
static uint32_t intermediate_textures[SHADER_PRESET_MAX_PASSES - 1];
static uint32_t intermediate_fbos[SHADER_PRESET_MAX_PASSES - 1];
static int intermediate_widths[SHADER_PRESET_MAX_PASSES - 1];
static int intermediate_heights[SHADER_PRESET_MAX_PASSES - 1];
static int source_width = 1;
static int source_height = 1;
static int pass_width = 1;
static int pass_height = 1;
static bool initialized = false;
static const char* framebuffer_name = "shader-chain framebuffer";
static ShaderPreset active_preset;
struct PresetProgramState
{
    uint32_t program;
    int uniform_source;
    int uniform_original;
    int uniform_feedback;
    int uniform_source_size;
    int uniform_original_size;
    int uniform_output_size;
    int uniform_final_viewport_size;
    int uniform_feedback_size;
    int uniform_frame_count;
    int uniform_frame_direction;
    int uniform_original_aspect;
    int uniform_parameters[SHADER_PRESET_MAX_PARAMETERS];
};
static PresetProgramState preset_programs[SHADER_PRESET_MAX_PASSES];
static bool preset_loaded = false;
static char last_error[2048];
static int preset_frame_count = 0;

static void configure_texture_2d(bool filter_linear);
static void resize_texture_2d(uint32_t texture, int width, int height, int internal_format, uint32_t format, uint32_t type, const void* pixels, bool filter_linear);
static bool check_framebuffer_complete(const char* name);
static bool resize_framebuffer_texture(uint32_t texture, uint32_t fbo, int width, int height, bool filter_linear, const char* name);
static bool load_preset_programs(const ShaderPreset* preset, PresetProgramState* programs, char* error, size_t error_size);
static void bind_preset_uniform_locations(PresetProgramState* state);
static void clear_preset_state(void);
static void delete_preset_programs(void);
static void clear_feedback_texture(void);
static void set_last_error(const char* message);
static int scale_dimension(ShaderPresetScaleType scale_type, float scale, int absolute_size, int source_size, int previous_size, int viewport_size);
static float safe_inverse(int value);

bool ogl_shader_chain_init(const char* name)
{
    ogl_shader_chain_destroy();

    framebuffer_name = name ? name : "shader-chain framebuffer";
    source_width = 1;
    source_height = 1;
    pass_width = 1;
    pass_height = 1;

    glGenTextures(1, &source_texture);
    resize_texture_2d(source_texture, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);

    glGenTextures(1, &pass_texture);
    resize_texture_2d(pass_texture, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);

    glGenTextures(1, &feedback_texture);
    resize_texture_2d(feedback_texture, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);

    glGenFramebuffers(1, &pass_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pass_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pass_texture, 0);
    bool pass_complete = check_framebuffer_complete(framebuffer_name);

    glGenFramebuffers(1, &feedback_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, feedback_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, feedback_texture, 0);
    bool feedback_complete = check_framebuffer_complete(framebuffer_name);

    bool intermediates_complete = true;
    for (int i = 0; i < SHADER_PRESET_MAX_PASSES - 1; i++)
    {
        intermediate_widths[i] = 1;
        intermediate_heights[i] = 1;
        glGenTextures(1, &intermediate_textures[i]);
        resize_texture_2d(intermediate_textures[i], 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        glGenFramebuffers(1, &intermediate_fbos[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediate_fbos[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediate_textures[i], 0);
        intermediates_complete = check_framebuffer_complete(framebuffer_name) && intermediates_complete;
    }

    initialized = pass_complete && feedback_complete && intermediates_complete;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return initialized;
}

void ogl_shader_chain_destroy(void)
{
    ogl_shader_chain_unload_preset();

    if (pass_fbo)
        glDeleteFramebuffers(1, &pass_fbo);
    if (feedback_fbo)
        glDeleteFramebuffers(1, &feedback_fbo);
    for (int i = 0; i < SHADER_PRESET_MAX_PASSES - 1; i++)
    {
        if (intermediate_fbos[i])
            glDeleteFramebuffers(1, &intermediate_fbos[i]);
        if (intermediate_textures[i])
            glDeleteTextures(1, &intermediate_textures[i]);
        intermediate_fbos[i] = 0;
        intermediate_textures[i] = 0;
        intermediate_widths[i] = 1;
        intermediate_heights[i] = 1;
    }
    if (feedback_texture)
        glDeleteTextures(1, &feedback_texture);
    if (pass_texture)
        glDeleteTextures(1, &pass_texture);
    if (source_texture)
        glDeleteTextures(1, &source_texture);

    pass_fbo = 0;
    feedback_fbo = 0;
    feedback_texture = 0;
    pass_texture = 0;
    source_texture = 0;
    source_width = 1;
    source_height = 1;
    pass_width = 1;
    pass_height = 1;
    initialized = false;
}

bool ogl_shader_chain_is_initialized(void)
{
    return initialized;
}

bool ogl_shader_chain_update_source_texture(int width, int height, const void* pixels, bool filter_linear)
{
    if (!source_texture)
        return false;

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    if (source_width != width || source_height != height)
    {
        resize_texture_2d(source_texture, width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, filter_linear);
        source_width = width;
        source_height = height;
    }

    glBindTexture(GL_TEXTURE_2D, source_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    configure_texture_2d(filter_linear);

    return true;
}

bool ogl_shader_chain_resize_pass_texture(int width, int height, bool filter_linear)
{
    if (!pass_texture || !pass_fbo || !feedback_texture || !feedback_fbo)
        return false;

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    if (pass_width == width && pass_height == height)
        return true;

    preset_frame_count = 0;

    bool pass_complete = resize_framebuffer_texture(pass_texture, pass_fbo, width, height, filter_linear, framebuffer_name);
    bool feedback_complete = resize_framebuffer_texture(feedback_texture, feedback_fbo, width, height, filter_linear, framebuffer_name);

    bool complete = pass_complete && feedback_complete;

    if (complete)
    {
        pass_width = width;
        pass_height = height;
        clear_feedback_texture();
    }

    return complete;
}

bool ogl_shader_chain_resize_intermediate_texture(int index, int width, int height, bool filter_linear)
{
    if (index < 0 || index >= SHADER_PRESET_MAX_PASSES - 1)
        return false;

    if (!intermediate_textures[index] || !intermediate_fbos[index])
        return false;

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;

    if (intermediate_widths[index] == width && intermediate_heights[index] == height)
        return true;

    if (!resize_framebuffer_texture(intermediate_textures[index], intermediate_fbos[index], width, height, filter_linear, framebuffer_name))
        return false;

    intermediate_widths[index] = width;
    intermediate_heights[index] = height;
    return true;
}

bool ogl_shader_chain_load_preset(const char* path)
{
    ShaderPreset new_preset;
    char error[sizeof(last_error)];

    if (!shader_preset_load(path, &new_preset, error, sizeof(error)))
    {
        set_last_error(error);
        Error("Shader preset load failed: %s", last_error);
        return false;
    }

    PresetProgramState new_programs[SHADER_PRESET_MAX_PASSES];
    memset(new_programs, 0, sizeof(new_programs));
    if (!load_preset_programs(&new_preset, new_programs, error, sizeof(error)))
    {
        set_last_error(error);
        Error("Shader preset compile failed: %s", last_error);
        return false;
    }

    delete_preset_programs();

    active_preset = new_preset;
    memcpy(preset_programs, new_programs, sizeof(preset_programs));
    preset_loaded = true;
    preset_frame_count = 0;
    last_error[0] = '\0';

    for (int i = 0; i < active_preset.pass_count; i++)
        bind_preset_uniform_locations(&preset_programs[i]);
    clear_feedback_texture();

    Log("Loaded shader preset: %s", active_preset.preset_path);
    return true;
}

bool ogl_shader_chain_reload_preset(void)
{
    if (!preset_loaded || active_preset.preset_path[0] == '\0')
    {
        set_last_error("No shader preset is loaded");
        return false;
    }

    char current_names[SHADER_PRESET_MAX_PARAMETERS][SHADER_PRESET_MAX_PARAMETER_NAME];
    float current_values[SHADER_PRESET_MAX_PARAMETERS];
    int current_count = active_preset.parameter_count;
    for (int i = 0; i < current_count; i++)
    {
        strncpy_fit(current_names[i], active_preset.parameters[i].name, sizeof(current_names[i]));
        current_values[i] = active_preset.parameters[i].value;
    }

    char path[SHADER_PRESET_MAX_PATH];
    strncpy_fit(path, active_preset.preset_path, sizeof(path));

    if (!ogl_shader_chain_load_preset(path))
        return false;

    for (int i = 0; i < active_preset.parameter_count; i++)
    {
        for (int j = 0; j < current_count; j++)
        {
            if (strcmp(active_preset.parameters[i].name, current_names[j]) == 0)
            {
                ogl_shader_chain_set_parameter(i, current_values[j]);
                break;
            }
        }
    }

    return true;
}

void ogl_shader_chain_unload_preset(void)
{
    delete_preset_programs();
    clear_preset_state();
}

bool ogl_shader_chain_has_preset(void)
{
    return preset_loaded && active_preset.pass_count > 0 && preset_programs[0].program != 0;
}

uint32_t ogl_shader_chain_get_preset_program(void)
{
    return ogl_shader_chain_get_preset_pass_program(0);
}

uint32_t ogl_shader_chain_get_preset_pass_program(int index)
{
    if (!preset_loaded || index < 0 || index >= active_preset.pass_count)
        return 0;

    return preset_programs[index].program;
}

const char* ogl_shader_chain_get_preset_name(void)
{
    return preset_loaded ? active_preset.name : "";
}

const char* ogl_shader_chain_get_preset_path(void)
{
    return preset_loaded ? active_preset.preset_path : "";
}

const char* ogl_shader_chain_get_last_error(void)
{
    return last_error;
}

bool ogl_shader_chain_get_preset_filter_linear(void)
{
    return ogl_shader_chain_get_preset_pass_filter_linear(0);
}

int ogl_shader_chain_get_preset_pass_count(void)
{
    return preset_loaded ? active_preset.pass_count : 0;
}

bool ogl_shader_chain_get_preset_pass_filter_linear(int index)
{
    if (!preset_loaded || index < 0 || index >= active_preset.pass_count)
        return false;

    return active_preset.passes[index].filter_linear;
}

bool ogl_shader_chain_preset_uses_feedback(void)
{
    if (!preset_loaded)
        return false;

    for (int i = 0; i < active_preset.pass_count; i++)
    {
        if (active_preset.passes[i].feedback)
            return true;
    }

    return false;
}

void ogl_shader_chain_get_preset_pass_output_size(int index, int source_width, int source_height, int previous_width, int previous_height, int viewport_width, int viewport_height, int* width, int* height)
{
    if (width)
        *width = viewport_width;
    if (height)
        *height = viewport_height;

    if (!preset_loaded || index < 0 || index >= active_preset.pass_count)
        return;

    const ShaderPresetPass* pass = &active_preset.passes[index];
    int output_width = scale_dimension(pass->scale_type_x, pass->scale_x, pass->absolute_width, source_width, previous_width, viewport_width);
    int output_height = scale_dimension(pass->scale_type_y, pass->scale_y, pass->absolute_height, source_height, previous_height, viewport_height);

    if (width)
        *width = MAX(1, output_width);
    if (height)
        *height = MAX(1, output_height);
}

int ogl_shader_chain_get_parameter_count(void)
{
    return preset_loaded ? active_preset.parameter_count : 0;
}

const ShaderPresetParameter* ogl_shader_chain_get_parameter(int index)
{
    if (!preset_loaded || index < 0 || index >= active_preset.parameter_count)
        return NULL;

    return &active_preset.parameters[index];
}

bool ogl_shader_chain_set_parameter(int index, float value)
{
    if (!preset_loaded || index < 0 || index >= active_preset.parameter_count)
        return false;

    ShaderPresetParameter* parameter = &active_preset.parameters[index];
    parameter->value = CLAMP(value, parameter->minimum, parameter->maximum);
    return true;
}

void ogl_shader_chain_apply_preset_uniforms(int index, int source_width, int source_height, int input_width, int input_height, int output_width, int output_height, int viewport_width, int viewport_height, float original_aspect)
{
    if (!preset_loaded || index < 0 || index >= active_preset.pass_count)
        return;

    PresetProgramState* state = &preset_programs[index];
    if (!state->program)
        return;

    if (state->uniform_source >= 0)
        glUniform1i(state->uniform_source, 0);
    if (state->uniform_original >= 0)
        glUniform1i(state->uniform_original, 1);
    if (state->uniform_feedback >= 0)
        glUniform1i(state->uniform_feedback, 2);

    if (state->uniform_source_size >= 0)
        glUniform4f(state->uniform_source_size, (float)input_width, (float)input_height, safe_inverse(input_width), safe_inverse(input_height));
    if (state->uniform_original_size >= 0)
        glUniform4f(state->uniform_original_size, (float)source_width, (float)source_height, safe_inverse(source_width), safe_inverse(source_height));
    if (state->uniform_output_size >= 0)
        glUniform4f(state->uniform_output_size, (float)output_width, (float)output_height, safe_inverse(output_width), safe_inverse(output_height));
    if (state->uniform_final_viewport_size >= 0)
        glUniform4f(state->uniform_final_viewport_size, (float)viewport_width, (float)viewport_height, safe_inverse(viewport_width), safe_inverse(viewport_height));
    if (state->uniform_feedback_size >= 0)
        glUniform4f(state->uniform_feedback_size, (float)pass_width, (float)pass_height, safe_inverse(pass_width), safe_inverse(pass_height));
    if (state->uniform_frame_count >= 0)
        glUniform1i(state->uniform_frame_count, preset_frame_count);
    if (state->uniform_frame_direction >= 0)
        glUniform1i(state->uniform_frame_direction, 1);
    if (state->uniform_original_aspect >= 0)
        glUniform1f(state->uniform_original_aspect, original_aspect);

    for (int i = 0; i < active_preset.parameter_count; i++)
    {
        if (state->uniform_parameters[i] >= 0)
            glUniform1f(state->uniform_parameters[i], active_preset.parameters[i].value);
    }

    if (index == active_preset.pass_count - 1)
        preset_frame_count++;
}

uint32_t ogl_shader_chain_get_source_texture(void)
{
    return source_texture;
}

uint32_t ogl_shader_chain_get_pass_texture(void)
{
    return pass_texture;
}

uint32_t ogl_shader_chain_get_pass_framebuffer(void)
{
    return pass_fbo;
}

uint32_t ogl_shader_chain_get_feedback_texture(void)
{
    return feedback_texture;
}

uint32_t ogl_shader_chain_get_feedback_framebuffer(void)
{
    return feedback_fbo;
}

uint32_t ogl_shader_chain_get_intermediate_texture(int index)
{
    if (index < 0 || index >= SHADER_PRESET_MAX_PASSES - 1)
        return 0;

    return intermediate_textures[index];
}

uint32_t ogl_shader_chain_get_intermediate_framebuffer(int index)
{
    if (index < 0 || index >= SHADER_PRESET_MAX_PASSES - 1)
        return 0;

    return intermediate_fbos[index];
}

int ogl_shader_chain_get_intermediate_width(int index)
{
    if (index < 0 || index >= SHADER_PRESET_MAX_PASSES - 1)
        return 1;

    return intermediate_widths[index];
}

int ogl_shader_chain_get_intermediate_height(int index)
{
    if (index < 0 || index >= SHADER_PRESET_MAX_PASSES - 1)
        return 1;

    return intermediate_heights[index];
}

int ogl_shader_chain_get_pass_width(void)
{
    return pass_width;
}

int ogl_shader_chain_get_pass_height(void)
{
    return pass_height;
}

static bool load_preset_programs(const ShaderPreset* preset, PresetProgramState* programs, char* error, size_t error_size)
{
    for (int i = 0; i < preset->pass_count; i++)
    {
        std::string source;
        if (!shader_preset_read_text_file(preset->passes[i].shader_path, source, error, error_size))
        {
            for (int j = 0; j < i; j++)
            {
                if (programs[j].program)
                    glDeleteProgram(programs[j].program);
            }
            return false;
        }

        uint32_t new_program = ogl_shader_program_create_fragment(preset->passes[i].shader_path, source.c_str(), error, error_size);
        if (!new_program)
        {
            for (int j = 0; j < i; j++)
            {
                if (programs[j].program)
                    glDeleteProgram(programs[j].program);
            }
            return false;
        }

        programs[i].program = new_program;
    }

    return true;
}

static void bind_preset_uniform_locations(PresetProgramState* state)
{
    uint32_t program = state->program;
    state->uniform_source = glGetUniformLocation(program, "Source");
    state->uniform_original = glGetUniformLocation(program, "Original");
    state->uniform_feedback = glGetUniformLocation(program, "PassFeedback0");
    state->uniform_source_size = glGetUniformLocation(program, "SourceSize");
    state->uniform_original_size = glGetUniformLocation(program, "OriginalSize");
    state->uniform_output_size = glGetUniformLocation(program, "OutputSize");
    state->uniform_final_viewport_size = glGetUniformLocation(program, "FinalViewportSize");
    state->uniform_feedback_size = glGetUniformLocation(program, "PassFeedback0Size");
    state->uniform_frame_count = glGetUniformLocation(program, "FrameCount");
    state->uniform_frame_direction = glGetUniformLocation(program, "FrameDirection");
    state->uniform_original_aspect = glGetUniformLocation(program, "OriginalAspect");

    for (int i = 0; i < SHADER_PRESET_MAX_PARAMETERS; i++)
        state->uniform_parameters[i] = -1;

    for (int i = 0; i < active_preset.parameter_count; i++)
        state->uniform_parameters[i] = glGetUniformLocation(program, active_preset.parameters[i].name);

    glUseProgram(program);
    if (state->uniform_source >= 0)
        glUniform1i(state->uniform_source, 0);
    if (state->uniform_original >= 0)
        glUniform1i(state->uniform_original, 1);
    if (state->uniform_feedback >= 0)
        glUniform1i(state->uniform_feedback, 2);
    glUseProgram(0);
}

static void clear_preset_state(void)
{
    memset(&active_preset, 0, sizeof(active_preset));
    memset(preset_programs, 0, sizeof(preset_programs));
    preset_loaded = false;
    preset_frame_count = 0;
}

static void delete_preset_programs(void)
{
    for (int i = 0; i < SHADER_PRESET_MAX_PASSES; i++)
    {
        if (preset_programs[i].program)
            glDeleteProgram(preset_programs[i].program);
        preset_programs[i].program = 0;
    }
}

static void clear_feedback_texture(void)
{
    if (!feedback_fbo)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, feedback_fbo);
    glViewport(0, 0, pass_width, pass_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void set_last_error(const char* message)
{
    strncpy_fit(last_error, message ? message : "Unknown shader preset error", sizeof(last_error));
}

static float safe_inverse(int value)
{
    return value > 0 ? 1.0f / (float)value : 0.0f;
}

static void configure_texture_2d(bool filter_linear)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_linear ? GL_LINEAR : GL_NEAREST);
}

static bool resize_framebuffer_texture(uint32_t texture, uint32_t fbo, int width, int height, bool filter_linear, const char* name)
{
    resize_texture_2d(texture, width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, NULL, filter_linear);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    bool complete = check_framebuffer_complete(name);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

static int scale_dimension(ShaderPresetScaleType scale_type, float scale, int absolute_size, int source_size, int previous_size, int viewport_size)
{
    switch (scale_type)
    {
        case ShaderPresetScale_Source:
            return (int)((float)source_size * scale + 0.5f);
        case ShaderPresetScale_Previous:
            return (int)((float)previous_size * scale + 0.5f);
        case ShaderPresetScale_Absolute:
            return absolute_size > 0 ? absolute_size : viewport_size;
        case ShaderPresetScale_Viewport:
        default:
            return (int)((float)viewport_size * scale + 0.5f);
    }
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