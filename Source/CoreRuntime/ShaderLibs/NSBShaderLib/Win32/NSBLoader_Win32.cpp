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
// Precompiled Header
#include "NSBShaderLibPCH.h"

#include "NSBLoader.h"

//--------------------------------------------------------------------------------------------------
unsigned int NSBLoader::LoadAllNSBFilesInDirectory(const char* pcDirectory,
    const char* pcExt, bool bRecurseDirectories,
    NiTPointerList<char*>* pkFileList)
{
    if (!pcDirectory || (strcmp(pcDirectory, "") == 0))
        return 0;
    if (!pcExt || (strcmp(pcExt, "") == 0))
        return 0;

    unsigned int uiCount    = 0;
    char acFilePath[NI_MAX_PATH];

    NiStrncpy(acFilePath, NI_MAX_PATH, pcDirectory, NI_MAX_PATH - 1);
    size_t stLen = strlen(acFilePath);
    if ((acFilePath[stLen - 1] != '\\') && (acFilePath[stLen - 1] != '/'))
    {
        acFilePath[stLen] = '\\';
        acFilePath[stLen + 1] = 0;
    }

    NiPath::Standardize(acFilePath);

    WIN32_FIND_DATA wfd ;
    HANDLE hFile = NULL;
    char acFileName[NI_MAX_PATH];
    char acFileName2[NI_MAX_PATH];
    DWORD dwAttrib;
    bool bDone = false;

    memset(&wfd, 0, sizeof(WIN32_FIND_DATA));

    NiStrcpy(acFileName, NI_MAX_PATH, pcDirectory);
    stLen = strlen(acFileName);
    if ((acFileName[stLen - 1] != '\\') && (acFileName[stLen - 1] != '/'))
    {
        acFileName[stLen] = '\\';
        acFileName[stLen + 1] = 0;
    }
    // This will cover the case when the directory is a mapped network
    // drive...
    NiStrcat(acFileName, NI_MAX_PATH, "*");

    NiPath::Standardize(acFileName);

    hFile = FindFirstFile(acFileName, &wfd);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        NiStrcpy(acFileName2, NI_MAX_PATH, acFilePath);
        NiStrcat(acFileName2, NI_MAX_PATH, wfd.cFileName);

        NiPath::Standardize(acFileName2);

        while (!bDone)
        {
            NiStrcpy(acFileName2, NI_MAX_PATH, acFilePath);
            NiStrcat(acFileName2, NI_MAX_PATH, wfd.cFileName);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                GetLastError();
                NILOG(NIMESSAGE_GENERAL_0,
                    "Invalid handle on FindXXXXXFile\n");
                bDone = true;
            }
            else
            {
                dwAttrib = GetFileAttributes(acFileName2);
                if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
                {
                    if (strcmp(wfd.cFileName, "."))
                    {
                        if (strcmp(wfd.cFileName, ".."))
                        {
                            // If we are recursing... do it
                            if (bRecurseDirectories)
                            {
                                NiStrcat(acFileName2, NI_MAX_PATH, "\\");
                                NILOG(NIMESSAGE_GENERAL_0,
                                    "    Recurse directory %s\n",
                                    acFileName2);
                                uiCount += LoadAllNSBFilesInDirectory(
                                    acFileName2, pcExt, bRecurseDirectories,
                                    pkFileList);
                            }
                        }
                    }
                }
                else
                {
                    if (ProcessNSBFile(acFileName2, pcExt, pkFileList))
                        uiCount++;
                }
            }

            if (FindNextFile(hFile, &wfd) == false)
                bDone = true;
        }

        FindClose(hFile);
    }

    return uiCount;
}

//--------------------------------------------------------------------------------------------------
