# CrackSensor OneDrive Image Viewer - Setup Guide

> **⚠️ IMPORTANT**: You MUST register your own Azure AD application to use this tool. Pre-configured client IDs do not work. This is a free, one-time setup that takes ~5 minutes. See **[QUICK_SETUP.md](QUICK_SETUP.md)** for the fastest walkthrough.

## Prerequisites

- Windows 10/11 computer
- Microsoft account with access to the shared OneDrive folder
- Python 3.8+ (for development only, not required for end users)

## Azure App Registration Setup

To enable OAuth authentication for OneDrive access, you need to register an application in Azure Active Directory.

### Step 1: Access Azure Portal

1. Go to [Azure Portal](https://portal.azure.com)
2. Sign in with your Microsoft account
3. If you don't have access to Azure AD, you can use the Microsoft Graph Explorer instead (see Alternative Setup below)

### Step 2: Register a New Application

1. In the Azure Portal, search for **"Azure Active Directory"** or **"Microsoft Entra ID"**
2. Click on **"App registrations"** in the left sidebar
3. Click **"+ New registration"**

### Step 3: Configure Application

**Application Name:**
```
CrackSensor Image Viewer
```

**Supported account types:**
- Select: **"Accounts in any organizational directory (Any Azure AD directory - Multitenant) and personal Microsoft accounts (e.g. Skype, Xbox)"**

**Redirect URI:**
- Platform: **Public client/native (mobile & desktop)**
- URI: `http://localhost:8080`

Click **"Register"**

### Step 4: Get Application IDs

After registration, you'll see the **Overview** page:

1. **Copy the "Application (client) ID"** - You'll need this for the config file
   - Example: `12345678-1234-1234-1234-123456789abc`

2. **Copy the "Directory (tenant) ID"** - You'll need this too
   - Example: `87654321-4321-4321-4321-cba987654321`
   - Or use `common` for multi-tenant apps

### Step 5: Configure API Permissions

1. Click **"API permissions"** in the left sidebar
2. You'll see `User.Read` is already added by default - leave it
3. Click **"+ Add a permission"**
4. Select **"Microsoft Graph"**
5. Select **"Delegated permissions"**
6. Search for and check: **`Files.Read.All`** - Read all files user can access
7. Click **"Add permissions"**
8. (Optional) Click **"Grant admin consent"** if you have admin rights

**Note:** We only need `Files.Read.All` permission for this application.

### Step 6: Enable Public Client Flow

1. Click **"Authentication"** in the left sidebar
2. Scroll down to **"Advanced settings"**
3. Under **"Allow public client flows"**, toggle **"Yes"**
4. Click **"Save"**

### Step 7: Create Configuration File

Create a file named `config.json` in the same directory as the application:

```json
{
  "client_id": "YOUR_APPLICATION_CLIENT_ID",
  "tenant_id": "common",
  "authority": "https://login.microsoftonline.com/common",
  "scope": ["Files.Read.All"],
  "share_link": "https://1drv.ms/f/c/a2c7444680a5045e/EouB0aaaQVBDquA2aO6-zYsBv4v6jHWdkfgtBwPwjeKVUw?e=QyzwGn",
  "cache_expiry_minutes": 15
}
```

**Replace `YOUR_APPLICATION_CLIENT_ID`** with the Application (client) ID from Step 4.

**Note:** Reduced permissions to only `Files.Read.All` for minimal access requirements.

## Running the Application

### For Developers (Python)

1. Install dependencies:
```bash
pip install -r requirements.txt
```

2. Run the application:
```bash
python image_viewer_app.py
```

### For End Users (Executable)

1. Download `CrackSensorViewer.exe`
2. Create `config.json` in the same folder as the executable
3. Double-click `CrackSensorViewer.exe` to launch

## First-Time Authentication

1. Launch the application
2. A browser window will open for Microsoft login
3. Sign in with your Microsoft account
4. Grant permissions when prompted
5. The application will authenticate and load your images

## Troubleshooting

### "AADSTS7000218: The request body must contain the following parameter: 'client_assertion'"

- Make sure **"Allow public client flows"** is enabled in Azure (Step 6)

### "Permission denied" or "Access denied"

- Ensure your Microsoft account has access to the shared OneDrive folder
- Check that API permissions are correctly configured

### "Invalid client"

- Double-check your `client_id` in `config.json`
- Ensure the redirect URI matches exactly: `http://localhost:8080`

### Application won't launch

- Ensure `config.json` exists in the same folder as the executable
- Check that the JSON syntax is valid

## Security Notes

- The application stores authentication tokens locally in `%APPDATA%/CrackSensorViewer/token_cache.bin`
- Tokens are encrypted and automatically refresh
- Never share your `client_id` or tokens publicly
- The application only requests read permissions, it cannot modify files

## Support

For issues or questions, contact your system administrator or refer to the Microsoft Graph API documentation:
https://learn.microsoft.com/en-us/graph/api/overview

