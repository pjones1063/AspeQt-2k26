import os
import re

def fix_qt6_deprecations(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        # Skip files that aren't standard text
        return False

    original_content = content

    # 1. Fix .count() -> .size()
    # Safely replaces the exact method call
    content = content.replace('.count()', '.size()')

    # 2. Fix QDropEvent / QDragMoveEvent pos() -> position().toPoint()
    content = content.replace('event->pos()', 'event->position().toPoint()')

    # 3. Fix QMessageBox flags -> StandardButton enum
    # Catches QMessageBox::Ok, QMessageBox::Yes, etc., and upgrades them
    content = re.sub(
        r'QMessageBox::(Ok|Yes|No|Cancel|Save|Discard|Abort|Retry|Ignore)',
        r'QMessageBox::StandardButton::\1', 
        content
    )

    # 4. Fix QTranslator::load [[nodiscard]] warning
    # Looks for any variable name ending in 'translator' calling .load( 
    # and safely prepends it with a (void) cast to silence the warning.
    # The negative lookbehind (?<!\(void\)) ensures it doesn't double-cast.
    content = re.sub(
        r'(?<!\(void\))(\s*)(\b\w*translator\.load\()', 
        r'\1(void)\2', 
        content
    )

    # If changes were made, write them back to the file
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Fixed deprecations in: {filepath}")
        return True
        
    return False

def main():
    # Set the target directory to your source folder
    target_dir = os.path.join(os.getcwd(), 'src')
    
    if not os.path.exists(target_dir):
        print(f"Error: Could not find directory {target_dir}")
        print("Please run this script from your main project folder.")
        return

    print("Scanning for Qt 6 deprecations...")
    files_modified = 0
    
    for root, dirs, files in os.walk(target_dir):
        for file in files:
            # Only process C++ source and header files
            if file.endswith(('.cpp', '.h', '.c', '.cc')):
                filepath = os.path.join(root, file)
                if fix_qt6_deprecations(filepath):
                    files_modified += 1

    print(f"\nDone! Modified {files_modified} files.")
    print("You can now run 'cmake --build' to verify the clean output.")

if __name__ == "__main__":
    main()
