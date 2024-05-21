import requests

from google.cloud import firestore
from datetime import datetime
from flask import Flask, jsonify, request
from bs4 import BeautifulSoup

app = Flask(__name__)

cred_path = 'acc.json'
db = firestore.Client.from_service_account_json(cred_path)

smhiURL = 'https://www.smhi.se/vader/prognoser/ortsprognoser/q/Stockholm/2673730'

def fetchWeather():
    current_hour = datetime.now().hour
    response = requests.get(smhiURL)

    soup = BeautifulSoup(response.content, 'html.parser')
    ths = soup.find_all('th', attrs={'scope': 'row'})

    today_th = []

    for th in ths:
        for span in th.find_all('span'):
            if span.text.strip() == current_hour:
                today_th = th
                break
        else:
            continue
        break

    parent = today_th.parent
    weather_icon = parent.find('img', class_="_weatherIcon_sq1e0_1 _smallIcon_q6vox_58")
    weather = weather_icon.get('alt')

    if 'moln' in weather:
        return True
    
    return False


@app.route('/save', methods=["POST"])
def save():
    if request.is_json:
        data = request.json
        data['date'] = datetime.now().strftime("%Y-%m-%d")
        data["time"] = datetime.now().strftime("%H:%M:%S")
        data['weather'] = fetchWeather()
        doc_ref = db.collection('data').add(data)
        print(f'Document added with ID: {doc_ref[1].id}')

        return 'OK'
    else:
        print("keep alive")
        return "keep alive"


    
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)