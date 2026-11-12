#include "stdafx.h"
#include "UIZoneMap.h"

#include "InfoPortion.h"
#include "PDA.h"

#include "Grenade.h"
#include "Level.h"
#include "game_cl_base.h"

#include "Actor.h"
#include "ai_space.h"
#include "game_graph.h"

#include "ui/UIMap.h"
#include "ui/UIXmlInit.h"
#include "ui/UIHelper.h"
#include "ui/UIInventoryUtilities.h"
//////////////////////////////////////////////////////////////////////////

CUIZoneMap::CUIZoneMap() : m_current_map_idx(u8(-1)), visible(true)
{
    m_background = xr_new<CUIStatic>();
    m_clipFrame = xr_new<CUIStatic>();
    m_center = xr_new<CUIStatic>();
    m_compass = xr_new<CUIStatic>();
    m_activeMap = xr_new<CUIMiniMap>();
}

CUIZoneMap::~CUIZoneMap() {}

void CUIZoneMap::Init()
{
    ZoneScoped;

    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, "zone_map.xml");

    CUIXmlInit xml_init;
    xml_init.InitStatic(uiXml, "minimap:background", 0, m_background);

    m_pointerDistanceText = UIHelper::CreateStatic(uiXml, "minimap:background:dist_text", m_background, false);

    xml_init.InitStatic(uiXml, "minimap:level_frame", 0, m_clipFrame);
    xml_init.InitStatic(uiXml, "minimap:center", 0, m_center);

    m_clock_wnd_background = UIHelper::CreateStatic(uiXml, "minimap:time_frame", m_background, false);
    m_clock_wnd = UIHelper::CreateStatic(uiXml, "minimap:clock_wnd", m_background, false);

    m_rounded = uiXml.ReadAttribInt("minimap", 0, "rounded", Core.Features.test(xrCore::Feature::cop_rounded_minimap)) == 1;
    m_use_cop_sizes = uiXml.ReadAttribInt("minimap", 0, "use_cop_sizes", Core.Features.test(xrCore::Feature::cop_rounded_minimap)) == 1; 
    m_alpha = uiXml.ReadAttribInt("minimap", 0, "alpha", 127);

    m_clipFrame->AttachChild(m_activeMap);
    m_activeMap->SetAutoDelete(true);

    m_activeMap->EnableHeading(true);
    m_activeMap->SetWindowName("minimap");
    m_activeMap->SetRounded(m_rounded);

    xml_init.InitStatic(uiXml, "minimap:compass", 0, m_compass);
    m_background->AttachChild(m_compass);

    m_activeMap->SetTextureColor(color_argb(m_alpha, 255, 255, 255));

    if (m_Counter = UIHelper::CreateStatic(uiXml, "minimap:static_counter", m_background, false))
        m_Counter_text = UIHelper::CreateStatic(uiXml, "minimap:static_counter:text_static", m_Counter, false);

    m_clipFrame->AttachChild(m_center);

    if (m_Counter_text)
        m_Counter_text->SetText("");

    visible = true;

    Fvector2 temp;
    const float k = UI().get_current_kx();

    if (m_use_cop_sizes)
    {
        temp = m_clipFrame->GetWndSize();
        temp.y *= UI_BASE_HEIGHT * k;
        temp.x = temp.y / k;
        m_clipFrame->SetWndSize(temp);

        temp = m_clipFrame->GetWndPos();
        m_clipFrame->SetWndPos(temp.mul(UI_BASE_HEIGHT));
    }

    if (m_use_cop_sizes)
    {
        m_background->SetHeight(m_background->GetHeight() * UI_BASE_HEIGHT);
        m_background->SetWidth(m_background->GetHeight() * k);

        m_clipFrame->GetWndRect().getcenter(temp);
        m_background->SetWndPos(temp);
    }

    temp = m_clipFrame->GetWndSize();
    m_center->SetWndPos(temp.div(2.0f));

    if (m_use_cop_sizes)
    {
        temp = m_compass->GetWndPos();
        temp.mul(m_background->GetWndSize());
        m_compass->SetWndPos(temp);
    }

    if (m_clock_wnd && m_use_cop_sizes)
    {
        if (m_clock_wnd_background)
        {
            temp = m_clock_wnd_background->GetWndPos();
            temp.mul(m_background->GetWndSize());
            m_clock_wnd_background->SetWndPos(temp);
        }
        temp = m_clock_wnd->GetWndPos();
        temp.mul(m_background->GetWndSize());
        m_clock_wnd->SetWndPos(temp);
    }

    if (m_Counter && m_use_cop_sizes)
    {
        temp = m_Counter->GetWndPos();
        temp.mul(m_background->GetWndSize());
        m_Counter->SetWndPos(temp);
    }
    m_fScale = 1.f;
}

void CUIZoneMap::Render()
{
    if (!visible)
        return;

    m_clipFrame->Draw();
    m_background->Draw();
}

void CUIZoneMap::Update()
{
    if (!visible)
        return;

    const auto* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    if (!(Device.dwFrame % 20) && m_Counter_text)
    {
        string16 text_str;
        xr_strcpy(text_str, sizeof(text_str), "");

        if (CPda* pda = pActor->GetPDA())
        {
            const u32 cn = pda->ActiveContactsNum();
            if (cn > 0)
                xr_sprintf(text_str, sizeof(text_str), "%d", cn);
        }

        m_Counter_text->SetText(text_str);
        m_Counter->Show(text_str[0] != '\0'); // Survivor: no icon without contacts
    }

    UpdateRadar(Device.vCameraPosition);
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    SetHeading(-h);

    if (m_clock_wnd)
        m_clock_wnd->TextItemControl()->SetText(InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());
}

void CUIZoneMap::SetHeading(float angle)
{
    m_activeMap->SetHeading(angle);
    m_compass->SetHeading(angle);
};

void CUIZoneMap::UpdateRadar(Fvector pos)
{
    m_clipFrame->Update();
    m_background->Update();
    m_activeMap->SetActivePoint(pos);

    if (m_pointerDistanceText)
    {
        if (m_activeMap->GetPointerDistance() > 0.5f)
        {
            string64 str;
            sprintf_s(str, "%.0f m", m_activeMap->GetPointerDistance());
            m_pointerDistanceText->SetText(str);
        }
        else
        {
            m_pointerDistanceText->SetText("");
        }
    }
}

bool CUIZoneMap::ZoomIn()
{
    m_fScale = m_fScale + m_fScale * 0.25f;
    clamp(m_fScale, 0.5f, 2.f);
    ApplyZoom();

    return true;
}

bool CUIZoneMap::ZoomOut()
{
    m_fScale = m_fScale - m_fScale * 0.25f;
    clamp(m_fScale, 0.5f, 2.f);
    ApplyZoom();

    return true;
}

void CUIZoneMap::SetupCurrentMap()
{
    m_activeMap->Initialize(Level().name(), "hud\\default");
    m_activeMap->SetWindowName("minimap"); // имя нужно задавать позже чем Init

    m_activeMap->SetTextureColor(color_argb(m_alpha, 255, 255, 255));

    Frect r;
    m_clipFrame->GetAbsoluteRect(r);
    m_activeMap->WorkingArea().set(r);

    ApplyZoom();
}

void CUIZoneMap::ApplyZoom()
{
    Fvector2 wnd_size{};
    float zoom_factor = float(m_clipFrame->GetWidth()) / 100.0f;

    LPCSTR ln = Level().name().c_str();
    if (pGameIni->section_exist(ln))
    {
        if (pGameIni->line_exist(ln, "minimap_zoom"))
            zoom_factor *= pGameIni->r_float(ln, "minimap_zoom");
    }
    else if (g_pGameLevel->pLevel->section_exist("minimap_zoom"))
    {
        zoom_factor *= g_pGameLevel->pLevel->r_float("minimap_zoom", "value");
    }
    wnd_size.x = m_activeMap->BoundRect().width() * zoom_factor * m_fScale;
    wnd_size.y = m_activeMap->BoundRect().height() * zoom_factor * m_fScale;
    m_activeMap->SetWndSize(wnd_size);
}

void CUIZoneMap::OnSectorChanged(IRender_Sector::sector_id_t sector)
{
    if (!g_pGameLevel->pLevel->section_exist("sub_level_map"))
        return;
    u8 map_idx = u8(-1);
    string64 s_sector;
    xr_sprintf(s_sector, "%zd", sector);

    if (!g_pGameLevel->pLevel->line_exist("sub_level_map", s_sector))
        return;

    map_idx = g_pGameLevel->pLevel->r_u8("sub_level_map", s_sector);
    if (m_current_map_idx == map_idx)
        return;

    m_current_map_idx = map_idx;

    string_path sub_texture;
    xr_sprintf(sub_texture, "%s#%d", m_activeMap->m_texture.c_str(), m_current_map_idx);

    if (map_idx == u8(-1))
        xr_sprintf(sub_texture, "%s", m_activeMap->m_texture.c_str());

    m_activeMap->InitTextureEx(sub_texture, m_activeMap->m_shader_name.c_str());
}

void CUIZoneMap::Counter_ResetClrAnimation()
{
    if (m_Counter_text)
        m_Counter_text->ResetColorAnimation();
}
