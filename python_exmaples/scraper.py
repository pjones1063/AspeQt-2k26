import requests
from bs4 import BeautifulSoup
from flask import Flask, make_response

app = Flask(__name__)

# The Correct URL
TNFS_URL = "https://13leader.net/tnfs_status.html"

@app.route('/tnfs', methods=['GET'])
def get_tnfs_status():
    try:
        print(f"Fetching {TNFS_URL}...")
        r = requests.get(TNFS_URL, timeout=10)
        
        # Parse HTML
        soup = BeautifulSoup(r.text, 'html.parser')
        
        # --- ATARI OUTPUT BUFFER ---
        output = []
        
        # 1. Header Line (40 Chars max)
        # "HOST..........................STATUS"
        header = "{:<28} {:>11}".format("HOST", "STATUS")
        output.append(header)
        output.append("-" * 40)

        # 2. Find the Table
        # The page usually has one main table for stats
        table = soup.find('table')
        
        if table:
            # Skip the header row (th), just get data rows (tr)
            rows = table.find_all('tr')
            
            for row in rows:
                cols = row.find_all('td')
                if len(cols) >= 2:
                    # Extract text
                    host = cols[0].text.strip().upper() # Uppercase looks better on Atari
                    status = cols[1].text.strip().upper()
                    
                    # Clean up: Remove 'http://' or 'tnfs://' if present to save space
                    host = host.replace("TNFS://", "").replace("HTTP://", "")

                    # --- 40-COLUMN FORMATTING ---
                    # Host: Max 27 chars
                    # Status: Max 10 chars (Right Aligned)
                    # "tnfs.fujinet.pl             UP"
                    line = "{:<28} {:>11}".format(host[:28], status[:11])
                    output.append(line)
        else:
            output.append("ERROR: NO TABLE FOUND.")

        # 3. Add Footer
        output.append("-" * 40)
        output.append("READY.")

        # 4. Convert to ATASCII EOL (155 / 0x9B)
        #    Join with chr(155)
        full_text = chr(155).join(output) + chr(155)

        # 5. Serve as Latin-1 (Raw Bytes)
        response = make_response(full_text.encode('latin-1', errors='replace'))
        response.headers['Content-Type'] = 'text/plain'
        
        print("--- Sent TNFS Status to Atari ---")
        return response

    except Exception as e:
        print(f"Error: {e}")
        err_msg = f"ERROR: {str(e)[:30]}" + chr(155)
        return make_response(err_msg.encode('latin-1'))

if __name__ == '__main__':
    print("--- TNFS SCRAPER STARTED ---")
    app.run(host='0.0.0.0', port=5000)
    
