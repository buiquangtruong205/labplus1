import json
import threading
from datetime import datetime, timezone
from collections import deque

import psycopg2
import paho.mqtt.client as mqtt
from flask import Flask, render_template, jsonify, request

# ================== CONFIG ==================
MQTT_BROKER = "broker.emqx.io"
MQTT_PORT   = 1883

PG_CONN = psycopg2.connect(
    dbname="vacsin",
    user="postgres",
    password="123456",
    host="localhost",
    port=5433
)
PG_CONN.autocommit = True

# MQTT Topics
TOPIC_REALTIME = "vacin/telemetry"
TOPIC_BATCH    = "vacin/telemetry/batch"
TOPIC_CMD      = "vacin/cmd"
TOPIC_STAT     = "vacin/stat"

# ================== FLASK APP ==================
app = Flask(__name__)

# In-memory cache for real-time display (last 100 readings)
recent_data = deque(maxlen=100)
device_status = {}  # {device_id: {cooler: "ON/OFF", alarm: "ON/OFF", last_update: timestamp}}

# ================== DB HELPERS ==================
def ensure_device(cur, device_id):
    cur.execute("""
        INSERT INTO devices (device_id, location_name)
        VALUES (%s, 'Kho Vắc-xin Chính')
        ON CONFLICT (device_id) DO NOTHING
    """, (device_id,))


def insert_telemetry(cur, device_id, temperature, ts, status):
    # Dùng thời gian server thay vì ts từ device (millis not Unix time)
    measured_at = datetime.now(tz=timezone.utc)
    ensure_device(cur, device_id)
    cur.execute("""
        INSERT INTO telemetry (device_id, temperature, measured_at, status)
        VALUES (%s, %s, %s, %s)
    """, (device_id, temperature, measured_at, status))
    
    # Add to in-memory cache
    recent_data.append({
        "dev": device_id,
        "temp": temperature,
        "time": measured_at.isoformat(),
        "status": status
    })
    print(f"[DB] Inserted: {device_id} temp={temperature} at {measured_at}")


# ================== MQTT CALLBACKS ==================
mqtt_client = None

def on_connect(client, userdata, flags, reason_code, properties=None):
    print("✅ MQTT connected, reason_code =", reason_code)
    client.subscribe([
        (TOPIC_REALTIME, 0),
        (TOPIC_BATCH, 0),
        (TOPIC_STAT, 0)
    ])


def on_message(client, userdata, msg):
    payload = msg.payload.decode(errors="ignore")

    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        print("❌ JSON lỗi:", payload)
        return

    # ----- STAT (device status) -----
    if msg.topic == TOPIC_STAT:
        dev = data.get("dev", "unknown")
        device_status[dev] = {
            "cooler": data.get("cooler", "OFF"),
            "alarm": data.get("alarm", "OFF"),
            "last_update": datetime.now().isoformat()
        }
        print(f"📊 STAT {dev}: cooler={data.get('cooler')} alarm={data.get('alarm')}")
        return

    with PG_CONN.cursor() as cur:
        # ----- REALTIME -----
        if msg.topic == TOPIC_REALTIME:
            insert_telemetry(
                cur,
                device_id=data["dev"],
                temperature=data["t"],
                ts=data["ts"],
                status=data.get("st", 0)
            )
            print(f"📥 REALTIME {data['dev']}  T={data['t']}°C")

        # ----- BATCH -----
        elif msg.topic == TOPIC_BATCH:
            count = 0
            for item in data["data"]:
                insert_telemetry(
                    cur,
                    device_id=item["dev"],
                    temperature=item["t"],
                    ts=item["ts"],
                    status=item.get("st", 0)
                )
                count += 1
            print(f"📦 BATCH from {data.get('gw')}  count={count}")


# ================== FLASK ROUTES ==================
@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/data")
def api_data():
    """Lấy dữ liệu nhiệt độ gần đây"""
    print(f"[API] /api/data called - recent_data count: {len(recent_data)}, status: {device_status}")
    return jsonify({
        "data": list(recent_data),
        "status": device_status
    })


@app.route("/api/history")
def api_history():
    """Lấy lịch sử từ database"""
    limit = request.args.get("limit", 50, type=int)
    device = request.args.get("device", None)
    
    with PG_CONN.cursor() as cur:
        if device:
            cur.execute("""
                SELECT device_id, temperature, measured_at, status 
                FROM telemetry 
                WHERE device_id = %s
                ORDER BY measured_at DESC 
                LIMIT %s
            """, (device, limit))
        else:
            cur.execute("""
                SELECT device_id, temperature, measured_at, status 
                FROM telemetry 
                ORDER BY measured_at DESC 
                LIMIT %s
            """, (limit,))
        
        rows = cur.fetchall()
        result = [{
            "dev": r[0],
            "temp": r[1],
            "time": r[2].isoformat() if r[2] else None,
            "status": r[3]
        } for r in rows]
    
    return jsonify(result)


@app.route("/api/devices")
def api_devices():
    """Lấy danh sách thiết bị"""
    with PG_CONN.cursor() as cur:
        cur.execute("SELECT device_id, location_name FROM devices")
        rows = cur.fetchall()
        result = [{"id": r[0], "location": r[1]} for r in rows]
    return jsonify(result)


@app.route("/api/command", methods=["POST"])
def api_command():
    """Gửi lệnh điều khiển qua MQTT"""
    data = request.json
    cmd = data.get("cmd", "")
    
    if cmd not in ["TURN_ON_BACKUP_COOLER", "RESET_ALARM"]:
        return jsonify({"error": "Invalid command"}), 400
    
    if mqtt_client:
        mqtt_client.publish(TOPIC_CMD, cmd)
        print(f"📤 CMD sent: {cmd}")
        return jsonify({"success": True, "cmd": cmd})
    else:
        return jsonify({"error": "MQTT not connected"}), 500


# ================== MAIN ==================
def run_mqtt():
    global mqtt_client
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    print("🚀 MQTT Worker started...")
    mqtt_client.loop_forever()


def run_flask():
    print("🌐 Flask Web Server starting on http://localhost:5000")
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)


if __name__ == "__main__":
    # Start MQTT in separate thread
    mqtt_thread = threading.Thread(target=run_mqtt, daemon=True)
    mqtt_thread.start()
    
    # Run Flask in main thread
    run_flask()
