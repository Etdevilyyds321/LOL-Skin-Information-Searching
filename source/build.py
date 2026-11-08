"""Build with Python 3 and Zig 0.14.1 (or a compatible release)."""
from pathlib import Path
import os
import subprocess
from generate_data import generate

ROOT = Path(__file__).resolve().parent
zig = os.environ.get('ZIG', 'zig')
generate()
# Keep the executable manifest version consistent with the version resource.
import re
config = (ROOT / 'update_config.h').read_text(encoding='utf-8')
resource_version = re.search(r'#define APP_VERSION_RESOURCE "([0-9.]+)"', config).group(1)
manifest = ROOT / 'app.manifest'
manifest.write_text(re.sub(r'(<assemblyIdentity version=")[0-9.]+', lambda m: m.group(1) + resource_version, manifest.read_text(encoding='utf-8')), encoding='utf-8')
subprocess.run([zig, 'rc', '/fo', 'app.res', 'app.rc'], cwd=ROOT, check=True)
output = ROOT.parent / 'LOL-Skin-Information-Searching_v1.0.exe'
subprocess.run([zig, 'cc', '-target', 'x86_64-windows-gnu', '-municode', '-O2',
                '-Wall', '-Wextra', '-static', '-s', '-Wl,--subsystem,windows',
                'main.c', 'app.res', '-lcomctl32', '-lgdi32', '-lshell32', '-lwinhttp',
                '-lole32', '-luuid', '-o', str(output)], cwd=ROOT, check=True)
print('Built:', output)
