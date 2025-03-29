//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "DockingFeature/resource.h"
#include <windows.h>
#include <vector>
#include <string>
#include <cstdio>
#include <sstream>
#include <shlobj.h>

// GLOBALS

HINSTANCE g_hInst = NULL;
std::wstring g_repoPath = L"F:\\CSI5610\\Repo";

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    g_hInst = reinterpret_cast<HINSTANCE>(hModule);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Open Versioned File"), openVersionedFile, NULL, false);
    setCommand(1, TEXT("Set Repo Location"), setRepoLocation, NULL, false);

}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
    // Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
std::vector<std::wstring> GetTextFiles(const std::wstring& folderPath)
{
    std::vector<std::wstring> files;
    WIN32_FIND_DATA findFileData;
    std::wstring searchPath = folderPath + L"\\*.txt";
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Skip directories
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                files.push_back(findFileData.cFileName);
            }
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }
    return files;
}

std::string ReadFileAsString(const std::wstring& filePath)
{
    FILE* fp = _wfopen(filePath.c_str(), L"rb");
    if (!fp)
        return "";

    std::ostringstream oss;
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        oss.write(buffer, bytesRead);
    }
    fclose(fp);
    return oss.str();
}

INT_PTR CALLBACK FileListDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static std::vector<std::wstring>* pFiles = nullptr;
    static std::wstring folderPath;  // store folder path if needed

    switch (message)
    {
    case WM_INITDIALOG:
    {
        // lParam contains pointer to a structure with file list and folder path
        auto* params = reinterpret_cast<std::pair<std::vector<std::wstring>*, std::wstring>*>(lParam);
        pFiles = params->first;
        folderPath = params->second;

        HWND hList = GetDlgItem(hDlg, IDC_FILE_LIST); // IDC_FILE_LIST: your list box control ID

        // Populate list box
        for (const auto& file : *pFiles)
        {
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)file.c_str());
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hList = GetDlgItem(hDlg, IDC_FILE_LIST);
            int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR)
            {
                wchar_t fileName[260];
                SendMessage(hList, LB_GETTEXT, sel, (LPARAM)fileName);

                // Build the full file path:
                std::wstring fullPath = folderPath + L"\\" + fileName;

                // Read the contents of the selected file:
                std::string fileContents = ReadFileAsString(fullPath);

                // Get the current Scintilla editor:
                int which = -1;
                ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
                if (which != -1)
                {
                    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

                    // Overwrite the current document with the file contents:
                    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)fileContents.c_str());
                }
            }
            EndDialog(hDlg, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
        }
        return TRUE;
    }

    return FALSE;
}

void openVersionedFile()
{
    // Use the user-defined repo location stored in g_repoPath
    std::wstring folderPath = g_repoPath;

    // Get the list of .txt files
    std::vector<std::wstring> files = GetTextFiles(folderPath);

    if (files.empty())
    {
        ::MessageBox(NULL, TEXT("No text files found in the folder."), TEXT("Info"), MB_OK);
        return;
    }

    // Prepare parameters for the dialog
    std::pair<std::vector<std::wstring>*, std::wstring> dlgParams(&files, folderPath);

    // Display the dialog using the plugin's instance handle (g_hInst) for resources
    DialogBoxParam(
        g_hInst,
        MAKEINTRESOURCE(IDD_FILE_LIST_DLG),
        nppData._nppHandle,
        FileListDlgProc,
        (LPARAM)&dlgParams);
}



std::wstring BrowseForFolder(HWND owner, const wchar_t* title)
{
    std::wstring folder;
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = owner;
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl)
    {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path))
        {
            folder = path;
        }
        CoTaskMemFree(pidl);
    }
    return folder;
}

void setRepoLocation()
{
    // Open the folder selection dialog with the plugin's main window as owner
    std::wstring chosenFolder = BrowseForFolder(nppData._nppHandle, L"Select Repository Folder");
    if (!chosenFolder.empty())
    {
        g_repoPath = chosenFolder;  // Save the new repo location

        // Optionally, notify the user:
        std::wstring msg = L"Repository location set to:\n" + chosenFolder;
        ::MessageBox(NULL, msg.c_str(), L"Repository Location", MB_OK);
    }
    else
    {
        ::MessageBox(NULL, L"No folder was selected.", L"Repository Location", MB_OK);
    }
}
