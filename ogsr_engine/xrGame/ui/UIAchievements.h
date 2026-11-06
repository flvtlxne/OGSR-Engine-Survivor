#pragma once
#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIHint;
class CUIScrollView;

class CUIAchievements : public CUIWindow
{
    typedef CUIWindow inherited;

private:
    CUIScrollView* m_parent;
    CUITextWnd* m_name;
    CUITextWnd* m_descr;
    CUIStatic* m_icon;
    UIHint* m_hint;
    string128 m_functor_str;
    bool m_repeat;
    // Survivor: progress hint for not yet received achievements
    string128 m_progress_functor_str;
    shared_str m_hint_text;
    shared_str m_progress_text;
    u32 m_name_color;
    u32 m_descr_color;
    bool m_locked;

public:
    CUIAchievements(CUIScrollView* parent);
    virtual ~CUIAchievements();

    void init_from_xml(CUIXml& xml);
    void Update();

    void SetName(LPCSTR name);
    void SetDescription(LPCSTR desc);
    void SetHint(LPCSTR hint);
    void SetIcon(LPCSTR icon);
    void SetFunctor(LPCSTR func);
    void SetRepeatable(bool repeat);
    void SetProgressFunctor(LPCSTR func);

    virtual void DrawHint();
    virtual void Reset();

protected:
    bool ParentHasMe();
    void SetLocked(bool locked);
};
