# 📷 ESP32-CAM Object Detection & Real-Time Notification System  
### Embedded Vision & Python AI Project

---

## 📌 Project Description

This project focuses on **real-time object detection** using an **ESP32-CAM camera module** combined with a **Python-based YOLO detection pipeline**.

The main goal of the project was to create a **complete vision system** that:

- Streams images from an ESP32-CAM module  
- Performs object detection using **YOLO neural network** on a PC  
- Sends detection results back to ESP32-CAM  
- Publishes notifications using **MQTT**  

The system integrates **embedded programming (Arduino C++)**, **computer vision**, and **network communication**, forming a distributed IoT-based vision solution.

---

## 🎯 Project Goal

The primary objective was to build software for the **ESP32-CAM module** that enables:

- Remote image acquisition  
- Communication with an external detection algorithm  
- Bidirectional message exchange  

The project uses:

- **Arduino C++** → camera control, web server, MQTT  
- **Python** → image processing, YOLO inference  

Pre-trained **YOLO models and libraries** were used to accelerate development and focus on system integration.

---

## 🔁 Project Workflow

The system operates in the following stages:

---

### 1️⃣ ESP32-CAM Initialization & Wi-Fi Connection

The ESP32-CAM:

- Initializes camera hardware  
- Connects to local Wi-Fi network  
- Starts an HTTP server  
- Connects to MQTT broker  

<img width="1024" height="1536" alt="ChatGPT Image Feb 2, 2026, 08_27_19 AM" src="https://github.com/user-attachments/assets/f4189665-26bf-446e-8438-0a9f3c24b773" />


---

### 2️⃣ Image Streaming from ESP32-CAM

ESP32-CAM exposes endpoints:

- `/cam-lo.jpg`  
- `/cam-mid.jpg`  
- `/cam-hi.jpg`  

The Python application periodically downloads frames using HTTP.

![Camera Stream](ADD_IMAGE_LINK_HERE)

---

### 3️⃣ Python Image Acquisition & Preprocessing

Python script:

- Downloads JPEG frame  
- Decodes image with OpenCV  
- Converts image to YOLO input blob  

---

### 4️⃣ YOLO Object Detection

Using OpenCV DNN and YOLO:

- Forward pass through neural network  
- Confidence threshold filtering  
- Non-Maximum Suppression (NMS)  
- Bounding box generation  

---

### 5️⃣ Visualization on PC

Detected objects are drawn on live video:

- Bounding boxes  
- Class names  
- Confidence percentages  
<img width="967" height="802" alt="Screenshot 2025-12-01 170834" src="https://github.com/user-attachments/assets/a7a6d7f5-c72e-4d9f-a7d9-3508cd51e58f" />
<img width="1195" height="945" alt="Screenshot 2025-12-01 174259" src="https://github.com/user-attachments/assets/3fdf4740-3816-4e24-9a83-341cc2b8c460" />



---

### 6️⃣ Detection Message Generation

Detected objects are formatted into messages:

detected: person(95%), laptop(88%)



Filtering options:

- Send all classes  
- Or only selected classes  

---

### 7️⃣ Notification Back to ESP32-CAM

Python sends message to:

http://ESP_IP/notify?msg=...


ESP32-CAM:

- Displays message on Serial Monitor  
- Publishes message via MQTT  

![Notification Flow](ADD_IMAGE_LINK_HERE)

---

### 8️⃣ MQTT Publishing

ESP32-CAM publishes detection results to topic:

esp32cam/notify


Other devices (PC, Raspberry Pi, Home Assistant, Node-RED) can subscribe.

![MQTT Flow](ADD_IMAGE_LINK_HERE)

---

The project forms a **closed feedback loop**:

ESP32-CAM → Python YOLO → ESP32-CAM → MQTT → Subscribers

---

## ⚙️ Technologies and Tools

- ESP32-CAM  
- Arduino C++  
- Python 3.9+  
- OpenCV  
- YOLOv3  
- NumPy  
- MQTT (PubSubClient)  
- HTTP Server  

---

## 🧠 Core Concepts Implemented

- Embedded web server  
- Camera streaming  
- Computer vision pipeline  
- Deep neural networks  
- HTTP communication  
- MQTT publish/subscribe  
- Rate limiting & debounce  
- Modular architecture  

---

## 🧩 Features

- Live image streaming  
- Real-time object detection  
- Bounding box visualization  
- Confidence threshold configuration  
- Selectable object classes  
- Automatic notifications  
- MQTT integration  
- Works over Wi-Fi  

---

## 📂 Project Structure
ESP32_Object_Detection/
│
├── esp32_firmware/
│ └── esp32cam_server.ino
│
├── python_detection/
│ ├── main.py
│ ├── yolov3.cfg
│ ├── yolov3.weights
│ └── cocov2.names
│
└── README.md



---

## 🔌 ESP32 Firmware Responsibilities

- Initialize camera  
- Manage Wi-Fi  
- Serve JPEG images  
- Receive detection messages  
- Publish MQTT messages  

---

## 🖥️ Python Application Responsibilities

- Fetch camera images  
- Run YOLO inference  
- Draw bounding boxes  
- Create detection messages  
- Send notifications  

---

## 🚀 Possible Future Improvements

- MJPEG streaming  
- GPU acceleration  
- Custom trained model  
- Web dashboard  
- Image saving  
- Alarm / buzzer support  

---
