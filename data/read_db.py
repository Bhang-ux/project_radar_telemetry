from sqlalchemy import create_engine, text

engine = create_engine("sqlite:///data/radar_events.db")

with engine.connect() as conn:
    results = conn.execute(text("SELECT * FROM events ORDER BY id ASC"))
    rows = results.fetchall()

if not rows:
    print("No events recorded yet")
else:
    print(f"\n{'ID':<5} {'Timestamp':<22} {'Status'}")
    print("-" * 75)
    for row in rows:
        print(f"{row[0]:<5} {row[1]:<22} {row[2]}")

print(f"\nTotal events: {len(rows)}")