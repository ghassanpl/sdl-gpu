#include "SDL_gpu_OpenGL_4.h"
#include "SDL_gpu_RendererImpl.h"


#if defined(SDL_GPU_DISABLE_OPENGL) || defined(SDL_GPU_DISABLE_OPENGL_4)

// Dummy implementations
GPU_Renderer* GPU_CreateRenderer_OpenGL_4(GPU_RendererID request) {return NULL;}
void GPU_FreeRenderer_OpenGL_4(GPU_Renderer* renderer) {}

#else


// Most of the code pulled in from here...
#define SDL_GPU_USE_OPENGL
#define SDL_GPU_USE_BUFFER_PIPELINE
#define SDL_GPU_ASSUME_CORE_FBO
#define SDL_GPU_ASSUME_SHADERS
#define SDL_GPU_SKIP_ENABLE_TEXTURE_2D
#define SDL_GPU_SKIP_LINE_WIDTH
#define SDL_GPU_GLSL_VERSION 150
#define SDL_GPU_GL_MAJOR_VERSION 4

#include "renderer_GL_common.inl"
#include "renderer_shapes_GL_common.inl"

static GPU_Image* CreateImageGL42(GPU_Renderer* renderer, Uint16 w, Uint16 h, GPU_FormatEnum format)
{
  /// First check if we support storage
  int major = 0, minor = 0;
  if (!(isExtensionSupported("ARB_texture_storage") || (get_GL_version(&major, &minor) && minor >= 2)))
    return CreateImage(renderer, w, h, format);

  GPU_Image* result;
  GLenum internal_format, data_format;
  static unsigned char* zero_buffer = NULL;
  static unsigned int zero_buffer_size = 0;

  if (format < 1)
  {
    GPU_PushErrorCode("GPU_CreateImage", GPU_ERROR_DATA_ERROR, "Unsupported image format (0x%x)", format);
    return NULL;
  }

  result = CreateUninitializedImage(renderer, w, h, format);

  if (result == NULL)
  {
    GPU_PushErrorCode("GPU_CreateImage", GPU_ERROR_BACKEND_ERROR, "Could not create image as requested.");
    return NULL;
  }

  changeTexturing(renderer, GPU_TRUE);
  bindTexture(renderer, result);

  internal_format = ((GPU_IMAGE_DATA*)(result->data))->internal_format;
  data_format = ((GPU_IMAGE_DATA*)(result->data))->format;
  w = result->w;
  h = result->h;
  if (!(renderer->enabled_features & GPU_FEATURE_NON_POWER_OF_TWO))
  {
    if (!isPowerOfTwo(w))
      w = (Uint16)getNearestPowerOf2(w);
    if (!isPowerOfTwo(h))
      h = (Uint16)getNearestPowerOf2(h);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
  glTexStorage2D(GL_TEXTURE_2D, 0, internal_format, w, h);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  
  result->texture_w = w;
  result->texture_h = h;

  return result;
}

GPU_Renderer* GPU_CreateRenderer_OpenGL_4(GPU_RendererID request)
{
    GPU_Renderer* renderer = (GPU_Renderer*)SDL_malloc(sizeof(GPU_Renderer));
    if(renderer == NULL)
        return NULL;

    memset(renderer, 0, sizeof(GPU_Renderer));

    renderer->id = request;
    renderer->id.renderer = GPU_RENDERER_OPENGL_4;
    renderer->shader_language = GPU_LANGUAGE_GLSL;
    renderer->min_shader_version = 110;
    renderer->max_shader_version = SDL_GPU_GLSL_VERSION;
    
    renderer->default_image_anchor_x = 0.5f;
    renderer->default_image_anchor_y = 0.5f;
    
    renderer->current_context_target = NULL;
    
    renderer->impl = (GPU_RendererImpl*)SDL_malloc(sizeof(GPU_RendererImpl));
    memset(renderer->impl, 0, sizeof(GPU_RendererImpl));
    SET_COMMON_FUNCTIONS(renderer->impl);

    renderer->impl->CreateImage = &CreateImageGL42;
    
    return renderer;
}

void GPU_FreeRenderer_OpenGL_4(GPU_Renderer* renderer)
{
    if(renderer == NULL)
        return;

    SDL_free(renderer->impl);
    SDL_free(renderer);
}


#endif
