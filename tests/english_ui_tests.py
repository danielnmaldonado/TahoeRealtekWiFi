#!/usr/bin/env python3
"""Check English-only app metadata, resources and known translated UI strings."""
from pathlib import Path
import plistlib
import re
project=Path(__file__).resolve().parents[1]
info=plistlib.loads((project/"app/Info.plist").read_bytes())
assert info["CFBundleDevelopmentRegion"]=="en"
assert info["CFBundleLocalizations"]==["en"]
assert info["CFBundleShortVersionString"]=="0.3.5"
ui=(project/"app/Main.mm").read_text()
for label in ["Connected via WPA2","Open at login","Refresh networks",
              "Signal diagnostics…","Adapter catalog…","Copy report","Quit"]:
    assert '@"'+label+'"' in ui,label
assert ".localizedDescription" not in ui
assert "RTWEnglishError(" in ui
for path in (project/"app/pkg/resources").glob("*.html"):
    assert '<html lang="en">' in path.read_text(),path
literal=re.compile(r'@?"((?:\\.|[^"\\])*)"')
for path in (project/"app").iterdir():
    if path.suffix not in {".h",".hpp",".cpp",".mm",".swift"}:continue
    for text in literal.findall(path.read_text()):
        assert not re.search(r"[ãõáéíóúçÁÉÍÓÚÃÕÇ]",text),(path,text)
        assert not re.search(r"\b(?:Conectado|Adaptador|Auxiliar|Senha|Senhas|Sair|Nenhum|Nenhuma|Indisponivel)\b",text),(path,text)
print("English app metadata, key menu labels, errors and installer resources passed")
