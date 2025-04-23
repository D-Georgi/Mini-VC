# MiniVC

A lightweight version control plugin for Notepad++, allowing you to track and browse revisions of individual text files directly within the editor.

---

## Author

David Georgi

---

## Features

- Initialize and manage a local repository for individual text files
- Commit current file with a custom message
- Browse and open previous versions of a file

---

## Prerequisites

- **Operating System:** Windows (bundled Notepad++ included in this repo)
- **Development (only for building from source):** Visual Studio

---

## Installing the Plugin

1. Download or clone this repository.
2. Open the `MiniVC` folder.
3. Open the included `Notepad++` directory and run `notepad++.exe`.
4. Navigate to the **Plugins** tab and locate **MiniVC** in the menu.
5. Under **MiniVC**, select **Set Repo Location** and follow the prompts to choose (or create) a folder for your local repository. 

   > **Tip:** A `Sample_Repo` folder is provided in this repo. If you’d like to explore without creating a new repository, select `Sample_Repo` as your repo location.

## Using the Plugin

1. Open or create a `.txt` document in Notepad++.
2. After making changes, go to **Plugins > MiniVC > Commit Current File**.
3. Enter a commit message when prompted and confirm to save the revision.
4. To view past commits, navigate to **Plugins > MiniVC > Open Versioned File**.
   - Selecting an older commit opens a popup to browse its contents.
   - Selecting the most recent commit opens it directly in Notepad++.

---

## Building from Source

If you wish to modify or rebuild the plugin yourself:

1. Ensure you have Windows and Visual Studio installed, do NOT launch Notepad++ yet.
2. Download or clone this repository.
3. Open the `MiniVC` folder.
4. Open the `vs.proj` subfolder and load the `MiniVC.sln` solution in Visual Studio.
5. In Visual Studio, go to **Build > Build Solution**.
6. After a successful build, open the `bin64` folder inside `MiniVC`.
7. In a separate File Explorer window, navigate to `MiniVC/Notepad++/plugins/MiniVC` and clear its contents (do **not** delete the folder).
8. Copy the newly built `MiniVC.dll` from `bin64` into the `MiniVC/Notepad++/plugins/MiniVC` folder.
9. Launch Notepad++ and follow the steps in **Installing the Plugin** to begin using your custom build.

---

## Challenges Faced

Text editors are a challenge to make, Notepad++ plugins are a slightly less challenging thing to make

---

## Important Files

There are a lot of files in this project. The files I worked on are found in 'MiniVC/src' directory. Descriptions of each file are:

1. `DockingFeature/resource.h`: A header file defining elements needed for popup windows used by plugin
2. `NppPluginDemo.rc`: A resource file that specifies the shapes, sizes, and layouts of the popup windows used
3. `PluginDefinition.h`: A header file that defines the 3 main buttons available in the MiniVC plugin tab of Notepad++
4. `PluginDefinition.cpp`: A C++ file that has all the implementation of the plugin's functionality and window management. This file utilizes the commitTree datastructure to handle all of the version control logic
5. `CommitTree.h`: A header file that implements the CommitTree, a partially persistent AVL tree data structure. I chose to use this as the datastructure as it will allow for the branching in the future with relative ease

