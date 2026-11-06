#include "stdafx.h"
#include "UIAchievements.h"
#include "UIScrollView.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "UIHint.h"
#include "UIStatic.h"
#include "UICursor.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "../string_table.h"

CUIAchievements::CUIAchievements(CUIScrollView* parent) : m_parent(parent), m_repeat(false), m_name_color(0), m_descr_color(0), m_locked(false)
{
    m_functor_str[0] = 0;
    m_progress_functor_str[0] = 0;
}

CUIAchievements::~CUIAchievements() { xr_delete(m_hint); }

void CUIAchievements::init_from_xml(CUIXml& xml)
{
    CUIXmlInit::InitWindow(xml, "achievements_itm", 0, this);

    XML_NODE* stored_root = xml.GetLocalRoot();
    XML_NODE* node = xml.NavigateToNode("achievements_itm", 0);
    xml.SetLocalRoot(node);

    m_name = UIHelper::CreateTextWnd(xml, "name", this);
    m_descr = UIHelper::CreateTextWnd(xml, "descr", this);
    m_icon = UIHelper::CreateStatic(xml, "icon", this);
    m_hint = UIHelper::CreateHint(xml, "hint_wnd");
    m_name_color = m_name->GetTextColor();
    m_descr_color = m_descr->GetTextColor();

    xml.SetLocalRoot(stored_root);
    Show(false);
}
void CUIAchievements::Update()
{
    if (ParentHasMe() && !m_repeat && !m_locked)
        return;

    luabind::functor<bool> f;
    R_ASSERT(ai().script_engine().functor(m_functor_str, f));
    const bool done = f();

    // Survivor: a not yet received achievement is shown grayed out if its progress functor returns a hint
    LPCSTR progress = nullptr;
    if (!done && m_progress_functor_str[0])
    {
        luabind::functor<LPCSTR> pf;
        R_ASSERT(ai().script_engine().functor(m_progress_functor_str, pf));
        progress = pf();
    }

    if (done || (progress && progress[0]))
    {
        SetLocked(!done);
        const shared_str hint = done ? m_hint_text : shared_str(progress);
        if (hint != m_progress_text)
        {
            m_progress_text = hint;
            m_hint->set_text(hint.c_str());
        }
        if (!ParentHasMe())
        {
            m_parent->AddWindow(this, false);
            Show(true);
        }
    }
    else
    {
        if (ParentHasMe())
        {
            m_parent->RemoveWindow(this);
            Show(false);
        }
    }
}
bool CUIAchievements::ParentHasMe()
{
    WINDOW_LIST::const_iterator it = std::find(m_parent->Items().begin(), m_parent->Items().end(), this);
    return it != m_parent->Items().end();
}
void CUIAchievements::SetName(LPCSTR name) { m_name->SetTextST(name); }

void CUIAchievements::SetDescription(LPCSTR desc)
{
    m_descr->SetTextST(desc);
    m_descr->AdjustHeightToText();
    Fvector2 descr_size = m_descr->GetWndSize();
    descr_size.y += 30.0f;
    if (descr_size.y > GetWndSize().y)
        SetWndSize(Fvector2().set(GetWndSize().x, descr_size.y));
}

void CUIAchievements::SetHint(LPCSTR hint)
{
    m_hint_text = CStringTable().translate(hint);
    m_hint->set_text(m_hint_text.c_str());
}

void CUIAchievements::SetIcon(LPCSTR icon) { m_icon->InitTexture(icon); }

void CUIAchievements::SetFunctor(LPCSTR func)
{
    //	string128 str = "xr_statistic.";
    xr_sprintf(m_functor_str, sizeof(m_functor_str), "%s", func);
}

void CUIAchievements::SetRepeatable(bool repeat) { m_repeat = repeat; }

void CUIAchievements::SetProgressFunctor(LPCSTR func) { xr_sprintf(m_progress_functor_str, sizeof(m_progress_functor_str), "%s", func); }

void CUIAchievements::SetLocked(bool locked)
{
    if (m_locked == locked)
        return;
    m_locked = locked;
    const u32 gray = color_argb(255, 110, 110, 110);
    m_icon->SetTextureColor(color_argb(locked ? 90 : 255, 255, 255, 255));
    m_name->SetTextColor(locked ? gray : m_name_color);
    m_descr->SetTextColor(locked ? gray : m_descr_color);
}

void CUIAchievements::DrawHint()
{
    Frect r;
    GetAbsoluteRect(r);
    Fvector2 pos = UI().GetUICursor().GetCursorPosition();
    if (r.in(pos))
        m_hint->Draw();
}

void CUIAchievements::Reset()
{
    if (ParentHasMe())
    {
        m_parent->RemoveWindow(this);
        Show(false);
    }
    inherited::Reset();
}