<p align="center">
  <img src="docs/media/en/00-how-it-works.png" alt="PC Migrate Portable" width="100%">
</p>

<h1 align="center">PC Migrate Portable</h1>

<p align="center">
  <b>Move your old PC to a new one — files, browsers, Windows settings and installed programs with their data.</b><br>
  Portable · Runs from Hiren's BootCD PE · English · Français · العربية · Español
</p>

<p align="center">
  <a href="#english"><img src="https://img.shields.io/badge/English-0e5a8a?style=for-the-badge" alt="English"></a>
  <a href="#francais"><img src="https://img.shields.io/badge/Français-1f6fd1?style=for-the-badge" alt="Français"></a>
  <a href="#arabic"><img src="https://img.shields.io/badge/العربية-ff9a1f?style=for-the-badge" alt="العربية"></a>
  <a href="#espanol"><img src="https://img.shields.io/badge/Español-c0392b?style=for-the-badge" alt="Español"></a>
  <a href="https://yacineboulyali.github.io/pcmigrate-portable/"><img src="https://img.shields.io/badge/🌐_Website-555?style=for-the-badge" alt="Website"></a>
</p>

<p align="center">
  <a href="https://github.com/yacineboulyali/pcmigrate-portable/releases/latest"><img src="https://img.shields.io/github/v/release/yacineboulyali/pcmigrate-portable?label=release&color=22c55e" alt="release"></a>
  <a href="https://github.com/yacineboulyali/pcmigrate-portable/actions"><img src="https://github.com/yacineboulyali/pcmigrate-portable/actions/workflows/build.yml/badge.svg" alt="build"></a>
  <img src="https://img.shields.io/badge/Windows-10%20%7C%2011%20%7C%20WinPE-0078d7" alt="windows">
  <img src="https://img.shields.io/badge/license-MIT-lightgrey" alt="license">
</p>

<p align="center">
  <a href="https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup.exe"><b>⬇ PCMigrate-Setup.exe</b></a> &nbsp;·&nbsp;
  <a href="https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable.zip"><b>⬇ PCMigrate-Portable.zip</b></a> &nbsp;·&nbsp;
  <a href="https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate.exe">PCMigrate.exe</a>
</p>

---

<a id="english"></a>
<details open>
<summary><h2>🇬🇧 English</h2></summary>

**PC Migrate Portable** is a free, open-source tool that moves everything from an old Windows PC to a new one — like PCmover, but portable and free. It is a single ~500 KB executable with no runtime to install, and it also runs from a **Hiren's BootCD PE** USB stick, even when the old Windows no longer boots.

### ✨ Features
- **User files** — Desktop, Documents, Pictures, Downloads, Music, Videos… (OneDrive folders included)
- **Browsers** — Chrome, Edge, Brave, Opera and Firefox profiles: bookmarks, history, extensions
- **Windows settings** — mouse, keyboard & languages, regional format, dark mode, wallpaper, Quick Access, **Wi-Fi**
- **Programs with their data** — program files, registry, services, Start-menu shortcuts and each user's AppData
- **Clean reinstall with winget** — or "winget + data": clean install, then the program's settings are restored
- **4 interface languages** — English, Français, العربية (right-to-left), Español — change it any time in **Settings**

### ⬇️ Download
| File | Use |
|---|---|
| [**PCMigrate-Setup.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup.exe) | Installer (Start-menu shortcut, uninstaller). The installer language becomes the app language. |
| [**PCMigrate-Portable.zip**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable.zip) | For a USB stick / Hiren's: `PCMigrate.exe` + `Run-PCMigrate.bat` + `Copy-To-USB.bat` |

> ⚠️ Not digitally signed: if SmartScreen warns you, click **More info → Run anyway**.

### 🎬 How it works
**1. Detects the old Windows and its users — choose what to move**<br>
<img src="docs/media/en/01-detect-and-select.gif" width="720" alt="Detect and select">

**2. Choose programs to move with their files and data**<br>
<img src="docs/media/en/02-choose-programs.gif" width="720" alt="Choose programs">

**3. Start the backup to an external disk**<br>
<img src="docs/media/en/03-start-backup.gif" width="720" alt="Start backup">

**4. On the new PC: restore files, browsers and settings**<br>
<img src="docs/media/en/04-restore-files.gif" width="720" alt="Restore">

**5. Transfer or install programs in one click, then restart**<br>
<img src="docs/media/en/05-transfer-programs.gif" width="720" alt="Transfer programs">

### 🖱️ Every button explained
<img src="docs/media/en/01-backup.png" width="100%" alt="Backup tab">
<img src="docs/media/en/02-program-chooser.png" width="100%" alt="Program chooser">
<img src="docs/media/en/03-restore.png" width="100%" alt="Restore tab">

### ⚙️ Settings & language
Click **Settings** (top-right) to switch between English, Français, العربية and Español. The window reloads instantly; the choice is saved in `PCMigrate.ini` next to the program. You can also start it with `PCMigrate.exe --lang=fr` (`en`, `fr`, `ar`, `es`).

### ⚠️ Good to know
- Chrome / Edge / Brave / Opera **passwords and cookies** are encrypted per PC — use browser sync. Firefox moves completely.
- Antivirus, VPN, drivers and Office / Adobe licences usually need a reinstall.
- Wi-Fi passwords move when the backup is taken from the running Windows; from Hiren's only network names move.
- Try it in a virtual machine first, and use an NTFS or exFAT disk.

</details>

<a id="francais"></a>
<details>
<summary><h2>🇫🇷 Français</h2></summary>

**PC Migrate Portable** est un outil gratuit et open source qui transfère tout le contenu d'un ancien PC Windows vers un nouveau — comme PCmover, mais portable et gratuit. Un seul exécutable d'environ 500 Ko, sans rien à installer, qui fonctionne aussi depuis une clé **Hiren's BootCD PE**, même si l'ancien Windows ne démarre plus.

### ✨ Fonctionnalités
- **Fichiers** — Bureau, Documents, Images, Téléchargements, Musique, Vidéos… (dossiers OneDrive compris)
- **Navigateurs** — profils Chrome, Edge, Brave, Opera et Firefox : favoris, historique, extensions
- **Paramètres Windows** — souris, clavier et langues, format régional, mode sombre, fond d'écran, Accès rapide, **Wi-Fi**
- **Programmes avec leurs données** — fichiers du programme, registre, services, raccourcis et AppData de chaque utilisateur
- **Réinstallation propre via winget** — ou « winget + données » : installation propre puis restauration des réglages du programme
- **4 langues d'interface** — English, Français, العربية, Español — modifiable à tout moment dans **Paramètres**

### ⬇️ Téléchargement
| Fichier | Utilisation |
|---|---|
| [**PCMigrate-Setup.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup.exe) | Installateur (raccourcis, désinstallation). La langue de l'installateur devient celle du programme. |
| [**PCMigrate-Portable.zip**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable.zip) | Pour clé USB / Hiren's : `PCMigrate.exe` + `Run-PCMigrate.bat` + `Copy-To-USB.bat` |

> ⚠️ Non signé numériquement : si SmartScreen vous avertit, cliquez sur **Informations complémentaires → Exécuter quand même**.

<img src="docs/media/fr/00-how-it-works.png" width="100%" alt="Comment ça marche">

### 🎬 Fonctionnement étape par étape
**1. Détecte l'ancien Windows et ses utilisateurs — choisissez quoi transférer**<br>
<img src="docs/media/fr/01-detect-and-select.gif" width="720" alt="Détection">

**2. Choisissez les programmes à déplacer avec leurs fichiers et données**<br>
<img src="docs/media/fr/02-choose-programs.gif" width="720" alt="Choix des programmes">

**3. Lancez la sauvegarde vers un disque externe**<br>
<img src="docs/media/fr/03-start-backup.gif" width="720" alt="Sauvegarde">

**4. Sur le nouveau PC : restaurez fichiers, navigateurs et paramètres**<br>
<img src="docs/media/fr/04-restore-files.gif" width="720" alt="Restauration">

**5. Transférez ou installez les programmes en un clic, puis redémarrez**<br>
<img src="docs/media/fr/05-transfer-programs.gif" width="720" alt="Programmes">

### ⚙️ Paramètres et langue
Cliquez sur **Paramètres** (en haut à droite) pour choisir English, Français, العربية ou Español. La fenêtre se recharge immédiatement ; le choix est enregistré dans `PCMigrate.ini` à côté du programme. Option en ligne de commande : `PCMigrate.exe --lang=fr`.

### ⚠️ À savoir
- Les **mots de passe et cookies** de Chrome / Edge / Brave / Opera sont chiffrés par PC : utilisez la synchronisation du navigateur. Firefox est transféré entièrement.
- Antivirus, VPN, pilotes et licences Office / Adobe demandent généralement une réinstallation.
- Les mots de passe Wi-Fi sont transférés si la sauvegarde est faite depuis Windows démarré ; depuis Hiren's, seuls les noms des réseaux le sont.
- Testez d'abord dans une machine virtuelle et utilisez un disque NTFS ou exFAT.

</details>

<a id="arabic"></a>
<details>
<summary><h2>🌍 العربية</h2></summary>

<div dir="rtl">

**PC Migrate Portable** أداة مجانية ومفتوحة المصدر تنقل كل شيء من حاسوب ويندوز قديم إلى حاسوب جديد، على طريقة PCmover لكنها محمولة ومجانية. ملف واحد حجمه حوالي 500 KB لا يحتاج تثبيت أي مكتبة، ويعمل أيضًا من فلاشة **Hiren's BootCD PE** حتى لو كان ويندوز القديم لا يُقلع.

### ✨ المميزات
- **ملفات المستخدم:** سطح المكتب، المستندات، الصور، التنزيلات، الموسيقى، الفيديو... مع مجلدات OneDrive
- **المتصفحات:** Chrome و Edge و Brave و Opera و Firefox: المفضلة والسجل والإضافات
- **إعدادات ويندوز:** الماوس، لوحة المفاتيح واللغات، الإعدادات الإقليمية، الوضع الداكن، الخلفية، الوصول السريع، **Wi-Fi**
- **البرامج مع بياناتها:** ملفات البرنامج، السجل (Registry)، الخدمات، الاختصارات، وبيانات كل مستخدم
- **إعادة تثبيت نظيفة عبر winget:** أو «winget + البيانات»، أي تثبيت نظيف ثم استرجاع إعدادات البرنامج
- **4 لغات للواجهة:** English و Français و العربية (من اليمين إلى اليسار) و Español، ويمكن تغييرها في أي وقت من **الإعدادات**

### ⬇️ التحميل
| الملف | الاستعمال |
|---|---|
| [**PCMigrate-Setup.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup.exe) | ملف تثبيت (اختصارات وإمكانية الإزالة)، ولغة التثبيت تصبح لغة البرنامج |
| [**PCMigrate-Portable.zip**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable.zip) | للفلاشة و Hiren's: `PCMigrate.exe` و `Run-PCMigrate.bat` و `Copy-To-USB.bat` |

> ⚠️ البرنامج غير موقّع رقميًا، فإذا ظهر تحذير SmartScreen اضغط **More info** ثم **Run anyway**.

<img src="docs/media/ar/00-how-it-works.png" width="100%" alt="كيف يعمل">

### 🎬 طريقة العمل خطوة بخطوة
**1. يكتشف ويندوز القديم والمستخدمين، فتختار ما تنقله**<br>
<img src="docs/media/ar/01-detect-and-select.gif" width="720" alt="الاكتشاف">

**2. اختر البرامج التي تُنقل مع ملفاتها وبياناتها**<br>
<img src="docs/media/ar/02-choose-programs.gif" width="720" alt="اختيار البرامج">

**3. ابدأ النسخ إلى قرص خارجي**<br>
<img src="docs/media/ar/03-start-backup.gif" width="720" alt="النسخ">

**4. على الجهاز الجديد: استعد الملفات والمتصفحات والإعدادات**<br>
<img src="docs/media/ar/04-restore-files.gif" width="720" alt="الاستعادة">

**5. انقل البرامج أو ثبّتها بنقرة واحدة، ثم أعد تشغيل الجهاز**<br>
<img src="docs/media/ar/05-transfer-programs.gif" width="720" alt="البرامج">

### 🖱️ شرح كل الأزرار
<img src="docs/media/ar/01-backup.png" width="100%" alt="تبويب النسخ">
<img src="docs/media/ar/02-program-chooser.png" width="100%" alt="اختيار البرامج">
<img src="docs/media/ar/03-restore.png" width="100%" alt="تبويب الاستعادة">

### ⚙️ الإعدادات واللغة
اضغط زر **الإعدادات** في أعلى النافذة لتختار English أو Français أو العربية أو Español. تُعاد تهيئة النافذة فورًا، ويُحفظ الاختيار في ملف `PCMigrate.ini` بجانب البرنامج. ويمكن أيضًا تشغيله بالأمر `PCMigrate.exe --lang=ar`.

### ⚠️ حدود يجب معرفتها
- **كلمات المرور والكوكيز** في Chrome و Edge و Brave و Opera مشفّرة لكل جهاز، فاستعمل المزامنة. أما Firefox فينتقل بالكامل.
- برامج الحماية و VPN والتعريفات وتراخيص Office و Adobe تحتاج غالبًا إعادة تثبيت.
- كلمات مرور Wi-Fi تنتقل إذا أُخذت النسخة من ويندوز وهو يعمل، أما من Hiren's فتنتقل أسماء الشبكات فقط.
- جرّب البرنامج أولًا على جهاز وهمي (VM)، واستعمل قرصًا بصيغة NTFS أو exFAT.

</div>
</details>

<a id="espanol"></a>
<details>
<summary><h2>🇪🇸 Español</h2></summary>

**PC Migrate Portable** es una herramienta gratuita y de código abierto que pasa todo de un PC con Windows antiguo a uno nuevo — como PCmover, pero portátil y gratis. Un solo ejecutable de unos 500 KB, sin nada que instalar, que también funciona desde un USB de **Hiren's BootCD PE**, incluso si el Windows antiguo ya no arranca.

### ✨ Funciones
- **Archivos** — Escritorio, Documentos, Imágenes, Descargas, Música, Vídeos… (incluidas las carpetas de OneDrive)
- **Navegadores** — perfiles de Chrome, Edge, Brave, Opera y Firefox: marcadores, historial, extensiones
- **Ajustes de Windows** — ratón, teclado e idiomas, formato regional, modo oscuro, fondo, Acceso rápido, **Wi-Fi**
- **Programas con sus datos** — archivos del programa, registro, servicios, accesos directos y AppData de cada usuario
- **Reinstalación limpia con winget** — o «winget + datos»: instalación limpia y luego se restauran los ajustes del programa
- **4 idiomas de interfaz** — English, Français, العربية, Español — se puede cambiar en cualquier momento en **Ajustes**

### ⬇️ Descarga
| Archivo | Uso |
|---|---|
| [**PCMigrate-Setup.exe**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Setup.exe) | Instalador (accesos directos, desinstalador). El idioma del instalador pasa a ser el del programa. |
| [**PCMigrate-Portable.zip**](https://github.com/yacineboulyali/pcmigrate-portable/releases/latest/download/PCMigrate-Portable.zip) | Para USB / Hiren's: `PCMigrate.exe` + `Run-PCMigrate.bat` + `Copy-To-USB.bat` |

> ⚠️ Sin firma digital: si SmartScreen te avisa, pulsa **Más información → Ejecutar de todas formas**.

<img src="docs/media/es/00-how-it-works.png" width="100%" alt="Cómo funciona">

### 🎬 Cómo funciona, paso a paso
**1. Detecta el Windows antiguo y sus usuarios — elige qué transferir**<br>
<img src="docs/media/es/01-detect-and-select.gif" width="720" alt="Detección">

**2. Elige los programas a mover con sus archivos y datos**<br>
<img src="docs/media/es/02-choose-programs.gif" width="720" alt="Elegir programas">

**3. Inicia la copia a un disco externo**<br>
<img src="docs/media/es/03-start-backup.gif" width="720" alt="Copia">

**4. En el PC nuevo: restaura archivos, navegadores y ajustes**<br>
<img src="docs/media/es/04-restore-files.gif" width="720" alt="Restauración">

**5. Transfiere o instala los programas con un clic y reinicia**<br>
<img src="docs/media/es/05-transfer-programs.gif" width="720" alt="Programas">

### ⚙️ Ajustes e idioma
Pulsa **Ajustes** (arriba a la derecha) para elegir English, Français, العربية o Español. La ventana se recarga al instante; la elección se guarda en `PCMigrate.ini` junto al programa. También puedes iniciarlo con `PCMigrate.exe --lang=es`.

### ⚠️ Conviene saber
- Las **contraseñas y cookies** de Chrome / Edge / Brave / Opera están cifradas por PC: usa la sincronización del navegador. Firefox se transfiere por completo.
- Antivirus, VPN, controladores y licencias de Office / Adobe suelen requerir reinstalación.
- Las contraseñas Wi-Fi se transfieren si la copia se hace desde Windows en ejecución; desde Hiren's solo se pasan los nombres de las redes.
- Pruébalo primero en una máquina virtual y usa un disco NTFS o exFAT.

</details>

---

## 🛠️ Build from source

```sh
sudo apt install g++-mingw-w64-x86-64 nsis zip
python3 tools/i18n/gen_strings.py   # translations -> src/strings.h
./build.sh                          # build/PCMigrate.exe
./package.sh                        # dist/: PCMigrate-Setup.exe + PCMigrate-Portable.zip
```

Translations live in [`tools/i18n/strings.py`](tools/i18n/strings.py) (English, French, Arabic, Spanish). Every push to `main` is built by GitHub Actions and published as release `v<VERSION>`.

<p align="center">MIT License © 2026 Yacine Boulyali</p>
