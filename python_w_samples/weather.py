import requests
from flask import Flask, make_response

app = Flask(__name__)

# Lincoln, Ontario Coordinates
LAT = 43.1594
LON = -79.4678

@app.route('/weather', methods=['GET'])
def get_weather():
    try:
        # Open-Meteo Free API (JSON)
        url = f"https://api.open-meteo.com/v1/forecast?latitude={LAT}&longitude={LON}&current_weather=true"
        print(f"Fetching Open-Meteo for Lat: {LAT}, Lon: {LON}...")
        
        # Headers to be polite
        headers = {'User-Agent': 'Atari800XL-WeatherStation/1.0'}
        
        r = requests.get(url, headers=headers, timeout=5)
        data = r.json()
        
        # Extract Data
        current = data.get('current_weather', {})
        temp_val = current.get('temperature')
        wmo_code = current.get('weathercode')
        
        # Format Temp string
        temp_str = f"{temp_val}C"
        
        # --- MAP WMO CODES TO ATARI ICONS ---
        # WMO Codes: https://open-meteo.com/en/docs
        
        icon_id = "1"     # Default Sun
        condition_text = "CLEAR"

        # 0 = Clear, 1-3 = Cloudy
        if wmo_code <= 1:
            icon_id = "1"
            condition_text = "SUNNY"
        elif wmo_code <= 3:
            icon_id = "2"
            condition_text = "CLOUDY"
        
        # 45, 48 = Fog
        elif wmo_code in [45, 48]:
            icon_id = "2"
            condition_text = "FOGGY"

        # 51-67, 80-82 = Rain/Drizzle
        elif 50 <= wmo_code <= 67 or 80 <= wmo_code <= 82:
            icon_id = "3"
            condition_text = "RAIN"

        # 71-77, 85-86 = Snow
        elif 71 <= wmo_code <= 77 or 85 <= wmo_code <= 86:
            icon_id = "4"
            condition_text = "SNOW"
            
        # 95-99 = Thunderstorm (Map to Rain for now)
        elif wmo_code >= 95:
            icon_id = "3"
            condition_text = "STORM"
            
        print(f"Result: {condition_text}, {temp_str} (Icon {icon_id})")

        # --- SEND TO ATARI ---
        # Line 1: Icon ID
        # Line 2: Temp
        # Line 3: Text
        output = [icon_id, temp_str, condition_text]
        full_text = chr(155).join(output) + chr(155)
        
        response = make_response(full_text.encode('latin-1', errors='replace'))
        return response

    except Exception as e:
        print(f"Error: {e}")
        # Fallback Data so Atari doesn't crash
        # Icon 2 (Cloud) + Error Message
        err_out = f"2{chr(155)}ERR{chr(155)}OFFLINE{chr(155)}"
        return make_response(err_out.encode('latin-1'))

if __name__ == '__main__':
    print(f"--- ATARI WEATHER (OPEN-METEO) RUNNING ---")
    app.run(host='0.0.0.0', port=5000)
    
