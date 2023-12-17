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

// Precompiled Header
#include "NiMainPCH.h"

#include "NiAVObject.h"
#include "NiNode.h"
#include "NiCollisionObject.h"

//--------------------------------------------------------------------------------------------------
void NiAVObject::UpdateWorldData()
{
    if (m_pkParent)
        m_kWorld = m_pkParent->m_kWorld * m_kLocal;
    else
        m_kWorld = m_kLocal;

    if (m_spCollisionObject)
        m_spCollisionObject->UpdateWorldData();
}

//--------------------------------------------------------------------------------------------------
