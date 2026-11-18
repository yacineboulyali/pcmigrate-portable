# "How it works" image in French and Spanish
import asyncio, os
from playwright.async_api import async_playwright
import annot as A
import data
data.LANG = 'en'
T = {
 "fr": ("Comment fonctionne PC Migrate Portable ?", "Transférez fichiers, programmes et paramètres vers un nouveau PC en 3 étapes — sans installation, sans Internet",
        [("Ancien PC", ["Démarrez la clé Hiren's (ou lancez-le sous Windows)", "Onglet <b>Sauvegarde</b> : utilisateurs et programmes", "<b>Lancer la sauvegarde</b>"]),
         ("Paquet de migration", ["Un seul dossier sur un disque externe ou une clé USB", "Fichiers, navigateurs, paramètres, Wi-Fi", "Programmes avec leurs fichiers et données"]),
         ("Nouveau PC", ["Onglet <b>Restauration</b> : <b>Lancer la restauration</b>", "<b>Installer / transférer</b> les programmes", "Redémarrez le PC ✓"])], "Comment ça marche"),
 "es": ("¿Cómo funciona PC Migrate Portable?", "Pasa archivos, programas y ajustes a un PC nuevo en 3 pasos — sin instalación, sin Internet",
        [("PC antiguo", ["Arranca el USB de Hiren's (o ejecútalo en Windows)", "Pestaña <b>Copia</b>: usuarios y programas", "<b>Iniciar copia</b>"]),
         ("Paquete de migración", ["Una sola carpeta en un disco externo o USB", "Archivos, navegadores, ajustes, Wi-Fi", "Programas con sus archivos y datos"]),
         ("PC nuevo", ["Pestaña <b>Restaurar</b>: <b>Iniciar restauración</b>", "<b>Instalar / transferir</b> los programas", "Reinicia el PC ✓"])], "Cómo funciona"),
}
def workflow(lang):
    title, sub, cs, kick = T[lang]
    st = A.workflow(); st["kicker"] = kick
    card = lambda icon, n, t, lines: A.card(icon, n, t, lines).replace("right:28px", "left:28px").replace("padding-right:20px;text-align:right", "padding-left:20px;text-align:left")
    arrow = A.ARROW.replace("<svg ", '<svg style="transform:scaleX(-1)" ')
    cards = card(A.PC_OLD, 1, *cs[0]) + arrow + card(A.DISK, 2, *cs[1]) + arrow + card(A.PC_NEW, 3, *cs[2])
    st["panel"] = f'''<div class="panel" style="left:0;top:150px;width:1620px;text-align:center;direction:ltr">
       <h1 style="font-size:40px;font-family:Inter">{title}</h1><div class="sub" style="font-size:18px">{sub}</div>
       <div style="display:flex;direction:ltr;justify-content:center;align-items:center;gap:22px;margin-top:80px">{cards}</div></div>''' + A.FOOT
    return st
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(executable_path="/usr/bin/google-chrome", args=["--no-sandbox"])
        pg = await b.new_page(viewport={"width": 1620, "height": 900}, device_scale_factor=2)
        await pg.goto("file:///home/claude/media/mock.html"); await pg.wait_for_timeout(400)
        for lang in ("fr", "es"):
            os.makedirs(f"out/img/{lang}", exist_ok=True)
            await pg.evaluate("s=>render(s)", workflow(lang)); await pg.wait_for_timeout(250)
            await pg.locator(".stage-annot").screenshot(path=f"out/img/{lang}/00-how-it-works.png"); print("ok", lang)
        await b.close()
asyncio.run(main())
