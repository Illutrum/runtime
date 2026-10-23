import re
import sys
from pathlib import Path

root = Path(sys.argv[1]) / "fgfx"
pattern = re.compile(
  r"\n  namespace \{\n    struct \w+Registrar \{.*?\n  \}\n",
  re.S,
)
for path in sorted(root.glob("*Asset.cpp")):
  text = path.read_text()
  updated, count = pattern.subn("\n", text)
  if count:
    path.write_text(updated)
    print(f"stripped {path.name}")
