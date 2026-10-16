#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#ifdef NDEBUG
#define GPU_DEBUG_MODE false
#else
#define GPU_DEBUG_MODE true
#endif

#if defined(SDL_PLATFORM_WINDOWS)
#define GPU_SHADER_FORMAT SDL_GPU_SHADERFORMAT_DXIL
#elif defined(SDL_PLATFORM_LINUX)
#define GPU_SHADER_FORMAT SDL_GPU_SHADERFORMAT_SPIRV
#elif defined(SDL_PLATFORM_MACOS)
#define GPU_SHADER_FORMAT SDL_GPU_SHADERFORMAT_MSL
#else
#error "Unsupported platform: no shader format defined"
#endif

typedef struct AppState
{
    SDL_Window* window;
    SDL_GPUDevice* device;
} AppState;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState* state = SDL_calloc(1, sizeof(*state));
    if (!state)
    {
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    state->device = SDL_CreateGPUDevice(GPU_SHADER_FORMAT, GPU_DEBUG_MODE, NULL);
    if (!state->device)
    {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_Log("GPU driver: %s", SDL_GetGPUDeviceDriver(state->device));

    state->window = SDL_CreateWindow("GS", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!state->window)
    {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(state->device, state->window))
    {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    (void)appstate;

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)
    {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    AppState* state = appstate;

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(state->device);
    if (!cmd)
    {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture* swapchain = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, state->window, &swapchain, NULL, NULL))
    {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd);
        return SDL_APP_FAILURE;
    }

    // The swapchain texture is NULL while the window is minimized or occluded.
    if (swapchain)
    {
        SDL_GPUColorTargetInfo target = {
            .texture = swapchain,
            .clear_color = {.r = 0.12f, .g = 0.12f, .b = 0.16f, .a = 1.0f},
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, NULL);
        SDL_EndGPURenderPass(pass);
    }

    if (!SDL_SubmitGPUCommandBuffer(cmd))
    {
        SDL_Log("SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    (void)result;

    AppState* state = appstate;
    if (state)
    {
        if (state->device)
        {
            if (state->window)
            {
                SDL_ReleaseWindowFromGPUDevice(state->device, state->window);
            }
            SDL_DestroyGPUDevice(state->device);
        }
        if (state->window)
        {
            SDL_DestroyWindow(state->window);
        }
        SDL_free(state);
    }
}
