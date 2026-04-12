import os
import time
import random
import xml.etree.ElementTree as ET
from deep_translator import GoogleTranslator

def auto_translate_ts():
    # --- BULLETPROOF PATHING ---
    # 1. Get the absolute path of the folder this script is currently sitting in
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 2. Point to the 'i18n' folder located in that exact same directory
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

        for message in root.iter('message'):
            source = message.find('source')
            translation = message.find('translation')

            if source is not None and translation is not None:
                # Target ALL unfinished strings, ignoring which file they came from
                if translation.get('type') == 'unfinished' or not translation.text:
                    original_text = source.text
                    
                    if original_text:
                        try:
                            translated_text = translator.translate(original_text)
                            translation.text = translated_text
                            
                            if 'type' in translation.attrib:
                                del translation.attrib['type']
                                
                            print(f" -> '{original_text}' => '{translated_text}'")
                            modified = True
                            
                            # The Anti-Ban Delay: Random sleep between 2.0 and 5.0 seconds
                            delay = random.uniform(2.0, 5.0)
                            print(f"    [Sleeping for {delay:.2f}s...]")
                            time.sleep(delay)
                            
                            # Save after EVERY successful translation. 
                            # If the script crashes or you stop it, you don't lose hours of work!
                            tree_str = ET.tostring(root, encoding='utf-8', xml_declaration=True).decode('utf-8')
                            tree_str = tree_str.replace("<?xml version='1.0' encoding='utf-8'?>", "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!DOCTYPE TS>")
                            with open(filepath, 'w', encoding='utf-8') as f:
                                f.write(tree_str)

                        except Exception as e:
                            print(f" [!] Error translating '{original_text}': {e}")
                            print("    [Taking a 10-second penalty box sleep before retrying...]")
                            time.sleep(10)

        print(f"[{time.strftime('%H:%M:%S')}] Finished {filename}")

if __name__ == "__main__":
    auto_translate_ts()
