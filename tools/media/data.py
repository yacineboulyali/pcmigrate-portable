import copy

SRC = "D:  [Windows]  476.94 GB total, 183.20 GB free   (offline)"

BACKUP_LOG = [
    "[10:02:11] PC Migrate Portable 1.1  -  running in WinPE (offline mode)",
    "[10:02:11] Found 1 Windows installation(s).",
]

def base_state():
    return {
        "mode": "gif", "tab": 0, "busy": False, "progress": 0, "status": "Ready.",
        "log": list(BACKUP_LOG),
        "b": {"src": SRC,
              "users": [{"name": "Ahmed", "path": "D:\\Users\\Ahmed", "checked": True},
                        {"name": "Salma", "path": "D:\\Users\\Salma", "checked": True}],
              "files": True, "browsers": True, "settings": True, "programs": True,
              "apps": False, "appsCount": 0, "dest": "E:\\"},
        "r": {"pkg": "E:\\PCMigrate_20261118_1002", "srcUser": "Ahmed",
              "tgt": "C:\\Users\\Ahmed   (you - current user)",
              "files": True, "browsers": True, "settings": True, "prefer": False,
              "progs": []},
    }

CHOOSER_APPS = [
    ("7-Zip 24.08 (x64)", "24.08", "5.60 MB", "C:\\Program Files\\7-Zip"),
    ("Discord", "1.0.9170", "78.31 MB", "C:\\Users\\Ahmed\\AppData\\Local\\Discord   [user: Ahmed]"),
    ("FileZilla 3.68.1", "3.68.1", "42.10 MB", "C:\\Program Files\\FileZilla FTP Client"),
    ("GestCom Pro 5.2", "5.2.0", "310.45 MB", "C:\\GestCom"),
    ("Google Chrome", "131.0.6778.86", "", "C:\\Program Files\\Google\\Chrome\\Application   (MSI)"),
    ("Kaspersky Free", "21.19", "612.00 MB", "C:\\Program Files (x86)\\Kaspersky Lab\\Kaspersky 21.19"),
    ("Microsoft Office Professional Plus 2019", "16.0.10417", "", "C:\\Program Files\\Microsoft Office   (MSI)"),
    ("Notepad++ (64-bit x64)", "8.7.1", "16.20 MB", "C:\\Program Files\\Notepad++"),
    ("Python 3.12.7 (64-bit)", "3.12.7", "101.30 MB", "no install folder - data only"),
    ("VLC media player", "3.0.21", "169.40 MB", "C:\\Program Files\\VideoLAN\\VLC"),
    ("WinRAR 7.01 (64-bit)", "7.01.0", "11.80 MB", "C:\\Program Files\\WinRAR"),
    ("Zoom Workplace", "6.2.11", "", "C:\\Users\\Ahmed\\AppData\\Roaming\\Zoom\\bin   [user: Ahmed]"),
]
PICK = {"7-Zip 24.08 (x64)", "Discord", "GestCom Pro 5.2", "Notepad++ (64-bit x64)", "VLC media player"}

def chooser(selected=()):
    return {"apps": [{"name": a[0], "ver": a[1], "size": a[2], "info": a[3], "checked": a[0] in selected} for a in CHOOSER_APPS]}

RESTORE_PROGS = [
    ("7-Zip 24.08 (x64)", "24.08", "Copy from package  |  or winget: 7zip.7zip", True),
    ("Discord", "1.0.9170", "Copy from package  |  or winget: Discord.Discord", True),
    ("GestCom Pro 5.2", "5.2.0", "Copy from package (files + data)", True),
    ("Notepad++ (64-bit x64)", "8.7.1", "Copy from package  |  or winget: Notepad++.Notepad++", True),
    ("VLC media player", "3.0.21", "Copy from package  |  or winget: VideoLAN.VLC", True),
    ("Google Chrome", "131.0.6778.86", "winget: Google.Chrome", True),
    ("Mozilla Firefox (x64 fr)", "132.0.2", "winget: Mozilla.Firefox", True),
    ("Zoom Workplace", "6.2.11", "winget: Zoom.Zoom", True),
    ("FileZilla 3.68.1", "3.68.1", "winget: TimKosse.FileZilla.Client", False),
    ("Realtek Audio Driver", "6.0.9549.1", "winget: search by name", False),
]

def restore_progs(check=True):
    return [{"name": p[0], "ver": p[1], "method": p[2], "checked": p[3] and check} for p in RESTORE_PROGS]

RESTORE_LOG = [
    "[11:20:04] PC Migrate Portable 1.1  -  running on Windows C:",
    "[11:20:04] Found 1 Windows installation(s).",
    "[11:20:05] Package loaded (created 2026-11-18 10:02:11): 2 user(s), 64 program(s), 5 saved with files + data.",
]
