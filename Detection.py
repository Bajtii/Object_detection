
import os
import time
import cv2
import numpy as np
import urllib.request
import urllib.parse

# === KONFIG ===
ESP_IP = '10.165.105.199'                       # <-- wstaw IP Twojego ESP32-CAM
CAM_URL = f'http://{ESP_IP}/cam-hi.jpg'         # /cam-mid.jpg będzie szybsze
NOTIFY_URL = f'http://{ESP_IP}/notify?msg='

whT = 320
confThreshold = 0.5
nmsThreshold = 0.3

# Wysyłaj WSZYSTKIE klasy (None), albo podaj zestaw interesujących:
INTERESTING_CLASSES = None
# np.: INTERESTING_CLASSES = {'person', 'laptop', 'dog', 'cat', 'bird'}

MIN_INTERVAL_S = 0.7      # min. odstęp między wysyłkami
LAST_SENT_TS = 0.0
LAST_SENT_PAYLOAD = None   # debounce po treści

# === ŚCIEŻKI PLIKÓW względem folderu skryptu ===
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
modelConfig  = os.path.join(BASE_DIR, 'yolov3.cfg')
modelWeights = os.path.join(BASE_DIR, 'yolov3.weights')
classesFile  = os.path.join(BASE_DIR, 'cocov2.names')  # dopasowane do wag COCO

for p in [modelConfig, modelWeights, classesFile]:
    if not os.path.isfile(p):
        raise FileNotFoundError(f'Brak pliku: {p}')

# === Klasy COCO ===
with open(classesFile, 'rt', encoding='utf-8') as f:
    classNames = [line.strip() for line in f if line.strip()]

# === YOLO ===
net = cv2.dnn.readNetFromDarknet(modelConfig, modelWeights)
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

def send_notify(msg: str):
    """Wyślij do ESP32-CAM /notify z rate-limit i debounce."""
    global LAST_SENT_TS, LAST_SENT_PAYLOAD
    now = time.time()

    # Debounce: nie wysyłaj identycznej treści
    if msg == LAST_SENT_PAYLOAD:
        return
    # Rate limit czasowy
    if now - LAST_SENT_TS < MIN_INTERVAL_S:
        return

    try:
        encoded = urllib.parse.quote(msg, safe='')
        url = NOTIFY_URL + encoded
        print(f"[PY->ESP32 /notify] {url}")
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=3) as resp:
            _ = resp.read()
        LAST_SENT_TS = now
        LAST_SENT_PAYLOAD = msg
    except Exception as e:
        print(f"[NOTIFY ERROR] {e}")

def build_message(detected_list):
    """
    detected_list: [(name, conf_percent), ...] po NMS.
    Zwraca np. 'detected: person(95%), laptop(88%)' lub None, jeśli nic nie przeszło filtra.
    """
    if INTERESTING_CLASSES is not None:
        filtered = [(n, c) for (n, c) in detected_list if n in INTERESTING_CLASSES]
    else:
        filtered = detected_list

    if not filtered:
        return None

    # (opcjonalnie) top-k po pewności:
    # filtered = sorted(filtered, key=lambda x: x[1], reverse=True)[:3]

    parts = [f"{n}({c}%)" for (n, c) in filtered]
    return "detected: " + ", ".join(parts)

def findObjects(outputs, img):
    hT, wT = img.shape[:2]
    bbox, classIds, confs = [], [], []

    for output in outputs:
        for det in output:
            scores = det[5:]
            classId = int(np.argmax(scores))
            confidence = float(scores[classId])
            if confidence > confThreshold:
                w = int(det[2] * wT)
                h = int(det[3] * hT)
                x = int(det[0] * wT - w / 2)
                y = int(det[1] * hT - h / 2)
                bbox.append([x, y, w, h])
                classIds.append(classId)
                confs.append(confidence)

    indices = cv2.dnn.NMSBoxes(bbox, confs, confThreshold, nmsThreshold)

    detected_list = []
    if len(indices) > 0:
        for i in np.array(indices).flatten():
            x, y, w, h = bbox[i]
            name = classNames[classIds[i]] if classIds[i] < len(classNames) else str(classIds[i])
            conf_pct = int(confs[i] * 100)

            # rysowanie
            cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 255), 2)
            cv2.putText(img, f'{name.upper()} {conf_pct}%', (x, y - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)

            detected_list.append((name, conf_pct))

    msg = build_message(detected_list)
    if msg:
        send_notify(msg)

# === PĘTLA GŁÓWNA ===
while True:
    try:
        req = urllib.request.Request(CAM_URL, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=5) as img_resp:
            imgnp = np.frombuffer(img_resp.read(), dtype=np.uint8)
        img = cv2.imdecode(imgnp, cv2.IMREAD_COLOR)
        if img is None:
            print("Błąd: nie udało się zdekodować obrazu (img None).")
            continue

        blob = cv2.dnn.blobFromImage(img, 1/255.0, (whT, whT), (0, 0, 0), swapRB=True, crop=False)
        net.setInput(blob)
        layerNames = net.getLayerNames()
        outLayers = net.getUnconnectedOutLayers()
        outputNames = [layerNames[i - 1] for i in outLayers.flatten()]
        outputs = net.forward(outputNames)

        findObjects(outputs, img)

        cv2.imshow('Camera', img)
        if cv2.waitKey(1) & 0xFF == 27:  # ESC
            break

    except Exception as e:
        print("Błąd podczas pobierania/obrabiania obrazu:", e)

cv
