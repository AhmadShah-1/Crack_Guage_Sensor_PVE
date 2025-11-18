# CrackSensor Image Viewer - User Guide

## Quick Start

1. **Launch the Application**
   - Double-click `CrackSensorViewer.exe`
   - A browser window will open for authentication

2. **Sign In**
   - Sign in with your Microsoft account
   - Grant permissions when prompted
   - Return to the application

3. **Wait for Loading**
   - The application will load the folder structure
   - This may take 30-60 seconds depending on the number of images
   - Progress is shown in the status bar

4. **Navigate and View Images**
   - Use the folder tree on the left to navigate
   - Select a camera to view its images
   - Click any image thumbnail to view full size

## Interface Overview

### Main Window Layout

```
┌─────────────────────────────────────────────────────┐
│ [🔄 Update]  [Sort: Date▼]              Status      │
├──────────────┬──────────────────────────────────────┤
│              │                                       │
│  Folder Tree │       Image Thumbnails                │
│              │                                       │
│  📁 PVEDI    │   [img] [img] [img] [img]            │
│    📁 Site1  │   [img] [img] [img] [img]            │
│      📁 Fl1  │   [img] [img] [img] [img]            │
│        📷 C1 │                                       │
│        📷 C2 │                                       │
│                                                      │
└──────────────┴──────────────────────────────────────┘
```

### Toolbar

- **🔄 Update**: Refresh data from OneDrive (clears cache and reloads)
- **Sort dropdown**: Change how images are sorted
- **Status bar**: Shows current operation and image count

### Folder Tree (Left Panel)

- **Structure**: PVEDI → Location → Floor → Camera
- **Click to expand**: Click the triangle (▶) to expand folders
- **Select camera**: Click a camera name to view its images

### Image Grid (Right Panel)

- **Thumbnails**: 150x150 pixel previews
- **Information**: Shows filename and date below each image
- **Click to view**: Click any thumbnail for full-size view
- **Scroll**: Use mouse wheel or scrollbar to navigate

## Viewing Images

### Full-Size Viewer

When you click an image thumbnail:

1. **Image Display**
   - Full-size image in center
   - Scales to fit window
   - Maintains aspect ratio

2. **Navigation**
   - **◀ Previous**: View previous image
   - **Next ▶**: View next image
   - **Arrow Keys**: ← → to navigate
   - **Escape**: Close viewer

3. **Metadata Panel** (Right side)
   - Location, Floor, Camera
   - Filename and file size
   - Date modified
   - Contents of .txt metadata file (if available)

### Keyboard Shortcuts

- **Left Arrow**: Previous image
- **Right Arrow**: Next image
- **Escape**: Close image viewer

## Sorting Images

Click the **Sort** dropdown to change order:

- **Date (Newest)**: Most recent images first (default)
- **Date (Oldest)**: Oldest images first
- **Filename (A-Z)**: Alphabetical order
- **Filename (Z-A)**: Reverse alphabetical

Sorting applies to the currently selected camera.

## Updating Data

Click the **🔄 Update** button to:

- Clear local image cache
- Reload folder structure from OneDrive
- Refresh with latest images

**When to update:**
- New images have been uploaded
- Images have been deleted
- Folder structure has changed

**Note:** Update may take 30-60 seconds.

## Understanding the Folder Structure

### Hierarchy

```
PVEDI (Root)
└── Location (e.g., "BryantPark", "Site_Alpha")
    └── Floor (e.g., "Floor1", "Basement")
        └── Camera (e.g., "A_T1", "Camera_01")
            ├── image1.jpg
            ├── image1.txt (metadata)
            ├── image2.jpg
            └── image2.txt (metadata)
```

### Metadata Files

- Each `.jpg` image may have a corresponding `.txt` file
- Contains: Site, Camera, Subreceiver, Timestamp, Size
- Automatically displayed in the full-size viewer

## Troubleshooting

### Authentication Issues

**Problem**: Cannot sign in
- **Solution**: Make sure you have a valid Microsoft account with access to the shared OneDrive

**Problem**: "Permission denied" error
- **Solution**: Contact administrator to grant access to the OneDrive folder

**Problem**: Browser doesn't open
- **Solution**: Manually open the URL shown in the console

### Loading Issues

**Problem**: Folder structure doesn't load
- **Solution**: Check internet connection
- **Solution**: Click Update button to retry
- **Solution**: Restart application

**Problem**: Images not appearing
- **Solution**: Select a camera in the folder tree
- **Solution**: Click Update to refresh
- **Solution**: Check that the camera folder contains images

### Display Issues

**Problem**: Thumbnails show "Loading..." forever
- **Solution**: Check internet connection
- **Solution**: Image file may be corrupted on OneDrive
- **Solution**: Click Update to clear cache

**Problem**: Full-size image won't open
- **Solution**: Try another image
- **Solution**: Check internet connection
- **Solution**: Image file may be corrupted

### Performance Issues

**Problem**: Application is slow
- **Solution**: Large number of images may take time to load
- **Solution**: Thumbnails are cached for faster subsequent viewing
- **Solution**: Restart application to clear memory

**Problem**: High memory usage
- **Solution**: Normal for image viewing applications
- **Solution**: Restart application if it becomes sluggish
- **Solution**: Close and reopen specific camera views

## Tips & Best Practices

### Efficient Navigation

1. **Collapse folders** you're not using to keep tree organized
2. **Remember sort preference** - sorting is remembered per session
3. **Use keyboard shortcuts** for faster image navigation

### Managing Cache

- Cache stored in: `%APPDATA%\CrackSensorViewer\cache\`
- Cleared automatically when clicking Update
- Manual clearing: Delete the cache folder above

### Multiple Cameras

- You can quickly switch between cameras using the tree
- Previously viewed thumbnails are cached for speed
- Each camera maintains its own sort order during session

### Metadata

- Always check metadata panel for important details
- Timestamp helps identify when image was captured
- Size information useful for quality verification

## Advanced Features

### Caching System

- **Thumbnail Cache**: 150x150 previews cached locally
- **Memory Cache**: Recently viewed images kept in RAM
- **Automatic**: No user action needed
- **Cleared on Update**: Ensures fresh data

### Multi-threading

- **Background Loading**: Images load without freezing UI
- **Parallel Downloads**: Multiple thumbnails load simultaneously
- **Responsive**: Can navigate while images are loading

## Privacy & Security

### Data Storage

- **No image data** stored permanently on local machine
- **Cache only**: Temporary thumbnails in AppData folder
- **Token storage**: Authentication tokens encrypted locally
- **OneDrive only**: All permanent data remains on OneDrive

### Authentication

- **OAuth 2.0**: Industry-standard secure authentication
- **Token refresh**: Automatic re-authentication
- **Read-only**: Application cannot modify or delete files
- **Revocable**: Remove access anytime via Microsoft account settings

### Permissions

Application requires:
- `Files.Read`: View files you can access
- `Files.Read.All`: View shared files
- `User.Read`: Basic profile information

To revoke access:
1. Go to account.microsoft.com
2. Sign in
3. Go to Privacy → Apps & services
4. Remove "CrackSensor Image Viewer"

## Support

### Getting Help

1. Check this user guide
2. Review README_SETUP.md for configuration
3. Check BUILD_INSTRUCTIONS.md for technical details
4. Contact system administrator

### Reporting Issues

When reporting issues, include:
- Error message (exact text)
- What you were doing when error occurred
- Screenshot if applicable
- Your Windows version

### System Requirements

- **OS**: Windows 10 or 11 (64-bit)
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk Space**: 100 MB for application, varies for cache
- **Internet**: Required for accessing OneDrive
- **Display**: 1280x800 minimum resolution recommended

## Frequently Asked Questions

### Q: Do I need Python installed?
**A:** No, the executable includes everything needed.

### Q: Can I use this offline?
**A:** No, internet connection required to access OneDrive.

### Q: How many images can it handle?
**A:** Tested with thousands of images. Performance depends on your computer.

### Q: Can I download all images at once?
**A:** No, images are viewed online. Use OneDrive sync if you need local copies.

### Q: Will this work with my company OneDrive?
**A:** Yes, if you have proper permissions and access to the shared folder.

### Q: Can I view other OneDrive folders?
**A:** No, the application is configured for a specific shared folder.

### Q: How do I change which OneDrive folder to view?
**A:** Edit the `config.json` file and change the `share_link` value.

### Q: Is my data secure?
**A:** Yes, uses Microsoft's secure OAuth authentication. Application is read-only.

### Q: Can I use this on Mac or Linux?
**A:** The Python script works on all platforms, but the .exe is Windows only.

### Q: How often should I click Update?
**A:** Only when you know new images have been uploaded.

## Version Information

**Version**: 1.0.0
**Last Updated**: 2025
**Platform**: Windows 10/11
**License**: Internal use only

## Changelog

### Version 1.0.0
- Initial release
- OAuth authentication
- Folder tree navigation
- Image thumbnail grid
- Full-size image viewer
- Metadata display
- Sort functionality
- Cache management
- Update button

