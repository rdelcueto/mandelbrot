
#include <cstdint>
#include <iostream>
#include <complex>
#include <algorithm>
#include <math.h>
#include <SDL.h>

using Complex = std::complex<float>;
using uint = uint32_t;

typedef struct {
  float x_scale = 0.015625f;
  float y_scale = 0.015625f;

  float x_shift = 0.396092;
  float y_shift = -0.370313;

  int iterations = 256;
  int depth_threshold = 8;
} settings_t;

typedef struct {
  bool s_quit = false;
  bool s_invert = false;
  SDL_Window * cg_Window = nullptr;
  SDL_Renderer * cg_Renderer = nullptr;
  SDL_Texture * cg_Texture = nullptr;
  void * cg_TexturePixels;
  int cg_TexturePitch;
  uint r_CustomWidth = 640;
  uint r_CustomHeight = 480;
  uint cg_Field = 1;
} resourcesTable_t;

static settings_t settings;
static resourcesTable_t resourcesTable;

void PrintHelp( void ) {
  std::cout <<
    "Controls:\n" <<
    "\tArrows\t\t - Pan Movement\n" <<
    "\t./,\t\t - Zoom In/Out\n" <<
    "\t]/[\t\t - Iterations +/-\n" <<
    "\tHome/End\t - Depth Limit +/-\n" <<
    std::endl;
  return;
};

void keyHandler( SDL_KeyboardEvent key_event ) {
  switch ( key_event.keysym.sym ) {
  case SDLK_DOWN:
    settings.y_shift += 0.1 * settings.y_scale;
    std::cout << "YShift: " << settings.y_shift << std::endl;
    break;
  case SDLK_UP:
    settings.y_shift -= 0.1 * settings.y_scale;
    std::cout << "YShift: " << settings.y_shift << std::endl;
    break;
  case SDLK_RIGHT:
    settings.x_shift += 0.1 * settings.x_scale;
    std::cout << "XShift: " << settings.x_shift << std::endl;
    break;
  case SDLK_LEFT:
    settings.x_shift -= 0.1 * settings.x_scale;
    std::cout << "XShift: " << settings.x_shift << std::endl;
    break;
  case SDLK_PERIOD:
    settings.x_scale *= 0.5;
    settings.y_scale *= 0.5;
    std::cout << "Scale: " << settings.x_scale << std::endl;
    break;
  case SDLK_COMMA:
    settings.x_scale *= 2.0;
    settings.y_scale *= 2.0;
    std::cout << "Scale: " << settings.x_scale << std::endl;
    break;
  case SDLK_RIGHTBRACKET:
    if ( settings.iterations < 1024 ) {
      settings.iterations *= 2;
      std::cout << "Iterations: " << settings.iterations << std::endl;
    }
    break;
  case SDLK_LEFTBRACKET:
    if ( settings.iterations > 1 ) {
      settings.iterations /= 2;
      std::cout << "Iterations: " << settings.iterations << std::endl;
    }
    break;
  case SDLK_HOME:
    if ( settings.depth_threshold < 1024 ) {
      settings.depth_threshold *= 2;
      std::cout << "Depth: " << settings.depth_threshold << std::endl;
    }
    break;
  case SDLK_END:
    if ( settings.depth_threshold > 1 ) {
      settings.depth_threshold /= 2;
      std::cout << "Depth: " << settings.depth_threshold << std::endl;
    }
    break;
  case SDLK_ESCAPE:
    resourcesTable.s_quit = true;
    break;
  case SDLK_SPACE:
    resourcesTable.s_invert = true;
    break;
  }

  resourcesTable.cg_Field = 1;

  return;
}

void CG_TerminateSDL( void ) {
  if ( resourcesTable.cg_Texture != nullptr ) {
    SDL_DestroyTexture( resourcesTable.cg_Texture );
  }
  
  if ( resourcesTable.cg_Renderer != nullptr ) {
    SDL_DestroyRenderer( resourcesTable.cg_Renderer );
  }

  if ( resourcesTable.cg_Window != nullptr ) {
    SDL_DestroyWindow( resourcesTable.cg_Window );
  }

  SDL_Quit();
}

int CG_InitializeSDL( void ) {

  auto status = 0;

  SDL_LogSetPriority( SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO );

  status = SDL_Init( SDL_INIT_VIDEO );
  if ( status != 0 ) {
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION,
                  "Couldn't initialize SDL: %s\n",
                  SDL_GetError());
    return 1;
  }

  resourcesTable.cg_Window = SDL_CreateWindow( "Mandelbrot",
                                               100,
                                               100,
                                               resourcesTable.r_CustomWidth,
                                               resourcesTable.r_CustomHeight,
                                               SDL_WINDOW_SHOWN );

  if ( resourcesTable.cg_Window == nullptr ) {
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION,
                  "Couldn't create SDL window: %s\n",
                  SDL_GetError());
    CG_TerminateSDL();
    return 1;
  }

  resourcesTable.cg_Renderer =
    SDL_CreateRenderer( resourcesTable.cg_Window,
                        -1,
                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if ( resourcesTable.cg_Renderer == nullptr ) {
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION,
                  "Couldn't create SDL renderer: %s\n",
                  SDL_GetError());
    CG_TerminateSDL();
    return 1;
  }

  resourcesTable.cg_Texture = SDL_CreateTexture( resourcesTable.cg_Renderer,
                                                 SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 resourcesTable.r_CustomWidth,
                                                 resourcesTable.r_CustomHeight );

  SDL_RenderClear( resourcesTable.cg_Renderer );
  
  if ( resourcesTable.cg_Texture == nullptr ) {
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION,
                  "Couldn't create SDL texture: %s\n",
                  SDL_GetError());
    CG_TerminateSDL();
    return 1;
  }

  return 0;
}

float DemMandelbrot( Complex point ) {
 
  Complex z (0.0f);
  Complex dz (1.0f, 0.0f);
  const Complex one (1.0f, 1.0f);

  float nz = 0.0f;

  for ( auto i = 0; i < settings.iterations; i++ ) {
    if ( nz > settings.depth_threshold ) {
      break;
    }
    // Z_n+1' -> 2*Z*Z' + 1
    dz = 2.0f * ( z * dz ) + one;

    // Z_n+1  -> Z*Z + c
    z = ( z * z ) + point;
    
    nz = std::norm(z);
    i++;
  }

  return 0.5f * sqrtf((nz) / std::norm(dz)) * logf(nz); 
}

Complex MappingFunction( uint x, uint y, uint max_x, uint max_y ) {
    
  const float x_0 = (-2.0 * settings.x_scale) + settings.x_shift;
  const float x_1 = ( 2.0 * settings.x_scale) + settings.x_shift;

  const float y_0 = (-2.0 * settings.y_scale) + settings.y_shift;
  const float y_1 = ( 2.0 * settings.y_scale) + settings.y_shift;
    
  return Complex( ((double) x) / max_x * ( x_1 - x_0 ) + x_0,
                  ((double) y) / max_y * ( y_1 - y_0 ) + y_0 );
}


void RenderPixels( void ) {
  const auto dim_x = resourcesTable.r_CustomWidth;
  const auto dim_y = resourcesTable.r_CustomHeight;

  int start_at = 0;

  switch ( resourcesTable.cg_Field ) {
  case 1:
    resourcesTable.cg_Field = 2;
    break;
  case 2:
    start_at = 1;
    resourcesTable.cg_Field = 0;
    break;
  default:
    return;
    break;
  }

  const float p = powf( 2.0f, 20.0);

  #pragma omp parallel for schedule(static, 16)
  for ( auto row = 0; row < dim_y; row++ ) {
    auto * pixel =
      (uint *)
      ((uint8_t *) resourcesTable.cg_TexturePixels +
       row * resourcesTable.cg_TexturePitch ) + start_at;
   
    for ( auto col = start_at; col < dim_x; col+=2, pixel+=2 ) {
      const auto target_point = MappingFunction( col, row, dim_x, dim_y );
      const auto distance = DemMandelbrot ( target_point );

      auto color = distance * p;
      color = std::min(color, 255.0f);
      color = std::max(color, 0.0f);

      uint icolor = (uint) ( color );

      *pixel = (0xff << 24)  | ( icolor << 16) | ( icolor << 8) | ( icolor );
    }
  }
  // SDL_Quit();
}

int DrawFrame( void ) {
  if ( SDL_LockTexture( resourcesTable.cg_Texture,
                        nullptr,
                        &resourcesTable.cg_TexturePixels,
                        &resourcesTable.cg_TexturePitch ) < 0 ) {
    
    SDL_LogError( SDL_LOG_CATEGORY_APPLICATION,
                  "Unable to lock texture %s\n",
                  SDL_GetError() );
    return 1;
  }

  RenderPixels();

  SDL_UnlockTexture( resourcesTable.cg_Texture );
  
  SDL_RenderClear( resourcesTable.cg_Renderer );
  SDL_RenderCopy( resourcesTable.cg_Renderer,
                  resourcesTable.cg_Texture,
                  NULL,
                  NULL );
  SDL_RenderPresent( resourcesTable.cg_Renderer );
  return 0;
}

uint G_MainLoop( void ) {
  
  SDL_Event sdl_event;
  uint t0;
  uint t1;
  uint t2;

  auto status = 0;
 
  while( !resourcesTable.s_quit && status == 0 ) {
 
    t0 = SDL_GetTicks();

    // Handle events on queue
    while ( SDL_PollEvent(&sdl_event) ) {
      
      switch ( sdl_event.type ) {
      case SDL_KEYDOWN:
        keyHandler( sdl_event.key );
        break;
        
      case SDL_QUIT:
        resourcesTable.s_quit = true;
        break;
      }
    }

    status = DrawFrame();

    // Sleep if rendering too fast
    t1 = SDL_GetTicks();
    t2 = t1 - t0;

    //std::cout << t2 << std::endl;
    
    if ( t2 < 20 ) {
      SDL_Delay( 20 - t2 );
    }
  }

  return status;
}

int main( int argc, char ** argv ) {
  auto status = 0;
  status = CG_InitializeSDL();
  
  if ( status ) {
    return status;
  }

  PrintHelp();

  status = G_MainLoop();
 
  CG_TerminateSDL();

  std::cout << "GOODBYE!" << std::endl;
  return status;
}
