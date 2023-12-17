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

//--------------------------------------------------------------------------------------------------
inline NiRWLock::NiRWLock()
    : m_uiNumReaders(0)
{
    m_hNoReadersEvent = CreateEventA(NULL, true, true, "NoReadersEvent");
}

//--------------------------------------------------------------------------------------------------
inline NiRWLock::~NiRWLock()
{
    CloseHandle(m_hNoReadersEvent);
}

//--------------------------------------------------------------------------------------------------
inline void NiRWLock::LockRead()
{
    m_kWriteLock.Lock();
    efd::AtomicIncrement(static_cast<NiUInt32>(m_uiNumReaders));
    ResetEvent(m_hNoReadersEvent);
    m_kWriteLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
inline void NiRWLock::LockWrite()
{
    m_kWriteLock.Lock();
    WaitForSingleObject(m_hNoReadersEvent, INFINITE);
}

//--------------------------------------------------------------------------------------------------
inline void NiRWLock::UnlockWrite()
{
    m_kWriteLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
inline void NiRWLock::UnlockRead()
{
    // read unlock
    if (efd::AtomicDecrement(static_cast<NiUInt32>(m_uiNumReaders)) == 0)
    {
        SetEvent(m_hNoReadersEvent);
    }
}

//--------------------------------------------------------------------------------------------------

