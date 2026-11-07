////////////////////////////////////////////////////////////////////////////
//  Created     : 19.06.2018
//  Authors     : Xottab_DUTY (OpenXRay project)
//                FozeSt
//                Unfainthful
//
//  Copyright (C) GSC Game World - 2018
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UILoadingScreen.h"
#include "../../xr_3da/GameFont.h"
#include "UIHelper.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "string_table.h"

UILoadingScreen::UILoadingScreen()
    : loadingProgress(nullptr), loadingProgressPercent(nullptr), loadingLogo(nullptr), loadingProgressBackground(nullptr), loadingStage(nullptr), loadingHeader(nullptr), loadingTipNumber(nullptr),
      loadingTip(nullptr), maxTip(100), loadingLevelName(nullptr), loadingLevelDescription(nullptr), loadingProgressShape(nullptr)
{
    UILoadingScreen::Initialize();
}

void UILoadingScreen::Initialize()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, "ui_mm_loading_screen.xml");

    const auto loadProgressBackground = [&]() { loadingProgressBackground = UIHelper::CreateStatic(uiXml, "loading_progress_background", this, false); };
    const auto loadProgressBar = [&]() { loadingProgress = UIHelper::CreateProgressBar(uiXml, "loading_progress", this, false); };
    const auto loadProgressShape = [&]() { loadingProgressShape = UIHelper::CreateProgressShape(uiXml, "loading_progress_shape", this, false); };

    const auto loadBackground = [&] { UIHelper::CreateStatic(uiXml, "background", this, false); };

    // Survivor: картинка уровня может рисоваться под фоном (under_background="1")
    const bool logoUnderBackground = uiXml.NavigateToNodeWithAttribute("loading_logo", "under_background", "1") != nullptr;
    if (logoUnderBackground)
        loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this, false);

    const auto node = uiXml.NavigateToNodeWithAttribute("loading_progress", "under_background", "0");
    if (node)
    {
        loadBackground();
        loadProgressBackground();
        loadProgressBar();
        loadProgressShape();
    }
    else
    {
        loadProgressBackground();
        loadProgressBar();
        loadProgressShape();
        loadBackground();
    }

    if (!logoUnderBackground)
        loadingLogo = UIHelper::CreateStatic(uiXml, "loading_logo", this, false);
    loadingProgressPercent = UIHelper::CreateStatic(uiXml, "loading_progress_percent", this, false);
    loadingStage = UIHelper::CreateStatic(uiXml, "loading_stage", this, false);

    loadingHeader = UIHelper::CreateStatic(uiXml, "loading_header", this, false);

    loadingTipNumber = UIHelper::CreateStatic(uiXml, "loading_tip_number", this, false);
    loadingTip = UIHelper::CreateStatic(uiXml, "loading_tip", this, false);

    maxTip = uiXml.ReadAttribInt("loading_tip", 0, "number_of_tips", maxTip);

    loadingLevelName = UIHelper::CreateStatic(uiXml, "loading_level_name", this, false);
    loadingLevelDescription = UIHelper::CreateStatic(uiXml, "loading_level_description", this, false);
}

void UILoadingScreen::Update(const int stagesCompleted, const int stagesTotal)
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    if (!IsShown())
    {
        if (force_stop)
            force_stop = false;
        if (force_drop)
            force_drop = false;
    }

    if (loadingProgressShape && !(force_stop || force_drop))
        loadingProgressShape->SetPos(stagesCompleted, stagesTotal);

    if (loadingProgress && !(force_stop || force_drop))
    {
        const float progress = float(stagesCompleted) / stagesTotal * loadingProgress->GetRange_max();

        loadingProgress->ForceSetProgressPos(progress);

        if (loadingProgressPercent)
        {
            char buf[5];
            xr_sprintf(buf, "%.0f%%", loadingProgress->GetProgressPos());
            loadingProgressPercent->TextItemControl()->SetText(buf);
        }
    }

    CUIWindow::Update();
    Draw();
}

void UILoadingScreen::ForceDrop()
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    force_drop = true;

    if (loadingProgressShape)
        loadingProgressShape->SetPos(0, 30);

    if (!loadingProgress)
        return;

    const float prev = loadingProgress->m_inertion;
    const float maximal = loadingProgress->GetRange_max();

    loadingProgress->m_inertion = 0.0f;
    loadingProgress->SetProgressPos(loadingProgress->GetRange_min());

    for (int i = 0; i < int(maximal); ++i)
    {
        loadingProgress->Update();
    }

    loadingProgress->m_inertion = prev;
}

void UILoadingScreen::ForceFinish()
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    force_stop = true;

    if (loadingProgressShape)
        loadingProgressShape->SetPos(30, 30);

    if (!loadingProgress)
        return;

    const float prev = loadingProgress->m_inertion;
    const float maximal = loadingProgress->GetRange_max();

    loadingProgress->m_inertion = 0.0f;
    loadingProgress->ForceSetProgressPos(maximal);

    for (int i = 0; i < int(maximal); ++i)
    {
        loadingProgress->Update();
    }

    loadingProgress->m_inertion = prev;
}

void UILoadingScreen::SetLevelLogo(const char* name)
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    if (loadingLogo)
    {
        Msg("~~[%s] set texture [%s] for loadingLogo", __FUNCTION__, name);
        loadingLogo->InitTexture(name);
    }
}

void UILoadingScreen::SetLevelText(const char* name)
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    if (!name)
        return;

    string512 levelDescription;

    if (loadingLevelName)
        loadingLevelName->TextItemControl()->SetText(CStringTable().translate(name).c_str());

    if (loadingLevelDescription)
    {
        xr_sprintf(levelDescription, "%s_description", name);
        loadingLevelDescription->TextItemControl()->SetText(CStringTable().translate(levelDescription).c_str());
    }
}

void UILoadingScreen::SetStageTitle(const char* title)
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    if (loadingStage)
        loadingStage->TextItemControl()->SetText(title);
}

void UILoadingScreen::SetStageTip()
{
    std::scoped_lock<decltype(loadingLock)> lock(loadingLock);

    u8 tip_num = Random.randI(1, maxTip);

    string512 buff;

    if (loadingHeader)
    {
        loadingHeader->TextItemControl()->SetText(CStringTable().translate("ls_header").c_str());
    }
    if (loadingTipNumber)
    {
        sprintf_s(buff, "%s%d:", CStringTable().translate("ls_tip_number").c_str(), tip_num);
        shared_str tipNumber = buff;
        loadingTipNumber->TextItemControl()->SetText(tipNumber.c_str());
    }
    if (loadingTip)
    {
        sprintf_s(buff, "ls_tip_%d", tip_num);
        loadingTip->TextItemControl()->SetText(CStringTable().translate(buff).c_str());
    }
}

void UILoadingScreen::Show(bool status) { CUIWindow::Show(status); }

bool UILoadingScreen::IsShown() { return CUIWindow::IsShown(); }
