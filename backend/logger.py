import paho.mqtt.client as mqtt
from sqlalchemy import create_engine, text
from datetime import datetime

# --- CONFIG ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_TOPIC  = "nivaasiq/bathroom/status"
DB_PATH     = "data/radar_events.db"
# --------------

engine = create_engine(f"sqlite:///{DB_PATH}")

with engine.connect() as conn:
    conn.execute(text("""
        CREATE TABLE IF NOT EXISTS events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp   TEXT,
            status      TEXT
        )
    """))
    conn.commit()

print("Database ready.")

def log_event(status):
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with engine.connect() as conn:
        conn.execute(text(
            "INSERT INTO events (timestamp, status) VALUES (:ts, :st)"
        ), {"ts": timestamp, "st": status})
        conn.commit()
    print(f"[{timestamp}] Logged: {status}")

last_status = ""

def on_message(client, userdata, msg):
    global last_status
    status = msg.payload.decode()
    if status != last_status:
        log_event(status)
        last_status = status

def on_connect(client, userdata, flags, reason_code, properties):
    print("Logger connected to MQTT")
    client.subscribe(MQTT_TOPIC)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

print("NivaasIQ Logger starting...")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()