"""
Example C-IDS plugin.

Contract (see docs/PLUGIN_GUIDE.md):
  - Must define a top-level `on_detection(event: dict) -> None`.
  - `event` keys: signature_id (int), severity (int, 0-100),
    timestamp_ns (int), description (str).
  - Must not block for long periods — it runs synchronously on the
    engine's detection thread while holding the GIL.
  - Any exception raised here is caught and logged by the host; it will
    NOT crash the engine, but it also won't retry.
"""

def on_detection(event: dict) -> None:
    if event["severity"] >= 90:
        print(
            f"[auto-block] signature={event['signature_id']} "
            f"severity={event['severity']} :: {event['description']}"
        )
        # Real implementation would call out to a firewall API, EDR
        # integration, SOAR webhook, etc. Kept as a stub here.
