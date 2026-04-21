"""
Sentinel Cloud Dashboard
========================
Flask backend that receives incident reports from edge devices and serves a
real-time web dashboard.

Run:
    pip install flask flask-socketio pillow
    python app.py
"""

import os
import uuid
import json
from datetime import datetime
from pathlib import Path

from flask import Flask, request, jsonify, render_template, send_from_directory
from flask_socketio import SocketIO

# ─── Config ──────────────────────────────────────────────────────────────────

BASE_DIR      = Path(__file__).parent
INCIDENT_DIR  = BASE_DIR / "incidents"
INCIDENT_DIR.mkdir(exist_ok=True)

DB_FILE       = BASE_DIR / "incidents.json"

app    = Flask(__name__)
app.config["SECRET_KEY"] = os.environ.get("SECRET_KEY", "sentinel-dev-secret")
io     = SocketIO(app, cors_allowed_origins="*")

# ─── Persistence helpers ─────────────────────────────────────────────────────

def _load_incidents() -> list:
    if DB_FILE.exists():
        return json.loads(DB_FILE.read_text())
    return []


def _save_incident(record: dict) -> None:
    incidents = _load_incidents()
    incidents.insert(0, record)          # newest first
    DB_FILE.write_text(json.dumps(incidents, indent=2))

# ─── Routes ──────────────────────────────────────────────────────────────────

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/incidents/<path:filename>")
def serve_incident_image(filename):
    return send_from_directory(INCIDENT_DIR, filename)


@app.route("/api/incidents", methods=["GET"])
def get_incidents():
    """Return paginated incident list."""
    page  = int(request.args.get("page",  1))
    limit = int(request.args.get("limit", 20))
    all_  = _load_incidents()
    start = (page - 1) * limit
    return jsonify({
        "total":     len(all_),
        "page":      page,
        "incidents": all_[start: start + limit],
    })


@app.route("/api/report-incident", methods=["POST"])
def report_incident():
    """
    Receives a multipart POST from the edge device:
        image    – JPEG frame
        type     – 'drowsiness' | 'crash'
        metadata – optional query-string-style key=value pairs
    """
    if "image" not in request.files:
        return jsonify({"error": "No image provided"}), 400

    img_file  = request.files["image"]
    inc_type  = request.form.get("type", "drowsiness")
    metadata  = request.form.get("metadata", "")

    # Persist image
    img_name  = f"{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}_{uuid.uuid4().hex[:6]}.jpg"
    img_path  = INCIDENT_DIR / img_name
    img_file.save(img_path)

    record = {
        "id":        uuid.uuid4().hex,
        "type":      inc_type,
        "timestamp": datetime.utcnow().isoformat() + "Z",
        "image":     img_name,
        "metadata":  metadata,
    }
    _save_incident(record)

    # Push to all connected dashboard clients in real-time
    io.emit("new_incident", record)

    print(f"[Dashboard] {inc_type.upper()} incident received → {img_name}")
    return jsonify({"status": "ok", "id": record["id"]}), 201


@app.route("/api/stats", methods=["GET"])
def stats():
    incidents = _load_incidents()
    total     = len(incidents)
    by_type   = {"drowsiness": 0, "crash": 0}
    for inc in incidents:
        by_type[inc.get("type", "drowsiness")] = \
            by_type.get(inc.get("type", "drowsiness"), 0) + 1
    return jsonify({"total": total, "by_type": by_type})


# ─── Run ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    print("Sentinel Dashboard → http://0.0.0.0:5000")
    io.run(app, host="0.0.0.0", port=5000, debug=False)
