"""Example pre-provisioning hook.

Register this Lambda on your fleet provisioning template.
Replace the allowlists with your manufacturing DB / Parameter Store lookup.

Never log raw certificates or private material.
"""


KNOWN_MODELS = frozenset({"ideaspark-oled", "esp32-s3", "esp32-c61", "esp32-c3"})


def handler(event, context):
    # event includes claim certificate id and template parameters from the device
    params = event.get("parameters") or {}
    model = params.get("Model", "")
    serial = params.get("SerialNumber", "")

    allowed = model in KNOWN_MODELS and bool(serial) and len(serial) <= 64

    return {
        "allowProvisioning": allowed,
        "parameterOverrides": {
            "Model": model,
            "SerialNumber": serial,
        },
    }
