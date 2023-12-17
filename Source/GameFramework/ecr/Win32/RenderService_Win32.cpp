// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net
#include "ecrPCH.h"

#include "../RenderService.h"

#include "efd/Win32/Win32PlatformService.h"

#include <NiDX9Renderer.h>

#include <efd/IConfigManager.h>

#include <NiRendererSettings.h>
#include <NiSettingsDialog.h>
#include <NiBaseRendererSetup.h>

// DX9 renderer is the only one required by ecr.
#include <NiDX9Renderer.h>

using namespace egf;
using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
void RenderService::InternalDestructor()
{
    NiDX9Renderer* pDX9Renderer = NiDynamicCast(NiDX9Renderer, m_spRenderer);

    if (pDX9Renderer != NULL)
    {
        pDX9Renderer->RemoveLostDeviceNotificationFunc(&RenderService::OnDeviceLost);
        pDX9Renderer->RemoveResetNotificationFunc(&RenderService::OnDeviceReset);
    }
}

//------------------------------------------------------------------------------------------------
bool RenderService::CreateRenderer()
{
    // If you are using the Win32PlatformService for your main message loop then we can
    // automatically figure out the correct parent window handle to use.
    Win32PlatformService* pWin32 =
        m_pServiceManager->GetSystemServiceAs<Win32PlatformService>();
    if (!m_parentHandle && pWin32)
    {
        m_parentHandle = pWin32->GetWindowRef();
    }

    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();

    NiRendererSettings settings;
    char settingsFile[EE_MAX_PATH];
    bool validFile = efd::PathUtils::GetExecutableDirectory(settingsFile, sizeof(settingsFile));

    // Config manager/command line override settings file settings.
    if (validFile)
    {
        NiStrcat(settingsFile, sizeof(settingsFile), "AppSettings.ini");
        settings.LoadSettings(settingsFile);
    }
    settings.LoadFromConfigManager(pConfigManager);

    if (settings.m_bRendererDialog && pWin32)
    {
        NiSettingsDialog dialog(&settings);
        if (dialog.InitDialog(pWin32->GetInstanceRef()) &&
            dialog.ShowDialog(m_parentHandle, (NiAcceleratorRef)m_parentHandle))
        {
            if (settings.m_bSaveSettings && validFile)
                settings.SaveSettings(settingsFile);
        }
    }

    if (m_parentHandle && settings.m_uiScreenHeight && settings.m_uiScreenWidth)
    {
        // Resize it to new resolution
        RECT rect =
        {
            0,
            0,
            settings.m_uiScreenWidth,
            settings.m_uiScreenHeight
        };

        // Read the style from the window handle
        WINDOWINFO windowInfo;
        windowInfo.cbSize = sizeof(windowInfo);

        EE_VERIFY(GetWindowInfo(m_parentHandle, &windowInfo));

        AdjustWindowRect(&rect,
            windowInfo.dwStyle,
            false);

        SetWindowPos(
            m_parentHandle,
            HWND_TOP,
            0, 0,
            rect.right-rect.left, rect.bottom-rect.top,
            SWP_NOMOVE);
    }

    m_spRenderer = NiBaseRendererSetup::CreateRenderer(
        &settings,
        m_parentHandle,
        m_parentHandle);

    if (settings.m_eRendererID == efd::SystemDesc::RENDERER_DX9)
    {
        NiRenderer* pkRenderer = m_spRenderer;
        NiDX9Renderer* pDX9Renderer = NiVerifyStaticCast(NiDX9Renderer, pkRenderer);

        if (pDX9Renderer != NULL)
        {
            pDX9Renderer->AddLostDeviceNotificationFunc(&RenderService::OnDeviceLost, this);
            pDX9Renderer->AddResetNotificationFunc(&RenderService::OnDeviceReset, this);
        }
    }

    return (m_spRenderer != NULL);
}

//------------------------------------------------------------------------------------------------
RenderSurfacePtr RenderService::CreateRenderSurface(NiWindowRef windowHandle)
{
    if (!windowHandle)
        windowHandle = m_parentHandle;

    if (windowHandle != m_parentHandle &&
        !m_spRenderer->CreateWindowRenderTargetGroup(windowHandle))
    {
        return false;
    }


    // Create a new render surface entry for the back-buffer.
    RenderSurfacePtr spSurface = NiNew RenderSurface(windowHandle, this);

    if (windowHandle == m_parentHandle)
    {
        spSurface->GetSceneRenderClick()->SetRenderTargetGroup(
            m_spRenderer->GetDefaultRenderTargetGroup());
    }
    else
    {
        spSurface->GetSceneRenderClick()->SetRenderTargetGroup(
            m_spRenderer->GetWindowRenderTargetGroup(windowHandle));
    }

    if (!m_pActiveSurface)
        SetActiveRenderSurface(spSurface);

    return spSurface;
}

//------------------------------------------------------------------------------------------------
bool RenderService::DestroyRenderSurface(RenderSurface* pSurface)
{
    m_spRenderer->ReleaseWindowRenderTargetGroup(pSurface->GetWindowRef());

    return true;
}

//------------------------------------------------------------------------------------------------
bool RenderService::RecreateRenderSurface(RenderSurface* pSurface)
{
    EE_ASSERT(pSurface);

    // If the specified surface maps to the back-buffer of the device then
    // tell the NiRenderer to re-create the device. If the surface maps to a
    // swap chain, destroy the swap chain and re-create it.

    if (pSurface->GetRenderTargetGroup() == m_spRenderer->GetDefaultRenderTargetGroup())
    {
        // Do nothing; the device has already recreated itself and its swap chain.
    }
    else
    {
        if (m_spRenderer->GetRendererID() == efd::SystemDesc::RENDERER_DX9)
        {
            NiRenderer* pkRenderer = m_spRenderer;
            NiDX9Renderer* pDX9Renderer = NiVerifyStaticCast(NiDX9Renderer, pkRenderer);
            // If we don't have a valid device, don't try to recreate the render target group.
            if (!pDX9Renderer->LostDeviceRestore())
                return false;
        }

        // Swap chain.
        if (!m_spRenderer->RecreateWindowRenderTargetGroup(pSurface->GetWindowRef()))
        {
            return false;
        }
    }

    // Make sure to fix the aspect ratio on the default camera if it's present.
    NiRenderTargetGroup* pRenderTarget = pSurface->GetRenderTargetGroup();
    EE_ASSERT(pRenderTarget);

    NiCamera* pCamera = pSurface->GetCamera();
    if (pCamera)
    {
        float width = (float)pRenderTarget->GetWidth(0);
        float height = (float)pRenderTarget->GetHeight(0);

        float aspectRatio = width / height;

        pCamera->AdjustAspectRatio(aspectRatio);
    }

    // Notify any callbacks that the render surface has been recreated.
    for (DelegateList::iterator i = m_renderServiceDelegates.begin();
         i != m_renderServiceDelegates.end(); ++i)
    {
        (*i)->OnSurfaceRecreated(this, pSurface);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
