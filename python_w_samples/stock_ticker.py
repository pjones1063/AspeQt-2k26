from flask import Flask, make_response
import socket
import yfinance as yf

app = Flask(__name__)

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

print(f"--- ATARI STOCK GATEWAY ---")
print(f"Running on: http://{get_ip()}:5000")
print(f"---------------------------")

@app.route('/stock/<ticker>', methods=['GET'])
def get_stock(ticker):
    print(f"[GATEWAY] Fetching ticker: {ticker}")
    
    try:
        # 1. Fetch Data
        stock = yf.Ticker(ticker)
        data = stock.history(period='1d')
        
        if data.empty:
            return "ERR: SYMBOL NOT FOUND\n"
            
        # Get latest price and calculate change
        current_price = data['Close'].iloc[-1]
        open_price = data['Open'].iloc[-1]
        change = current_price - open_price
        pct_change = (change / open_price) * 100
        
        # 2. Format the string
        # We use explicit symbols (+) instead of colors or arrows
        sign = "+" if change >= 0 else ""
        output = f"{ticker.upper()}: ${current_price:.2f} ({sign}{pct_change:.1f}%)"
        
        # 3. SANITIZE for Atari
        # Ensure pure ASCII (strips potential weird currency symbols)
        clean_text = output.encode('ascii', 'ignore').decode('ascii')
        
        print(f"[GATEWAY] Sending: {clean_text}")
        
        # 4. Return with newline for BASIC INPUT statement
        response = make_response(clean_text + "\n")
        response.headers['Content-Type'] = 'text/plain'
        return response

    except Exception as e:
        print(f"Error: {e}")
        return "ERR: API TIMEOUT\n"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)