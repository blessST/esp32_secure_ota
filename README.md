# ESP32-S3 Secure Boot + OTA Update Project

This project demonstrates how to use **Secure Boot V2** with **HTTPS OTA updates** on the ESP32-S3. It includes:

✅ Automatic firmware signing using RSA-3072
✅ Auto-generated `metadata.json` for OTA updates
✅ Version check before downloading new firmware
✅ GitHub Actions CI to build and sign firmware
✅ Support for reproducible builds with `sdkconfig.defaults`

---

## 📂 Project Structure

```
esp32-secure-ota/
 ├─ main/
 │   ├─ main.c               # OTA update logic with version check
 │   └─ CMakeLists.txt
 ├─ CMakeLists.txt           # Handles firmware signing & metadata generation
 ├─ partitions.csv           # Partition table with OTA support
 ├─ tools/
 │   └─ gen_metadata.py      # Generates metadata.json
 ├─ sdkconfig.defaults       # Default ESP-IDF config for reproducible builds
 └─ .github/workflows/
     └─ build.yml           # CI build pipeline
```

---

## 🔑 Generating Signing Keys

Generate a Secure Boot signing key (only once):

```bash
espsecure.py generate_signing_key --version 2 signing_key.pem
```

---

## 🔨 Building Signed Firmware

Build and sign the firmware:

```bash
idf.py build
```

The build will produce:

```
build/esp32_secure_ota-signed.bin  # Signed firmware
build/metadata.json                # OTA metadata (version, URL, hash)
```

---

## ⚙️ Managing `sdkconfig` and `sdkconfig.defaults`

To create a reproducible configuration for CI, generate `sdkconfig.defaults` from your current `sdkconfig`:

```bash
idf.py save-defconfig
```

This creates a minimal `sdkconfig.defaults` with only non-default options.

### 🛠 Using in CI

When building in CI:

```bash
idf.py defconfig build
```

This regenerates `sdkconfig` from `sdkconfig.defaults` and builds the firmware.

---

## 🔥 Flashing Secure Firmware

Flash the **signed binary**:

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x10000 build/esp32_secure_ota-signed.bin
```

Flash bootloader & partition table (if not already done):

```bash
idf.py -p /dev/ttyUSB0 flash
```

---

## 🔒 Enabling Secure Boot V2

1️⃣ Extract public key and digest:

```bash
espsecure.py extract_public_key --key signing_key.pem public_key.pem
espsecure.py digest_public_key --keyfile public_key.pem --output key_digest.bin
```

2️⃣ Burn the digest to eFuse (irreversible):

```bash
espefuse.py --chip esp32s3 burn_key_digest 0 key_digest.bin
```

3️⃣ Enable Secure Boot:

```bash
espefuse.py --chip esp32s3 set_secure_boot_v2_en
```

⚠️ After this step, **only signed firmware can run.**

(Optional) Enable flash encryption:

```bash
espefuse.py --chip esp32s3 set_flash_encryption
```

---

## 🌐 OTA Update Process

1️⃣ Host these files on your HTTPS server or GitHub Release:

```
- build/esp32_secure_ota-signed.bin
- build/metadata.json
```

Example `metadata.json`:

```json
{
  "version": "1.0.1",
  "url": "https://example.com/esp32_secure_ota-signed.bin",
  "sha256": "abcdef1234567890..."
}
```

2️⃣ The ESP32 boots, downloads `metadata.json`, compares version, and updates only if the new version is greater.

3️⃣ Bootloader verifies the **RSA-3072 signature** at boot. If invalid, the new firmware will not run.

---

## 🚀 GitHub Actions CI

The included `.github/workflows/build.yml` will:

✅ Build firmware
✅ Sign it automatically
✅ Generate `metadata.json`
✅ Upload artifacts

Trigger the workflow by pushing a tag like `v1.0.1`:

```bash
git tag v1.0.1
git push origin v1.0.1
```

---

## ⚠️ Important Notes

🔒 Secure Boot eFuse burning is **irreversible**.
🔒 Always back up `signing_key.pem`. If lost, you can never update the device again.
🔒 The same key must be used for all future firmware updates.

---

## 🛠 OTA Logic

* Device downloads `metadata.json`
* Compares current version with server version
* If newer → downloads signed firmware → installs
* Bootloader verifies signature → boots only if valid

---

## ✅ Summary

✔️ Secure Boot V2 ensures only signed firmware runs
✔️ OTA updates via HTTPS are automatic and secure
✔️ Version checking prevents unnecessary downloads
✔️ GitHub Actions can automate build, signing, and release
✔️ `sdkconfig.defaults` ensures reproducible builds in CI
