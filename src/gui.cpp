#include <imgui.h>
#include <bgfx/bgfx.h>
#include <bx/math.h> 

#include "gui.h"
#include "rendererState.h"
#include "mathUtils.h"

static bgfx::TextureHandle g_FontTexture = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout g_VertexLayout;

// Much of this is blatantly copied from https://github.com/pr0g/sdl-bgfx-imgui-starter

void ImGui_Implbgfx_RenderDrawLists(ImDrawData* draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays
    // (screen coordinates != framebuffer coordinates)
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
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
    //bgfx::setViewRect(RENDER_SCREEN_ID, 0, 0, (uint16_t)fb_width, (uint16_t)fb_height);

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        uint32_t idx_buffer_offset = 0;

        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        uint32_t numVertices = (uint32_t)cmd_list->VtxBuffer.size();
        uint32_t numIndices = (uint32_t)cmd_list->IdxBuffer.size();

        if ((numVertices != bgfx::getAvailTransientVertexBuffer(
                                numVertices, g_VertexLayout)) ||
            (numIndices != bgfx::getAvailTransientIndexBuffer(numIndices))) {
            // not enough space in transient buffer, quit drawing the rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, g_VertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        memcpy(
            verts, cmd_list->VtxBuffer.begin(),
            numVertices * sizeof(ImDrawVert));

        ImDrawIdx* indices = (ImDrawIdx*)tib.data;
        memcpy(
            indices, cmd_list->IdxBuffer.begin(),
            numIndices * sizeof(ImDrawIdx));

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                const uint16_t xx = (uint16_t)bx::max(pcmd->ClipRect.x, 0.0f);
                const uint16_t yy = (uint16_t)bx::max(pcmd->ClipRect.y, 0.0f);
                bgfx::setScissor(
                    xx, yy, (uint16_t)bx::min(pcmd->ClipRect.z, 65535.0f) - xx,
                    (uint16_t)bx::min(pcmd->ClipRect.w, 65535.0f) - yy);

                bgfx::setState(state);
                bgfx::TextureHandle texture = {
                    (uint16_t)((intptr_t)pcmd->TextureId & 0xffff)};
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
        (uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8,
        0, bgfx::copy(pixels, width * height * 4));

    // Store our identifier
    io.Fonts->TexID = (void*)(intptr_t)g_FontTexture.idx;

    return true;
}

bool ImGui_Implbgfx_CreateDeviceObjects() {
    bgfx::RendererType::Enum type = bgfx::getRendererType();

    g_VertexLayout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    g_AttribLocationTex =
        bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);

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

void ImGui_Implbgfx_Shutdown()
{
    ImGui_Implbgfx_InvalidateDeviceObjects();
}

void ImGui_Implbgfx_NewFrame()
{
    if (!isValid(g_FontTexture)) {
        ImGui_Implbgfx_CreateDeviceObjects();
    }
}

void initGui() {
    ImGui::CreateContext();

    ImGui_Implbgfx_Init();

    ImGui::GetIO().DisplaySize = ImVec2(3440, 1440);
}

void drawGui() {
    ImGui_Implbgfx_NewFrame();

    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void terminateGui() {
    ImGui_Implbgfx_Shutdown();

    ImGui::DestroyContext();
}