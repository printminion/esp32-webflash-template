#!/usr/bin/env python3
"""
set_wifi_ap_flags.py — Set PLATFORMIO_BUILD_FLAGS for WiFi AP policy,
                       OTA credentials, and VERSION_CHECK_URL.

Reads configuration from environment variables and writes build flags
to GITHUB_ENV.

Environment variables:
  WIFI_AP_PASSWORD_SECRET   The WIFI_AP_PASSWORD secret value (empty if not set).
  WIFI_STRICT               "true" = release policy (fail if unconfigured);
                            "false" = dev policy (open AP fallback).
  WIFI_AP_ALLOW_OPEN        "1" to allow open AP in release builds (opt-in).
  OTA_USERNAME_SECRET       Optional: username for the /update OTA endpoint.
  OTA_PASSWORD_SECRET       Optional: password for the /update OTA endpoint.
  BOARD_ID                  Board environment name (e.g. seeed_xiao_esp32c3).
                            Used to compute per-board VERSION_CHECK_URL.

Usage (called by CI workflows via scripts/set_wifi_ap_flags.py):
  WIFI_STRICT=false python scripts/set_wifi_ap_flags.py           # dev
  WIFI_STRICT=true  python scripts/set_wifi_ap_flags.py           # release
"""
import json
import os
import sys
import shlex
import uuid

pw           = os.environ.get('WIFI_AP_PASSWORD_SECRET', '')
strict       = os.environ.get('WIFI_STRICT', 'false').lower() == 'true'
allow_open   = os.environ.get('WIFI_AP_ALLOW_OPEN', '')
github_env   = os.environ['GITHUB_ENV']
step_summary = os.environ.get('GITHUB_STEP_SUMMARY', '')
repo         = os.environ.get('GITHUB_REPOSITORY', '')
ota_user     = os.environ.get('OTA_USERNAME_SECRET', '').strip()
ota_pass     = os.environ.get('OTA_PASSWORD_SECRET', '').strip()
board_id     = os.environ.get('BOARD_ID', '').strip()


def escape_for_c_string(s: str) -> str:
    """Escape a value for embedding in a C string literal."""
    return (s.replace('\\', '\\\\')
             .replace('"', '\\"')
             .replace('\n', '\\n')
             .replace('\r', '\\r'))

if pw:
    # Use shlex.quote on the C string literal so single-quotes in the
    # password don't break PlatformIO's shlex-style flag parsing.
    c_literal = '"' + escape_for_c_string(pw) + '"'
    flag = "-D WIFI_AP_PASSWORD=" + shlex.quote(c_literal)
elif not strict:
    # Dev builds: fall back to open AP when no password is configured.
    flag = "-D WIFI_AP_OPEN=1"
elif allow_open == '1':
    # Release builds: explicit opt-in to open AP via repo variable.
    flag = "-D WIFI_AP_OPEN=1"
else:
    # Release builds: neither password nor opt-in configured — block the build.
    settings_url = f"https://github.com/{repo}/settings/secrets/actions"
    link = f"[Settings \u2192 Secrets and variables \u2192 Actions]({settings_url})"
    lines = [
        "## \u274c Release blocked: WiFi AP policy not configured",
        "",
        "The release build requires an explicit WiFi AP security policy.",
        "Choose **one** of the following options, then re-run the workflow.",
        "",
        "### Option 1 \u2014 Password-protected AP (recommended)",
        "",
        f"1. Go to {link}",
        "2. Click **New repository secret**",
        "3. Name: `WIFI_AP_PASSWORD`",
        "4. Value: your chosen password (minimum 8 characters)",
        "5. Click **Add secret**, then re-run this workflow",
        "",
        "### Option 2 \u2014 Open AP (testing / demo only, not for production)",
        "",
        f"1. Go to {link}",
        "2. Click the **Variables** tab \u2192 **New repository variable**",
        "3. Name: `WIFI_AP_ALLOW_OPEN_RELEASE`",
        "4. Value: `1`",
        "5. Click **Add variable**, then re-run this workflow",
        "",
    ]
    if step_summary:
        with open(step_summary, 'a', encoding='utf-8') as f:
            f.write("\n".join(lines))
    print("::error::WiFi AP policy not configured. See the job Summary tab for setup instructions.")
    sys.exit(1)

# ── Optional OTA credentials ──────────────────────────────────────────────────
# Both must be set together; either alone is silently ignored (incomplete config).
extra_flags = [flag]

if ota_user and ota_pass:
    extra_flags.append("-D OTA_USERNAME=" + shlex.quote('"' + escape_for_c_string(ota_user) + '"'))
    extra_flags.append("-D OTA_PASSWORD=" + shlex.quote('"' + escape_for_c_string(ota_pass) + '"'))

# ── VERSION_CHECK_URL — auto-computed per board from project.json ─────────────
# Enabled whenever installer.baseUrl is set and BOARD_ID is known.
try:
    with open('project.json', encoding='utf-8') as pf:
        project = json.load(pf)
    base_url = project.get('installer', {}).get('baseUrl', '').rstrip('/')
    if base_url and board_id:
        version_url = f"{base_url}/version/{board_id}.json"
        extra_flags.append("-D VERSION_CHECK_URL=" + shlex.quote(f'"{version_url}"'))
except (OSError, json.JSONDecodeError):
    pass  # project.json unavailable — skip VERSION_CHECK_URL injection

# ── Write all flags to GITHUB_ENV (multiline syntax) ─────────────────────────
# Multiline syntax prevents newlines inside values from corrupting the env file.
pio_flags_delimiter = 'EOF_PIO_FLAGS_' + uuid.uuid4().hex
with open(github_env, 'a', encoding='utf-8') as f:
    flags_value = '\n'.join(extra_flags)
    f.write(f"PLATFORMIO_BUILD_FLAGS<<{pio_flags_delimiter}\n{flags_value}\n{pio_flags_delimiter}\n")
