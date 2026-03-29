import os
import shutil

# Target files
HEADER_FILE = os.path.join("src", "mainwindow.h")
CPP_FILE = os.path.join("src", "mainwindow.cpp")

# We ONLY target signatures with parameters so we don't break the main menu () slots!
ACTIONS = [
    "MountDisk", "MountFolder", "Eject", "WriteProtect", 
    "EditDisk", "Save", "AutoSave", "SaveAs", "Revert", 
    "InspectSectors", "Info", "HappyMode", "MountTnfs"
]

def process_file(filepath, replacements):
    if not os.path.exists(filepath):
        print(f"[ERROR] Could not find {filepath}")
        return False
    
    # Create a backup just in case
    shutil.copy(filepath, filepath + ".bak")
    print(f"Created backup: {filepath}.bak")

    with open(filepath, 'r', encoding='utf-8') as file:
        content = file.read()

    original_content = content
    for old, new in replacements:
        content = content.replace(old, new)

    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as file:
            file.write(content)
        print(f"[SUCCESS] Updated {filepath}")
    else:
        print(f"[INFO] No changes needed in {filepath}")
    
    return True

def main():
    print("--- AspeQt Qt Slot Renamer ---")

    # 1. Build Header Replacements
    h_replacements = [
        ("void on_actionMountRecent_triggered(const QString", "void handle_actionMountRecent_triggered(const QString")
    ]
    for action in ACTIONS:
        h_replacements.append((f"void on_action{action}_triggered(int", f"void handle_action{action}_triggered(int"))

    # 2. Build CPP Replacements
    cpp_replacements = [
        # Implementations
        ("void MainWindow::on_actionMountRecent_triggered(const QString", "void MainWindow::handle_actionMountRecent_triggered(const QString"),
        # Slots inside connect()
        ("SLOT(on_actionMountRecent_triggered(QString", "SLOT(handle_actionMountRecent_triggered(QString")
    ]
    
    for action in ACTIONS:
        # Implementations
        cpp_replacements.append((f"void MainWindow::on_action{action}_triggered(int", f"void MainWindow::handle_action{action}_triggered(int"))
        # Slots inside connect() macros
        cpp_replacements.append((f"SLOT(on_action{action}_triggered(int", f"SLOT(handle_action{action}_triggered(int"))
        # Explicit Lambda calls (like MountTnfs)
        cpp_replacements.append((f"on_action{action}_triggered(i);", f"handle_action{action}_triggered(i);"))

    # 3. Run the replacements
    process_file(HEADER_FILE, h_replacements)
    process_file(CPP_FILE, cpp_replacements)

    print("--- Finished! ---")
    print("Please run 'Build -> Rebuild Project' in Qt Creator.")

if __name__ == "__main__":
    main()

