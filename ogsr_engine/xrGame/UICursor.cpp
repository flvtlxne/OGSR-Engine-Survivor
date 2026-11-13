#include "stdafx.h"
#include "UICursor.h"

#include "../xr_3da/IInputReceiver.h"

#include "ui/UIStatic.h"
#include "ui/UIBtnHint.h"

constexpr auto C_DEFAULT = color_xrgb(0xff, 0xff, 0xff);

CUICursor::CUICursor() : m_static(nullptr), m_b_use_win_cursor(false)
{
    bVisible = false;
    vPrevPos.set(0.0f, 0.0f);
    vPos.set(0.f, 0.f);
    InitInternal();
    Device.seqRender.Add(this, 1);
    Device.seqResolutionChanged.Add(this);
}
//--------------------------------------------------------------------
CUICursor::~CUICursor()
{
    xr_delete(m_static);
    Device.seqRender.Remove(this);
    Device.seqResolutionChanged.Remove(this);
}

void CUICursor::OnScreenResolutionChanged()
{
    xr_delete(m_static);
    InitInternal();
}

void CUICursor::InitInternal()
{
    m_static = xr_new<CUIStatic>();
    m_static->InitTextureEx("ui\\surv_cursor_hd", "hud\\cursor"); // Survivor: 128x128 cursor drawn at 40x40 units
    Frect rect;
    rect.set(0.0f, 0.0f, 128.0f, 128.0f);
    m_static->SetTextureRect(rect);
    Fvector2 sz;
    sz.set(40.0f, 40.0f);
    sz.x *= UI().get_current_kx();

    m_static->SetWndSize(sz);
    m_static->SetStretchTexture(true);

    u32 screen_size_x = GetSystemMetrics(SM_CXSCREEN);
    u32 screen_size_y = GetSystemMetrics(SM_CYSCREEN);
    m_b_use_win_cursor = (screen_size_y >= Device.dwHeight && screen_size_x >= Device.dwWidth);
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;
void CUICursor::OnRender()
{
    g_btnHint->OnRender();
    g_statHint->OnRender();

    if (!IsVisible())
        return;
#ifdef DEBUG
    last_render_frame = Device.dwFrame;

    if (bDebug)
    {
        CGameFont* F = UI().Font().pFontDI;
        F->SetAligment(CGameFont::alCenter);
        F->SetHeightI(0.02f);
        F->OutSetI(0.f, -0.9f);
        F->SetColor(0xffffffff);
        Fvector2 pt = GetCursorPosition();
        F->OutNext("%f-%f", pt.x, pt.y);
    }
#endif

    m_static->SetWndPos(vPos);
    m_static->Update();
    m_static->Draw();
}

Fvector2 CUICursor::GetCursorPosition() { return vPos; }

Fvector2 CUICursor::GetCursorPositionDelta()
{
    Fvector2 res_delta;

    res_delta.x = vPos.x - vPrevPos.x;
    res_delta.y = vPos.y - vPrevPos.y;
    return res_delta;
}

void CUICursor::UpdateCursorPosition(const int _dx, const int _dy)
{
    vPrevPos = vPos;

    const u32 screen_size_x = GetSystemMetrics(SM_CXSCREEN);
    const u32 screen_size_y = GetSystemMetrics(SM_CYSCREEN);

    const bool m_b_use_win_cursor = (screen_size_y >= Device.dwHeight && screen_size_x >= Device.dwWidth);

    if (m_b_use_win_cursor)
    {
        const bool is_fs = psDeviceFlags.test(rsFullscreen);

        POINT pti{};
        GetCursorPos(&pti);
        if (Device.m_hWnd)
            ScreenToClient(Device.m_hWnd, &pti);

        vPos.x = static_cast<float>(pti.x) * (UI_BASE_WIDTH / static_cast<float>(is_fs ? screen_size_x : Device.dwWidth));
        vPos.y = static_cast<float>(pti.y) * (UI_BASE_HEIGHT / static_cast<float>(is_fs ? screen_size_y : Device.dwHeight));
    }
    else
    {
        constexpr float sens = 1.0f;
        vPos.x += _dx * sens;
        vPos.y += _dy * sens;
    }

    clamp(vPos.x, 0.f, UI_BASE_WIDTH);
    clamp(vPos.y, 0.f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(const Fvector2& pos)
{
    vPos = pos;

    const int x = iFloor(vPos.x / (UI_BASE_WIDTH / (float)Device.dwWidth));
    const int y = iFloor(vPos.y / (UI_BASE_HEIGHT / (float)Device.dwHeight));

    ::SetCursorPos(x, y);
}
