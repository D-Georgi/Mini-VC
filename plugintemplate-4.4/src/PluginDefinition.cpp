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
#include "CommitTree.h"
#include <commctrl.h>
#include <stdexcept>

// GLOBALS

HINSTANCE g_hInst = NULL;
std::wstring g_repoPath = L"F:\\CSI5610\\Repo";
std::shared_ptr<CommitNode> g_commitTree = nullptr;
int g_commitCounter = 1;

struct TimelineData {
    std::vector<std::pair<int, std::wstring>> commits; // commit number and file name
    std::wstring folderPath; // repo folder
};

// Function Declerations
void InitializeCommitTree(const std::wstring& repoFolder);


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
    InitializeCommitTree(g_repoPath);
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
    setCommand(2, TEXT("Commit Current File"), commitCurrentFile, NULL, false);
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

INT_PTR CALLBACK TimelineDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static TimelineData* pData = nullptr;
    switch (message)
    {
    case WM_INITDIALOG:
    {
        pData = reinterpret_cast<TimelineData*>(lParam);
        HWND hList = GetDlgItem(hDlg, IDC_FILE_LIST);
        // Set extended style for full row selection.
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

        // Insert columns: column 0: "Time", column 1: "Filename"
        LVCOLUMN lvCol = { 0 };
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvCol.pszText = const_cast<LPWSTR>(L"Time");
        lvCol.cx = 50;
        ListView_InsertColumn(hList, 0, &lvCol);

        lvCol.pszText = const_cast<LPWSTR>(L"Filename");
        lvCol.cx = 150;
        ListView_InsertColumn(hList, 1, &lvCol);

        // Populate the list view with commit data
        LVITEM lvItem = { 0 };
        for (size_t i = 0; i < pData->commits.size(); i++)
        {
            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = (int)i;
            // Column 0: commit number
            std::wstring commitStr = std::to_wstring(pData->commits[i].first);
            lvItem.pszText = const_cast<LPWSTR>(commitStr.c_str());
            ListView_InsertItem(hList, &lvItem);
            // Column 1: filename
            ListView_SetItemText(hList, (int)i, 1, const_cast<LPWSTR>(pData->commits[i].second.c_str()));
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND hList = GetDlgItem(hDlg, IDC_FILE_LIST);
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel != -1)
            {
                // Get the commit filename from the data vector.
                auto commitPair = pData->commits[sel];
                std::wstring commitFileName = commitPair.second;
                std::wstring fullPath = pData->folderPath + L"\\" + commitFileName;

                // Read the file and load it into Notepad++
                std::string fileContents = ReadFileAsString(fullPath);
                int which = -1;
                ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
                if (which != -1)
                {
                    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
                    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)fileContents.c_str());
                }
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


void openVersionedFile()
{
    // If no commits exist, notify the user.
    if (!g_commitTree)
    {
        ::MessageBox(NULL, TEXT("No commits available."), TEXT("Info"), MB_OK);
        return;
    }

    // Build a vector of commit pairs (commit number and filename) by in-order traversal.
    std::vector<std::pair<int, std::wstring>> commitList;
    InOrderTraversal(g_commitTree, commitList);
    if (commitList.empty())
    {
        ::MessageBox(NULL, TEXT("No commits available."), TEXT("Info"), MB_OK);
        return;
    }

    // Prepare timeline data to pass to the dialog.
    TimelineData timelineData;
    timelineData.commits = commitList;
    timelineData.folderPath = g_repoPath;

    // Display the dialog using the plugin's instance handle (g_hInst) for resources.
    // Note: The dialog resource must now be updated to include a SysListView32 control.
    DialogBoxParam(
        g_hInst,
        MAKEINTRESOURCE(IDD_FILE_LIST_DLG),
        nppData._nppHandle,
        TimelineDlgProc,
        (LPARAM)&timelineData);
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

void commitCurrentFile() {
    // Get the current Scintilla editor handle
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
    {
        ::MessageBox(NULL, TEXT("No active document found."), TEXT("Commit Error"), MB_OK);
        return;
    }
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    // Retrieve the current document text.
    // First, get the text length:
    int textLength = (int)::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    // Allocate a buffer (SCI_GETTEXT works with char*; Notepad++ uses ANSI encoding here).
    char* textBuffer = new char[textLength + 1];
    ::SendMessage(curScintilla, SCI_GETTEXT, textLength + 1, (LPARAM)textBuffer);

    // Construct a custom commit file name using the commit counter.
    std::wstring commitFileName = L"commit_" + std::to_wstring(g_commitCounter) + L".txt";
    std::wstring fullPath = g_repoPath + L"\\" + commitFileName;

    // Write the file contents to the new commit file.
    FILE* fp = _wfopen(fullPath.c_str(), L"wb");
    if (!fp)
    {
        ::MessageBox(NULL, TEXT("Error writing commit file."), TEXT("Commit Error"), MB_OK);
        delete[] textBuffer;
        return;
    }
    // Write the text content. Note: writing textBuffer as binary.
    fwrite(textBuffer, sizeof(char), textLength, fp);
    fclose(fp);
    delete[] textBuffer;

    // Insert the new commit into the persistent AVL tree.
    g_commitTree = insertNode(g_commitTree, g_commitCounter, commitFileName);
    g_commitCounter++;

    // Notify the user of success.
    std::wstring msg = L"File committed as " + commitFileName;
    ::MessageBox(NULL, msg.c_str(), L"Commit Successful", MB_OK);
}

void InitializeCommitTree(const std::wstring& repoFolder)
{
    // Get all files from the repo folder.
    std::vector<std::wstring> files = GetTextFiles(repoFolder);
    int maxCommit = 0;

    // Iterate through each file.
    for (const auto& file : files)
    {
        // Check if file name matches the pattern "commit_<number>.txt"
        std::wstring prefix = L"commit_";
        std::wstring suffix = L".txt";
        if (file.compare(0, prefix.size(), prefix) == 0 &&
            file.size() > prefix.size() + suffix.size() &&
            file.compare(file.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            // Extract the number between the prefix and suffix.
            std::wstring numStr = file.substr(prefix.size(), file.size() - prefix.size() - suffix.size());
            try
            {
                int commitNum = _wtoi(numStr.c_str());
                // Insert into the persistent AVL tree.
                g_commitTree = insertNode(g_commitTree, commitNum, file);
                if (commitNum > maxCommit)
                    maxCommit = commitNum;
            }
            catch (const std::invalid_argument&)
            {
                // If conversion fails, skip this file.
                continue;
            }
        }
    }
    // Set the global commit counter to one more than the highest commit number.
    g_commitCounter = maxCommit + 1;
}