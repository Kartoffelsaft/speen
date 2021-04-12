#include <imgui.h>
#include <bgfx/bgfx.h>
#include <bx/math.h> 

#include "guiImpl.h"
#include "rendererState.h"
#include "mathUtils.h"
#include "config.h"

static bgfx::TextureHandle g_FontTexture = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout  g_VertexLayout;

static char* clipboardContents = nullptr;
static SDL_Cursor* cursors[ImGuiMouseCursor_COUNT];

// Much of this is blatantly copied from https://github.com/pr0g/sdl-bgfx-imgui-starter

void ImGui_Implbgfx_RenderDrawLists(ImDrawData* draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays
    // (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width  = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }

    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup render state: alpha-blending enabled, no face culling,
    // no depth testing, scissor enabled
    uint64_t state = 
        BGFX_STATE_WRITE_RGB 
      | BGFX_STATE_WRITE_A 
      | BGFX_STATE_MSAA 
      | BGFX_STATE_BLEND_FUNC(
            BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA
        );

    const bgfx::Caps* caps = bgfx::getCaps();

    // Setup viewport, orthographic projection matrix
    Mat4 ortho;
    bx::mtxOrtho(
        ortho.data(), 
        0.0f, 
        io.DisplaySize.x, 
        io.DisplaySize.y, 
        0.0f, 
        -1.0f, 
        1.0f, 
        0.0f, 
        caps->homogeneousDepth
    );
    bgfx::setViewTransform(RENDER_SCREEN_ID, NULL, ortho.data());

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        uint32_t idx_buffer_offset = 0;

        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        uint32_t numVertices = (uint32_t)cmd_list->VtxBuffer.size();
        uint32_t numIndices  = (uint32_t)cmd_list->IdxBuffer.size();

        if ((numVertices != bgfx::getAvailTransientVertexBuffer(numVertices, g_VertexLayout)) 
        || ( numIndices  != bgfx::getAvailTransientIndexBuffer (numIndices))) {
            // not enough space in transient buffer, quit drawing the rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, g_VertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        memcpy(
            verts, 
            cmd_list->VtxBuffer.begin(),
            numVertices * sizeof(ImDrawVert)
        );

        ImDrawIdx* indices = (ImDrawIdx*)tib.data;
        memcpy(
            indices, 
            cmd_list->IdxBuffer.begin(),
            numIndices * sizeof(ImDrawIdx)
        );

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                const uint16_t xx = (uint16_t)bx::max(pcmd->ClipRect.x, 0.0f);
                const uint16_t yy = (uint16_t)bx::max(pcmd->ClipRect.y, 0.0f);
                bgfx::setScissor(
                    xx, 
                    yy, 
                    (uint16_t)bx::min(pcmd->ClipRect.z, 65535.0f) - xx,
                    (uint16_t)bx::min(pcmd->ClipRect.w, 65535.0f) - yy
                );

                bgfx::setState(state);
                bgfx::TextureHandle texture = {
                    (uint16_t)((intptr_t)pcmd->TextureId & 0xffff)
                };
                bgfx::setTexture(0, g_AttribLocationTex, texture);
                bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
                bgfx::setIndexBuffer(&tib, idx_buffer_offset, pcmd->ElemCount);
                bgfx::submit(RENDER_SCREEN_ID, rendererState.screenProgram);
            }

            idx_buffer_offset += pcmd->ElemCount;
        }
    }
}

bool ImGui_Implbgfx_CreateFontsTexture() {
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    g_FontTexture = bgfx::createTexture2D(
        (uint16_t)width, 
        (uint16_t)height, 
        false, 
        1, 
        bgfx::TextureFormat::BGRA8,
        0, 
        bgfx::copy(pixels, width * height * 4)
    );

    // Store our identifier
    io.Fonts->TexID = (void*)(intptr_t)g_FontTexture.idx;

    return true;
}

bool ImGui_Implbgfx_CreateDeviceObjects() {
    g_VertexLayout.begin()
        .add(bgfx::Attrib::Position , 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Uint8, true)
        .end();

    g_AttribLocationTex = bgfx::createUniform(
        "g_AttribLocationTex", 
        bgfx::UniformType::Sampler
    );

    ImGui_Implbgfx_CreateFontsTexture();

    return true;
}

void ImGui_Implbgfx_InvalidateDeviceObjects() {
    bgfx::destroy(g_AttribLocationTex);

    if (isValid(g_FontTexture)) {
        bgfx::destroy(g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture.idx = bgfx::kInvalidHandle;
    }
}

void ImGui_Implbgfx_Init() {
}

void ImGui_Implbgfx_Shutdown() {
    ImGui_Implbgfx_InvalidateDeviceObjects();
}

void ImGui_Implbgfx_NewFrame() {
    if (!isValid(g_FontTexture)) {
        ImGui_Implbgfx_CreateDeviceObjects();
    }
}

// TODO: implement mouse cursors (like the I bar when you mouse ove editable text)

char const * ImGui_ImplSDL2_GetClipboardText(void* _) {
    if(clipboardContents) SDL_free(clipboardContents);
    clipboardContents = SDL_GetClipboardText();
    return clipboardContents;
}

void ImGui_ImplSDL2_SetClipboardText(void* _, char const * text) {
    SDL_SetClipboardText(text);
}

bool ImGui_ImplSDL2_ProcessEvent(SDL_Event const event) {
    ImGuiIO& io = ImGui::GetIO();
    switch (event.type) {
        case SDL_MOUSEWHEEL: {
            io.MouseWheel  += event.wheel.y;
            io.MouseWheelH += event.wheel.x;

            return io.WantCaptureMouse;
        }
        case SDL_MOUSEBUTTONDOWN: 
        case SDL_MOUSEBUTTONUP: 
        case SDL_MOUSEMOTION: {
            int mx, my;
            Uint32 mButtons = SDL_GetMouseState(&mx, &my);
            io.MouseDown[0] = mButtons & SDL_BUTTON(SDL_BUTTON_LEFT);
            io.MouseDown[1] = mButtons & SDL_BUTTON(SDL_BUTTON_RIGHT);
            io.MouseDown[2] = mButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
            io.MousePos = ImVec2((float)mx, (float)my);

            if(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
                ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
                if(io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
                    SDL_ShowCursor(SDL_FALSE);
                } else {
                    SDL_SetCursor(
                        cursors[cursor]?
                        cursors[cursor] : cursors[ImGuiMouseCursor_Arrow]
                    );
                }
            }

            return io.WantCaptureMouse;
        }
        case SDL_TEXTINPUT: {
            io.AddInputCharactersUTF8(event.text.text);
            return io.WantCaptureKeyboard;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            auto key = event.key.keysym.scancode;
            io.KeysDown[key] = event.type == SDL_KEYDOWN;

            auto mods = SDL_GetModState();
            io.KeyShift = mods & KMOD_SHIFT;
            io.KeyCtrl  = mods & KMOD_CTRL;
            io.KeyAlt   = mods & KMOD_ALT;

            return io.WantCaptureKeyboard;
        }
    }

    return false;
}

void ImGui_ImplSDL2_Init() {
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2(
        config.graphics.resolutionX,
        config.graphics.resolutionY
    );

    io.KeyMap[ImGuiKey_Tab        ] = SDL_SCANCODE_TAB      ;
    io.KeyMap[ImGuiKey_LeftArrow  ] = SDL_SCANCODE_LEFT     ;
    io.KeyMap[ImGuiKey_RightArrow ] = SDL_SCANCODE_RIGHT    ;
    io.KeyMap[ImGuiKey_UpArrow    ] = SDL_SCANCODE_UP       ;
    io.KeyMap[ImGuiKey_DownArrow  ] = SDL_SCANCODE_DOWN     ;
    io.KeyMap[ImGuiKey_PageUp     ] = SDL_SCANCODE_PAGEUP   ;
    io.KeyMap[ImGuiKey_PageDown   ] = SDL_SCANCODE_PAGEDOWN ;
    io.KeyMap[ImGuiKey_Home       ] = SDL_SCANCODE_HOME     ;
    io.KeyMap[ImGuiKey_End        ] = SDL_SCANCODE_END      ;
    io.KeyMap[ImGuiKey_Insert     ] = SDL_SCANCODE_INSERT   ;
    io.KeyMap[ImGuiKey_Delete     ] = SDL_SCANCODE_DELETE   ;
    io.KeyMap[ImGuiKey_Backspace  ] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space      ] = SDL_SCANCODE_SPACE    ;
    io.KeyMap[ImGuiKey_Enter      ] = SDL_SCANCODE_RETURN   ;
    io.KeyMap[ImGuiKey_Escape     ] = SDL_SCANCODE_ESCAPE   ;
    io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER ;
    io.KeyMap[ImGuiKey_A          ] = SDL_SCANCODE_A        ;
    io.KeyMap[ImGuiKey_C          ] = SDL_SCANCODE_C        ;
    io.KeyMap[ImGuiKey_V          ] = SDL_SCANCODE_V        ;
    io.KeyMap[ImGuiKey_X          ] = SDL_SCANCODE_X        ;
    io.KeyMap[ImGuiKey_Y          ] = SDL_SCANCODE_Y        ;
    io.KeyMap[ImGuiKey_Z          ] = SDL_SCANCODE_Z        ;

    cursors[ImGuiMouseCursor_Arrow     ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW   );
    cursors[ImGuiMouseCursor_TextInput ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM   );
    cursors[ImGuiMouseCursor_ResizeAll ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL );
    cursors[ImGuiMouseCursor_ResizeNS  ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS  );
    cursors[ImGuiMouseCursor_ResizeEW  ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE  );
    cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    cursors[ImGuiMouseCursor_Hand      ] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND    );
    cursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO      );

    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
    io.ClipboardUserData  = nullptr;
}

void ImGui_ImplSDL2_Shutdown() {
    if(clipboardContents) {
        SDL_free(clipboardContents);
    }

    for(ImGuiMouseCursor cursor; cursor < ImGuiMouseCursor_COUNT; cursor++) {
        SDL_FreeCursor(cursors[cursor]);
    }
}

void ImGui_ImplSDL2_NewFrame(float const frameTime) {
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = frameTime;
}