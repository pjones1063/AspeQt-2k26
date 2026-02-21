from flask import Flask, make_response
import feedparser
import textwrap  # <--- This library handles the wrapping

app = Flask(__name__)

# RSS Feeds configuration
FEEDS = {
    'TOP': 'http://feeds.bbci.co.uk/news/rss.xml',
    'TECH': 'http://feeds.bbci.co.uk/news/technology/rss.xml',
    'SPACE': 'https://www.nasa.gov/rss/dyn/breaking_news.rss',
    'PY': 'https://realpython.com/atom.xml'
}

@app.route('/news/<category>', methods=['GET'])
def get_news(category):
    cat = category.upper()
    if cat not in FEEDS:
        return "ERR: UNKNOWN CATEGORY\n"
        
    print(f"[GATEWAY] Fetching News: {cat}")
    
    try:
        # 1. Parse the Feed
        feed = feedparser.parse(FEEDS[cat])
        output_lines = []
        
        # 2. Loop through top 5 articles
        for i, entry in enumerate(feed.entries[:5]):
            
            # --- PREPARE TITLE ---
            raw_title = f"{i+1}. {entry.title}"
            
            # Sanitize (Remove smart quotes, etc)
            clean_title = raw_title.replace("‘", "'").replace("’", "'")
            clean_title = clean_title.encode('ascii', 'ignore').decode('ascii')
            
            # --- WORD WRAP (The Magic Part) ---
            # Wrap text to 38 chars so it fits on 40-column screen
            # "wrapped_list" will be a list of short strings
            wrapped_list = textwrap.wrap(clean_title, width=38)
            
            # Add wrapped lines to output
            for line in wrapped_list:
                output_lines.append(line.upper())
            
            # --- BLANK SEPARATOR ---
            # Add an empty string to create a blank line between articles
            output_lines.append("") 

        # 3. Join everything with newlines
        # The final result is one big string with many lines
        final_response = "\n".join(output_lines) + "\n"
        
        response = make_response(final_response)
        response.headers['Content-Type'] = 'text/plain'
        return response

    except Exception as e:
        return f"ERR: {str(e)}\n"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)