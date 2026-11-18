# Language-aware demo data for the screenshots / GIFs
import sys
sys.path.insert(0, "/home/claude/repo/pcmigrate-portable/tools/i18n")
from strings import S
LI = {"en": 1, "fr": 2, "ar": 3, "es": 4}
YESNO = {"en": ("Yes", "No"), "fr": ("Oui", "Non"), "ar": ("نعم", "لا"), "es": ("Sí", "No")}
LANG = "en"

def ui(lang=None):
    lang = lang or LANG
    d = {r[0]: r[LI[lang]] for r in S}
    d["YES"], d["NO"] = YESNO[lang]
    return d

def tr(key, *a):
    t = ui()[key].replace("%ls", "%s").replace("%lu", "%d").replace("%08lX", "%08X")
    return t % a if a else t

def L(ts, key, *a): return f"[{ts}] " + tr(key, *a)

def src_label(): return tr("S_INST_LABEL", "D:", "Windows", "476.94 GB", "183.20 GB", tr("S_OFFLINE"))

def base_state():
    return {
        "mode": "gif", "tab": 0, "busy": False, "progress": 0, "status": tr("S_READY"),
        "ui": ui(), "rtl": LANG == "ar",
        "log": ["[10:02:11] PC Migrate Portable 1.2" + tr("S_L_RUN_PE"), L("10:02:11", "S_L_FOUND", 1)],
        "b": {"src": src_label(),
              "users": [{"name": "Ahmed", "path": "D:\\Users\\Ahmed", "checked": True},
                        {"name": "Salma", "path": "D:\\Users\\Salma", "checked": True}],
              "files": True, "browsers": True, "settings": True, "programs": True,
              "apps": False, "appsCount": 0, "dest": "E:\\"},
        "r": {"pkg": "E:\\PCMigrate_20261118_1002", "srcUser": "Ahmed",
              "tgt": "C:\\Users\\Ahmed" + tr("S_YOU"),
              "files": True, "browsers": True, "settings": True, "prefer": False, "progs": []},
    }

CHOOSER_APPS = [
    ("7-Zip 24.08 (x64)", "24.08", "5.60 MB", "C:\\Program Files\\7-Zip", None),
    ("Discord", "1.0.9170", "78.31 MB", "C:\\Users\\Ahmed\\AppData\\Local\\Discord", "user"),
    ("FileZilla 3.68.1", "3.68.1", "42.10 MB", "C:\\Program Files\\FileZilla FTP Client", None),
    ("GestCom Pro 5.2", "5.2.0", "310.45 MB", "C:\\GestCom", None),
    ("Google Chrome", "131.0.6778.86", "", "C:\\Program Files\\Google\\Chrome\\Application", "msi"),
    ("Kaspersky Free", "21.19", "612.00 MB", "C:\\Program Files (x86)\\Kaspersky Lab\\Kaspersky 21.19", None),
    ("Microsoft Office Professional Plus 2019", "16.0.10417", "", "C:\\Program Files\\Microsoft Office", "msi"),
    ("Notepad++ (64-bit x64)", "8.7.1", "16.20 MB", "C:\\Program Files\\Notepad++", None),
    ("Python 3.12.7 (64-bit)", "3.12.7", "101.30 MB", "", "nofolder"),
    ("VLC media player", "3.0.21", "169.40 MB", "C:\\Program Files\\VideoLAN\\VLC", None),
    ("WinRAR 7.01 (64-bit)", "7.01.0", "11.80 MB", "C:\\Program Files\\WinRAR", None),
    ("Zoom Workplace", "6.2.11", "", "C:\\Users\\Ahmed\\AppData\\Roaming\\Zoom\\bin", "user"),
]
PICK = ["7-Zip 24.08 (x64)", "Discord", "GestCom Pro 5.2", "Notepad++ (64-bit x64)", "VLC media player"]

def chooser(selected=()):
    out = []
    for n, v, sz, d, kind in CHOOSER_APPS:
        if kind == "nofolder": info = tr("S_NOTE_NOFOLDER")
        elif kind == "msi": info = d + "   (MSI)"
        elif kind == "user": info = d + tr("S_USER_TAG", "Ahmed")
        else: info = d
        out.append({"name": n, "ver": v, "size": sz, "info": info, "checked": n in selected})
    return {"apps": out}

RESTORE_PROGS = [
    ("7-Zip 24.08 (x64)", "24.08", ("or", "7zip.7zip"), True),
    ("Discord", "1.0.9170", ("or", "Discord.Discord"), True),
    ("GestCom Pro 5.2", "5.2.0", ("copy", None), True),
    ("Notepad++ (64-bit x64)", "8.7.1", ("or", "Notepad++.Notepad++"), True),
    ("VLC media player", "3.0.21", ("or", "VideoLAN.VLC"), True),
    ("Google Chrome", "131.0.6778.86", ("wg", "Google.Chrome"), True),
    ("Mozilla Firefox (x64 fr)", "132.0.2", ("wg", "Mozilla.Firefox"), True),
    ("Zoom Workplace", "6.2.11", ("wg", "Zoom.Zoom"), True),
    ("FileZilla 3.68.1", "3.68.1", ("wg", "TimKosse.FileZilla.Client"), False),
    ("Realtek Audio Driver", "6.0.9549.1", ("name", None), False),
]
def method(m):
    k, i = m
    return {"or": lambda: tr("S_M_COPY_OR", i), "copy": lambda: tr("S_M_COPY"), "wg": lambda: tr("S_M_WINGET", i), "name": lambda: tr("S_M_WINGET_NAME")}[k]()

def restore_progs(check=True):
    return [{"name": p[0], "ver": p[1], "method": method(p[2]), "checked": p[3] and check} for p in RESTORE_PROGS]

def restore_log():
    return ["[11:20:04] PC Migrate Portable 1.2" + tr("S_L_RUN_WIN", "C:"), L("11:20:04", "S_L_FOUND", 1),
            L("11:20:05", "S_L_PKGLOADED", "2026-11-18 10:02:11", 2, 64, 5)]
