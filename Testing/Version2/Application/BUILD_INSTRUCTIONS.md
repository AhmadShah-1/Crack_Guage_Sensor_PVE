# Building CrackSensor Image Viewer Executable

## Prerequisites

- Python 3.8 or higher
- Windows 10/11 (for building Windows executable)
- All dependencies installed

## Step 1: Install Dependencies

```bash
pip install -r requirements.txt
```

## Step 2: Test the Application

Before building, test that the application works:

```bash
python image_viewer_app.py
```

Make sure you have `config.json` in the same directory.

## Step 3: Build the Executable

### Option A: Using the Spec File (Recommended)

```bash
pyinstaller CrackSensorViewer.spec
```

### Option B: Using Command Line

```bash
pyinstaller --onefile --windowed --name="CrackSensorViewer" ^
    --hidden-import=msal ^
    --hidden-import=PIL._tkinter_finder ^
    --hidden-import=requests ^
    image_viewer_app.py
```

## Step 4: Locate the Executable

After building, the executable will be in:
```
dist/CrackSensorViewer.exe
```

## Step 5: Prepare for Distribution

1. Create a distribution folder:
```
CrackSensorViewer/
├── CrackSensorViewer.exe
├── config.json
└── README_SETUP.md
```

2. Copy files:
```bash
mkdir CrackSensorViewer
copy dist\CrackSensorViewer.exe CrackSensorViewer\
copy config.json CrackSensorViewer\
copy README_SETUP.md CrackSensorViewer\
```

3. Create a ZIP file for distribution

## Step 6: First-Time User Setup

Users should:

1. Extract all files to a folder
2. Ensure `config.json` is in the same folder as the .exe
3. Double-click `CrackSensorViewer.exe`
4. Follow the authentication prompts in the browser
5. Wait for the folder structure to load

## Troubleshooting Build Issues

### "Module not found" errors

Add missing modules to `hiddenimports` in the spec file:
```python
hiddenimports=[
    'msal',
    'your_missing_module',
],
```

### Executable is too large

Remove UPX compression in the spec file:
```python
upx=False,
```

### Console appears when running

Change in spec file:
```python
console=False,  # No console window
```

### Missing DLL errors

Install Visual C++ Redistributables on the target machine:
https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads

## Development vs Production

### Development (Testing):
```bash
python image_viewer_app.py
```
- Faster iteration
- See console output for debugging
- Can modify code easily

### Production (Distribution):
```bash
CrackSensorViewer.exe
```
- Single file distribution
- No Python required
- Professional appearance

## File Size Optimization

The executable will be approximately 40-60 MB due to bundled dependencies.

To reduce size:
1. Use `--onefile` instead of `--onedir`
2. Disable UPX: `upx=False`
3. Exclude unnecessary modules
4. Use virtual environment with minimal packages

## Testing Checklist

Before distributing:

- [ ] Executable runs on clean Windows machine (no Python installed)
- [ ] Authentication flow works correctly
- [ ] Folder structure loads
- [ ] Images display correctly
- [ ] Thumbnails load
- [ ] Full-size viewer works
- [ ] Sort functionality works
- [ ] Update button refreshes data
- [ ] No console window appears (if console=False)
- [ ] Error messages display properly

## Common Distribution Issues

### Antivirus False Positives

PyInstaller executables may trigger antivirus software. Solutions:
1. Sign the executable with a code signing certificate
2. Submit to antivirus vendors for whitelisting
3. Distribute source code + instructions instead

### Missing config.json

The application will show an error if config.json is missing.
Always include it in the distribution package.

### OneDrive Access Issues

Ensure users have:
1. Microsoft account
2. Access to the shared OneDrive folder
3. Internet connection
4. No corporate firewall blocking Microsoft Graph API

## Building for Different Platforms

### Windows
```bash
pyinstaller CrackSensorViewer.spec
```

### Note on Cross-Platform

PyInstaller creates executables for the platform it runs on:
- Build on Windows → Windows .exe
- Build on macOS → macOS app
- Build on Linux → Linux binary

You cannot cross-compile. Build on the target platform.

## Automated Build Script (Optional)

Create `build.bat`:

```batch
@echo off
echo Building CrackSensor Image Viewer...
rmdir /s /q build dist
pyinstaller CrackSensorViewer.spec
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Executable location: dist\CrackSensorViewer.exe
) else (
    echo Build failed!
)
pause
```

Run with:
```bash
build.bat
```

## Version Information

To add version info to the executable, create `version_info.txt`:

```python
VSVersionInfo(
  ffi=FixedFileInfo(
    filevers=(1, 0, 0, 0),
    prodvers=(1, 0, 0, 0),
    mask=0x3f,
    flags=0x0,
    OS=0x40004,
    fileType=0x1,
    subtype=0x0,
    date=(0, 0)
  ),
  kids=[
    StringFileInfo(
      [
      StringTable(
        u'040904B0',
        [StringStruct(u'CompanyName', u'CrackSensor Team'),
        StringStruct(u'FileDescription', u'OneDrive Image Viewer'),
        StringStruct(u'FileVersion', u'1.0.0.0'),
        StringStruct(u'ProductName', u'CrackSensor Image Viewer'),
        StringStruct(u'ProductVersion', u'1.0.0.0')])
      ]), 
    VarFileInfo([VarStruct(u'Translation', [1033, 1200])])
  ]
)
```

Then add to build command:
```bash
pyinstaller --version-file=version_info.txt CrackSensorViewer.spec
```

