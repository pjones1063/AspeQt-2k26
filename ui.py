import os
import xml.etree.ElementTree as ET

# Configuration
TARGET_DIR = "./src"  # Adjust to where your .ui files are
# Elements to strip strict sizing from
TAGS_TO_CLEAN = ['minimumSize', 'maximumSize'] 

def clean_ui_file(filepath):
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
        modified = False

        # Find all widgets
        for widget in root.iter('widget'):
            # You can filter by class if you only want to fix specific items
            # if widget.get('class') in ['QPushButton', 'QLabel']: 
            
            for prop in widget.findall('property'):
                if prop.get('name') in TAGS_TO_CLEAN:
                    # Remove the property
                    widget.remove(prop)
                    modified = True
                    print(f"  - Removed {prop.get('name')} from {widget.get('class')}")

        if modified:
            tree.write(filepath, encoding='UTF-8', xml_declaration=True)
            print(f"Fixed: {filepath}")
        else:
            print(f"Skipped (Clean): {filepath}")

    except Exception as e:
        print(f"Error processing {filepath}: {e}")

# Main loop
print(f"Scanning {TARGET_DIR} for .ui files...")
for root, dirs, files in os.walk(TARGET_DIR):
    for file in files:
        if file.endswith(".ui"):
            clean_ui_file(os.path.join(root, file))

