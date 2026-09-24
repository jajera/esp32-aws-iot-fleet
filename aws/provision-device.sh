#!/usr/bin/env bash
# Provision a single IoT Thing + cert for a fleet device pattern (not talk demo).
# Does NOT touch iot-everywhere-aws-talk Things (e.g. esp32-c61-01 on devices/).
#
# Topics use fleet/ prefix so talk ingest rules on devices/+ are not polluted.
#
# Examples:
#   ./aws/provision-device.sh
#   THING_NAME=esp32-c61-01 THING_TYPE=esp32-c61 BOARD_ATTR=ESP32-C61-DevKitC-1 PIO_ENV=esp32-c61 ./aws/provision-device.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
REGION="${AWS_REGION:-ap-southeast-2}"
THING_NAME="${THING_NAME:-esp32-c61-01}"
THING_TYPE="${THING_TYPE:-esp32-c61}"
BOARD_ATTR="${BOARD_ATTR:-ESP32-C61-DevKitC-1}"
PATTERN="${PATTERN:-$THING_TYPE}"
PIO_ENV="${PIO_ENV:-$THING_TYPE}"
UPLOAD_HINT="${UPLOAD_HINT:-/dev/ttyACM0}"
POLICY_NAME="${POLICY_NAME:-iot-fleet-${THING_NAME}-policy}"
OUT_DIR="${OUT_DIR:-$ROOT/out/${THING_NAME}}"
ACCOUNT_ID="$(aws sts get-caller-identity --query Account --output text)"

mkdir -p "$OUT_DIR"

echo "Region=$REGION Thing=$THING_NAME Type=$THING_TYPE Board=$BOARD_ATTR Out=$OUT_DIR"

if ! aws iot describe-thing-type --thing-type-name "$THING_TYPE" --region "$REGION" >/dev/null 2>&1; then
  aws iot create-thing-type --thing-type-name "$THING_TYPE" --region "$REGION" >/dev/null
  echo "Created Thing Type $THING_TYPE"
else
  echo "Thing Type $THING_TYPE already exists"
fi

if ! aws iot describe-thing --thing-name "$THING_NAME" --region "$REGION" >/dev/null 2>&1; then
  aws iot create-thing \
    --thing-name "$THING_NAME" \
    --thing-type-name "$THING_TYPE" \
    --attribute-payload "{\"attributes\":{\"model\":\"${THING_TYPE}\",\"board\":\"${BOARD_ATTR}\",\"pattern\":\"${PATTERN}\"},\"merge\":false}" \
    --region "$REGION" >/dev/null
  echo "Created Thing $THING_NAME"
else
  echo "Thing $THING_NAME already exists"
  aws iot update-thing \
    --thing-name "$THING_NAME" \
    --attribute-payload "{\"attributes\":{\"model\":\"${THING_TYPE}\",\"board\":\"${BOARD_ATTR}\",\"pattern\":\"${PATTERN}\"},\"merge\":true}" \
    --region "$REGION" >/dev/null || true
fi

CERT_ARN_FILE="$OUT_DIR/certificate-arn.txt"
if [[ -f "$CERT_ARN_FILE" ]]; then
  CERT_ARN="$(cat "$CERT_ARN_FILE")"
  echo "Reusing certificate $CERT_ARN"
else
  aws iot create-keys-and-certificate \
    --set-as-active \
    --certificate-pem-outfile "$OUT_DIR/device.pem.crt" \
    --public-key-outfile "$OUT_DIR/device.public.key" \
    --private-key-outfile "$OUT_DIR/device.private.key" \
    --region "$REGION" \
    --query certificateArn \
    --output text >"$CERT_ARN_FILE"
  CERT_ARN="$(cat "$CERT_ARN_FILE")"
  echo "Created certificate $CERT_ARN"
fi

if [[ ! -f "$OUT_DIR/AmazonRootCA1.pem" ]]; then
  curl -fsSL "https://www.amazontrust.com/repository/AmazonRootCA1.pem" -o "$OUT_DIR/AmazonRootCA1.pem"
fi

POLICY_DOC=$(cat <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:${REGION}:${ACCOUNT_ID}:client/${THING_NAME}"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:${REGION}:${ACCOUNT_ID}:topic/fleet/${THING_NAME}/telemetry",
        "arn:aws:iot:${REGION}:${ACCOUNT_ID}:topic/fleet/${THING_NAME}/events",
        "arn:aws:iot:${REGION}:${ACCOUNT_ID}:topic/fleet/${THING_NAME}/camera"
      ]
    },
    {
      "Effect": "Allow",
      "Action": "iot:Receive",
      "Resource": "arn:aws:iot:${REGION}:${ACCOUNT_ID}:topic/fleet/${THING_NAME}/*"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Subscribe",
      "Resource": "arn:aws:iot:${REGION}:${ACCOUNT_ID}:topicfilter/fleet/${THING_NAME}/*"
    }
  ]
}
EOF
)

if aws iot get-policy --policy-name "$POLICY_NAME" --region "$REGION" >/dev/null 2>&1; then
  aws iot create-policy-version \
    --policy-name "$POLICY_NAME" \
    --policy-document "$POLICY_DOC" \
    --set-as-default \
    --region "$REGION" >/dev/null
  echo "Updated policy $POLICY_NAME"
else
  aws iot create-policy \
    --policy-name "$POLICY_NAME" \
    --policy-document "$POLICY_DOC" \
    --region "$REGION" >/dev/null
  echo "Created policy $POLICY_NAME"
fi

aws iot attach-policy --policy-name "$POLICY_NAME" --target "$CERT_ARN" --region "$REGION" >/dev/null || true
aws iot attach-thing-principal --thing-name "$THING_NAME" --principal "$CERT_ARN" --region "$REGION" >/dev/null || true

ENDPOINT="$(aws iot describe-endpoint --endpoint-type iot:Data-ATS --region "$REGION" --query endpointAddress --output text)"
echo "$ENDPOINT" >"$OUT_DIR/iot-endpoint.txt"

python3 - "$OUT_DIR" "$THING_NAME" <<'PY'
import pathlib, sys
out = pathlib.Path(sys.argv[1])
thing = sys.argv[2]

def pem_c_string(path: pathlib.Path) -> str:
    text = path.read_text().strip() + "\n"
    escaped = text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f'"{escaped}"'

ca = pem_c_string(out / "AmazonRootCA1.pem")
crt = pem_c_string(out / "device.pem.crt")
key = pem_c_string(out / "device.private.key")
header = f'''#ifndef DEVICE_CERTS_H
#define DEVICE_CERTS_H

// Generated by aws/provision-device.sh for {thing} — do not commit.

#ifndef THING_NAME
#define THING_NAME "{thing}"
#endif

static const char AWS_CERT_CA[] = {ca};

static const char AWS_CERT_CRT[] = {crt};

static const char AWS_CERT_PRIVATE[] = {key};

#endif  // DEVICE_CERTS_H
'''
(out / "device_certs.h").write_text(header)
print(f"Wrote {out / 'device_certs.h'}")
PY

DEST_DIR="$ROOT/../secrets/devices/${THING_NAME}"
mkdir -p "$DEST_DIR"
cp "$OUT_DIR/device_certs.h" "$DEST_DIR/device_certs.h"
echo "Copied device_certs.h → $DEST_DIR/device_certs.h"
echo
echo "Next:"
echo "  1. Put Wi-Fi + endpoint in secrets.ini (custom_wifi_*, custom_aws_iot_endpoint)"
echo "  2. pio run -e ${PIO_ENV} -t upload --upload-port ${UPLOAD_HINT}"
echo "  3. Topics under fleet/${THING_NAME}/ (not talk devices/)"
