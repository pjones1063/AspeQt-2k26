import os
import time
import random
import re
import xml.etree.ElementTree as ET
from deep_translator import GoogleTranslator

########################################################
#
#  source .venv/bin/activate
#  /usr/lib/qt6/bin/lupdate src/ -ts res/i18n/*.ts
#  python3  res/translate.py
#
########################################################

# --- Hardcoded Language Names for the OptionsDialog ---
# This ensures that when the combo box looks for "English", it finds 
# the localized name of the target language (e.g., "Deutsch" instead of "Englisch" in German).
LANGUAGE_NAMES = {
    'de': 'Deutsch', 
    'es': 'Español', 
    'pl': 'Polski', 
    'ru': 'Русский', 
    'sk': 'Slovenčina', 
    'tr': 'Türkçe', 
    'fr': 'Français',
    'cs': 'Čeština',
    'it': 'Italiano'
}

# ---Detection function for paths and URLs ---
def is_path_or_url(text):
    if not text:
        return False
    
    # 1. Catch obvious prefixes
    if text.startswith(('qrc:/', 'http://', 'https://', 'file://', 'ftp://')):
        return True
        
    # 2. Catch anything that looks like a file path ending in common extensions
    # (e.g., "/images/icon.png" or "docs/manual.html")
    if re.search(r'\.(html|htm|xml|png|jpg|jpeg|gif|css|js|txt|ini)$', text, re.IGNORECASE) and '/' in text:
        return True
        
    return False

def auto_translate_ts():
    # --- BULLETPROOF PATHING ---
    script_dir = os.path.dirname(os.path.abspath(__file__))
    directory = os.path.join(script_dir, 'i18n') 
    
    lang_map = {
        'de': 'de', 'es': 'es', 'pl': 'pl', 
        'ru': 'ru', 'sk': 'sk', 'tr': 'tr', 
        'fr': 'fr','cs':'cs','it':'it'
    }

    for filename in os.listdir(directory):
        if not filename.endswith('.ts'):
            continue

        lang_code = filename.split('_')[-1].split('.')[0]
        if lang_code not in lang_map:
            continue

        filepath = os.path.join(directory, filename)
        tree = ET.parse(filepath)
        root = tree.getroot()
        modified = False

        print(f"\n[{time.strftime('%H:%M:%S')}] Processing {filename}...")
        translator = GoogleTranslator(source='en', target=lang_map[lang_code])

        # --- CONTEXT ITERATION ---
        # We need to iterate through contexts first to catch the OptionsDialog
        for context in root.findall('context'):
            context_name_elem = context.find('name')
            context_name = context_name_elem.text if context_name_elem is not None else ""

            for message in context.findall('message'):
                source = message.find('source')
                translation = message.find('translation')

                if source is not None and translation is not None:
                    original_text = source.text
                    
                    # --- NEW: OptionsDialog "English" Override ---
                    # Always force this specific translation, even if it's not marked 'unfinished', 
                    # to correct previously bad translations.
                    if context_name == "OptionsDialog" and original_text == "English":
                        target_name = LANGUAGE_NAMES.get(lang_code, "English")
                        if translation.text != target_name:
                            print(f" [!] Enforcing Language Name Override: '{original_text}' => '{target_name}'")
                            translation.text = target_name
                            
                            if 'type' in translation.attrib:
                                del translation.attrib['type'] # Mark as finished
                                
                            modified = True
                        continue # Skip to the next message, override handled

                    # Only process normal translations if they are unfinished or empty
                    if translation.get('type') == 'unfinished' or not translation.text:
                        if original_text:
                            # --- The Path/URL Bypass ---
                            if is_path_or_url(original_text):
                                print(f" [!] Bypassing path/URL: '{original_text}'")
                                translation.text = original_text # Copy exactly
                                
                                if 'type' in translation.attrib:
                                    del translation.attrib['type'] # Mark as finished
                                    
                                modified = True
                                continue # Skip the Google API and the sleep timer!
                                
                            # --- Standard Translation ---
                            try:
                                translated_text = translator.translate(original_text)
                                translation.text = translated_text
                                
                                if 'type' in translation.attrib:
                                    del translation.attrib['type']
                                    
                                print(f" -> '{original_text}' => '{translated_text}'")
                                modified = True
                                
                                delay = random.uniform(2.0, 5.0)
                                print(f"    [Sleeping for {delay:.2f}s...]")
                                time.sleep(delay)
                                
                                tree_str = ET.tostring(root, encoding='utf-8', xml_declaration=True).decode('utf-8')
                                tree_str = tree_str.replace("<?xml version='1.0' encoding='utf-8'?>", "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>")
                                with open(filepath, 'w', encoding='utf-8') as f:
                                    f.write(tree_str)

                            except Exception as e:
                                print(f" [!] Error translating '{original_text}': {e}")
                                print("    [Taking a 10-second penalty box sleep before retrying...]")
                                time.sleep(10)

        # Save any final bypass modifications that didn't trigger the API save block
        if modified:
            tree_str = ET.tostring(root, encoding='utf-8', xml_declaration=True).decode('utf-8')
            tree_str = tree_str.replace("<?xml version='1.0' encoding='utf-8'?>", "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>")
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(tree_str)

        print(f"[{time.strftime('%H:%M:%S')}] Finished {filename}")

if __name__ == "__main__":
    auto_translate_ts()
