# CrackSensor OneDrive Image Viewer

A Windows desktop application for viewing and managing crack sensor images stored in OneDrive with an intuitive folder navigation interface.

## Overview

This application provides a user-friendly GUI for accessing images from the CrackSensor system stored in OneDrive. It features:

- **Microsoft OAuth Authentication** - Secure sign-in with your Microsoft account
- **Hierarchical Folder Navigation** - Browse by Location → Floor → Camera
- **Image Thumbnail Grid** - Quick preview of all camera images
- **Full-Size Image Viewer** - Detailed view with metadata
- **Sorting & Filtering** - Sort by date or filename
- **Automatic Caching** - Fast loading with thumbnail caching
- **Update Function** - Refresh data from OneDrive with one click

## Quick Start

### For End Users

1. **Download** the application package
2. **Extract** all files to a folder
3. **Run** `CrackSensorViewer.exe`
4. **Sign in** when prompted in your browser
5. **Navigate** using the folder tree on the left
6. **Click** any image to view full size

### For Developers

```bash
# Install dependencies
pip install -r requirements.txt

# Run the application
python image_viewer_app.py

# Build executable
pyinstaller CrackSensorViewer.spec
```

## Documentation

- **[README_SETUP.md](README_SETUP.md)** - Azure configuration and authentication setup
- **[USER_GUIDE.md](USER_GUIDE.md)** - Complete user guide with screenshots
- **[BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md)** - Building and distributing the executable

## Features

### Authentication
- OAuth 2.0 device code flow
- Secure token storage
- Automatic token refresh
- Multi-account support

### Navigation
- **Tree View**: PVEDI → Location → Floor → Camera
- Collapsible folder hierarchy
- Quick camera switching
- Status updates during loading

### Image Display
- **Grid View**: 4-column thumbnail layout
- 150x150 pixel thumbnails
- Filename and date display
- Lazy loading for performance

### Full-Size Viewer
- High-resolution image display
- Navigation arrows (Previous/Next)
- Keyboard shortcuts (←/→)
- Metadata panel with details

### Sorting
- Date (Newest first)
- Date (Oldest first)
- Filename (A-Z)
- Filename (Z-A)

### Caching
- Thumbnail cache for fast loading
- Memory cache for recently viewed
- Automatic cache management
- Clear cache with Update button

## System Requirements

- **Operating System**: Windows 10 or Windows 11 (64-bit)
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk Space**: 100 MB for application + cache
- **Internet**: Required for OneDrive access
- **Display**: 1280x800 resolution or higher

## Architecture

### Components

1. **Config Manager** - Loads and validates configuration
2. **Auth Manager** - Handles Microsoft OAuth authentication
3. **OneDrive Client** - Interfaces with Microsoft Graph API
4. **Folder Navigator** - Builds and manages folder tree structure
5. **Image Cache** - Downloads and caches thumbnails/images
6. **Image Viewer** - Full-size image display window
7. **Main GUI** - Primary application window with tree and grid

### Data Flow

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   User      │────▶│ Application  │────▶│  Microsoft  │
│             │◀────│              │◀────│  Graph API  │
└─────────────┘     └──────────────┘     └─────────────┘
                            │
                            ▼
                    ┌──────────────┐
                    │ Local Cache  │
                    │  (AppData)   │
                    └──────────────┘
```

### Folder Structure

```
PVEDI/
├── BryantPark/
│   ├── Floor1/
│   │   ├── A_T1/
│   │   │   ├── 71318.jpg
│   │   │   ├── 71318.txt
│   │   │   └── ...
│   │   └── A_T2/
│   └── Floor2/
└── Site_Alpha/
    └── ...
```

## Configuration

### config.json

```json
{
  "client_id": "YOUR_AZURE_CLIENT_ID",
  "tenant_id": "common",
  "authority": "https://login.microsoftonline.com/common",
  "scope": ["Files.Read", "Files.Read.All", "User.Read"],
  "share_link": "https://1drv.ms/f/...",
  "cache_expiry_minutes": 15
}
```

### Parameters

- **client_id**: Azure AD application client ID
- **tenant_id**: Azure AD tenant ID or "common" for multi-tenant
- **authority**: Microsoft login endpoint
- **scope**: Required Microsoft Graph permissions
- **share_link**: OneDrive shared folder URL
- **cache_expiry_minutes**: Cache lifetime (not currently enforced)

## Security

### Authentication
- Industry-standard OAuth 2.0
- No password storage
- Encrypted token cache
- Automatic token refresh

### Permissions
- **Read-only**: Cannot modify or delete files
- **User consent**: User must explicitly grant access
- **Revocable**: Access can be removed anytime

### Data Privacy
- Images viewed from cloud (not permanently stored locally)
- Thumbnails cached temporarily in AppData
- Cache cleared on Update or application close
- No telemetry or tracking

## Building from Source

### Prerequisites

```bash
pip install msal requests Pillow pyinstaller
```

### Build Steps

1. **Test the application**:
   ```bash
   python image_viewer_app.py
   ```

2. **Build executable**:
   ```bash
   pyinstaller CrackSensorViewer.spec
   ```

3. **Find executable**:
   ```
   dist/CrackSensorViewer.exe
   ```

4. **Test executable**:
   - Copy `config.json` to `dist/` folder
   - Run `dist/CrackSensorViewer.exe`

### Build Customization

Edit `CrackSensorViewer.spec`:
- Add icon: `icon='icon.ico'`
- Show console: `console=True`
- Add data files: `datas=[('icon.png', '.')]`

## Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| Can't authenticate | Check internet connection, verify config.json |
| Folders not loading | Click Update, check OneDrive access |
| Images not appearing | Select a camera in tree, check internet |
| Slow performance | Normal for first load, cached afterwards |
| Missing config.json | Create config.json in same folder as exe |

### Debug Mode

Run with console output:
```bash
python image_viewer_app.py
```

Check logs in console for detailed error messages.

### Cache Location

```
Windows: %APPDATA%\CrackSensorViewer\
```

To clear manually:
1. Close application
2. Delete the folder above
3. Restart application

## Development

### Project Structure

```
OneDrive/
├── image_viewer_app.py          # Main application
├── config.json                   # Configuration
├── requirements.txt              # Dependencies
├── CrackSensorViewer.spec       # PyInstaller spec
├── README.md                     # This file
├── README_SETUP.md              # Setup instructions
├── USER_GUIDE.md                # User documentation
└── BUILD_INSTRUCTIONS.md        # Build guide
```

### Dependencies

- **msal** (>=1.24.0) - Microsoft authentication
- **requests** (>=2.31.0) - HTTP requests
- **Pillow** (>=10.0.0) - Image processing
- **tkinter** - GUI framework (built-in)
- **pyinstaller** (>=6.0.0) - Executable builder

### Code Organization

- **Classes**:
  - `Config` - Configuration management
  - `AuthManager` - OAuth authentication
  - `OneDriveClient` - API client
  - `FolderNavigator` - Folder structure
  - `ImageCache` - Image caching
  - `ImageViewerWindow` - Full-size viewer
  - `CrackSensorViewer` - Main application

- **Threading**: Background loading for responsiveness
- **Caching**: Multi-level (memory + disk)
- **Error Handling**: Graceful failures with user feedback

## Contributing

### Code Style
- PEP 8 compliant
- Type hints where applicable
- Docstrings for classes and methods

### Testing
- Test on Windows 10 and 11
- Test with various image counts
- Test authentication flow
- Test error scenarios

## License

Internal use only. Not for public distribution.

## Support

For issues or questions:
1. Check USER_GUIDE.md
2. Check README_SETUP.md
3. Contact system administrator

## Acknowledgments

- Microsoft Graph API for OneDrive access
- MSAL library for authentication
- Pillow for image processing
- PyInstaller for executable packaging

## Version History

### 1.0.0 (2025)
- Initial release
- OAuth authentication
- Folder navigation
- Image viewing
- Thumbnail caching
- Sort functionality
- Update mechanism

## Future Enhancements

Potential future features:
- Search functionality
- Date range filtering
- Bulk download
- Image comparison
- Annotation support
- Export reports
- Multiple OneDrive sources
- Dark mode theme

## Contact

For technical support or feature requests, contact the CrackSensor development team.

---

**CrackSensor Image Viewer** - Version 1.0.0

