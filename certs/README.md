# Certs directory (gitignored contents)

Download the Amazon Root CA and keep claim or device PEMs outside git.

```bash
mkdir -p secrets
curl -sS -o secrets/AmazonRootCA1.pem \
  https://www.amazontrust.com/repository/AmazonRootCA1.pem
```

Claim cert and key from AWS IoT (CreateKeysAndCertificate or console) belong in `secrets/claim/`.

Placeholder filenames used by docs and scripts:

- `secrets/claim/claim-cert.pem`
- `secrets/claim/claim-key.pem`
- `secrets/AmazonRootCA1.pem`

This folder only tracks this README; all `*.pem` / `*.key` are ignored by `.gitignore`.
