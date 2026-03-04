"""PlatformIO pre-build hook: generate TLS certificates for HTTPS server.

Generates a self-signed CA and device certificate (signed by the CA) with SANs
for IP:192.168.4.1 and DNS:ouispy.local. Outputs C header files that embed the
PEM/DER data for the firmware, and a ca.der for iOS profile download.

Certificates are only regenerated when missing.
"""
import os
import datetime
import sys

Import("env")  # noqa: F821 — PlatformIO injects this

PROJECT_DIR = env.subst("$PROJECT_DIR")
CERTS_DIR = os.path.join(PROJECT_DIR, "certs")
HEADER_DIR = os.path.join(PROJECT_DIR, "src", "web", "certs")

# Output files
CA_KEY_FILE = os.path.join(CERTS_DIR, "ca.key")
CA_CERT_PEM_FILE = os.path.join(CERTS_DIR, "ca.pem")
CA_CERT_DER_FILE = os.path.join(CERTS_DIR, "ca.der")
DEV_KEY_FILE = os.path.join(CERTS_DIR, "dev.key")
DEV_CERT_PEM_FILE = os.path.join(CERTS_DIR, "dev.pem")

# C header outputs
CA_CERT_PEM_H = os.path.join(HEADER_DIR, "ca_cert_pem.h")
DEV_CERT_PEM_H = os.path.join(HEADER_DIR, "dev_cert_pem.h")
DEV_KEY_PEM_H = os.path.join(HEADER_DIR, "dev_key_pem.h")
CA_CERT_DER_H = os.path.join(HEADER_DIR, "ca_cert_der.h")


def needs_generation():
    """Check if any output file is missing."""
    for f in [CA_KEY_FILE, CA_CERT_PEM_FILE, CA_CERT_DER_FILE,
              DEV_KEY_FILE, DEV_CERT_PEM_FILE,
              CA_CERT_PEM_H, DEV_CERT_PEM_H, DEV_KEY_PEM_H, CA_CERT_DER_H]:
        if not os.path.exists(f):
            return True
    return False


def bytes_to_c_array(data, var_name, header_guard):
    """Convert bytes to a C header with a const uint8_t array."""
    lines = [
        f"#pragma once",
        f"// Auto-generated — do not edit",
        f"#include <cstdint>",
        f"#include <cstddef>",
        f"",
        f"static const uint8_t {var_name}[] = {{",
    ]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_vals},")
    lines.append("};")
    lines.append(f"static const size_t {var_name}_LEN = sizeof({var_name});")
    lines.append("")
    return "\n".join(lines)


def generate_certs():
    """Generate CA + device certificates using the cryptography library."""
    try:
        from cryptography import x509
        from cryptography.x509.oid import NameOID
        from cryptography.hazmat.primitives import hashes, serialization
        from cryptography.hazmat.primitives.asymmetric import rsa
        import ipaddress
    except ImportError:
        print("[generate_cert] ERROR: 'cryptography' package not installed.", file=sys.stderr)
        print("[generate_cert] Run: pip install cryptography", file=sys.stderr)
        env.Exit(1)
        return

    os.makedirs(CERTS_DIR, exist_ok=True)
    os.makedirs(HEADER_DIR, exist_ok=True)

    now = datetime.datetime.now(datetime.timezone.utc)
    validity = datetime.timedelta(days=3650)  # 10 years

    # --- CA Key + Certificate ---
    ca_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    ca_name = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, "OUI SPY CA"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "OUI SPY"),
    ])
    ca_cert = (
        x509.CertificateBuilder()
        .subject_name(ca_name)
        .issuer_name(ca_name)
        .public_key(ca_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + validity)
        .add_extension(x509.BasicConstraints(ca=True, path_length=0), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True, key_cert_sign=True, crl_sign=True,
                content_commitment=False, key_encipherment=False,
                data_encipherment=False, key_agreement=False,
                encipher_only=False, decipher_only=False,
            ),
            critical=True,
        )
        .sign(ca_key, hashes.SHA256())
    )

    # --- Device Key + Certificate (signed by CA) ---
    dev_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    dev_name = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, "ouispy.local"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "OUI SPY"),
    ])
    dev_cert = (
        x509.CertificateBuilder()
        .subject_name(dev_name)
        .issuer_name(ca_name)
        .public_key(dev_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(now)
        .not_valid_after(now + validity)
        .add_extension(
            x509.SubjectAlternativeName([
                x509.IPAddress(ipaddress.IPv4Address("192.168.4.1")),
                x509.DNSName("ouispy.local"),
            ]),
            critical=False,
        )
        .add_extension(
            x509.KeyUsage(
                digital_signature=True, key_encipherment=True,
                content_commitment=False, key_cert_sign=False, crl_sign=False,
                data_encipherment=False, key_agreement=False,
                encipher_only=False, decipher_only=False,
            ),
            critical=True,
        )
        .add_extension(
            x509.ExtendedKeyUsage([x509.oid.ExtendedKeyUsageOID.SERVER_AUTH]),
            critical=False,
        )
        .sign(ca_key, hashes.SHA256())
    )

    # --- Write PEM/DER files ---
    ca_key_pem = ca_key.private_bytes(
        serialization.Encoding.PEM,
        serialization.PrivateFormat.TraditionalOpenSSL,
        serialization.NoEncryption(),
    )
    ca_cert_pem = ca_cert.public_bytes(serialization.Encoding.PEM)
    ca_cert_der = ca_cert.public_bytes(serialization.Encoding.DER)
    dev_key_pem = dev_key.private_bytes(
        serialization.Encoding.PEM,
        serialization.PrivateFormat.TraditionalOpenSSL,
        serialization.NoEncryption(),
    )
    dev_cert_pem = dev_cert.public_bytes(serialization.Encoding.PEM)

    with open(CA_KEY_FILE, "wb") as f:
        f.write(ca_key_pem)
    with open(CA_CERT_PEM_FILE, "wb") as f:
        f.write(ca_cert_pem)
    with open(CA_CERT_DER_FILE, "wb") as f:
        f.write(ca_cert_der)
    with open(DEV_KEY_FILE, "wb") as f:
        f.write(dev_key_pem)
    with open(DEV_CERT_PEM_FILE, "wb") as f:
        f.write(dev_cert_pem)

    # --- Write C headers ---
    with open(CA_CERT_PEM_H, "w") as f:
        f.write(bytes_to_c_array(ca_cert_pem, "CA_CERT_PEM", "CA_CERT_PEM_H"))
    with open(DEV_CERT_PEM_H, "w") as f:
        f.write(bytes_to_c_array(dev_cert_pem, "DEV_CERT_PEM", "DEV_CERT_PEM_H"))
    with open(DEV_KEY_PEM_H, "w") as f:
        f.write(bytes_to_c_array(dev_key_pem, "DEV_KEY_PEM", "DEV_KEY_PEM_H"))
    with open(CA_CERT_DER_H, "w") as f:
        f.write(bytes_to_c_array(ca_cert_der, "CA_CERT_DER", "CA_CERT_DER_H"))

    print(f"[generate_cert] Generated certificates in {CERTS_DIR}")
    print(f"[generate_cert] Generated C headers in {HEADER_DIR}")


if needs_generation():
    generate_certs()
else:
    print("[generate_cert] Certificates already exist, skipping generation")
