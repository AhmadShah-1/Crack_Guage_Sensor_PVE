# Quick Azure Setup (5 Minutes)

## Why Do I Need This?

Microsoft requires all applications accessing OneDrive to be registered. This is a one-time setup that takes about 5 minutes.

## Step-by-Step Instructions

### 1. Go to Azure Portal

Open: https://portal.azure.com

Sign in with **any Microsoft account** (even personal @outlook.com, @hotmail.com, @gmail.com accounts work!)

### 2. Find App Registrations

In the search bar at the top, type: **"App registrations"**

Click on the result.

### 3. Create New Registration

Click the **"+ New registration"** button at the top.

Fill in:
- **Name**: `CrackSensorViewer` (or anything you want)
- **Supported account types**: Select **"Accounts in any organizational directory (Any Azure AD directory - Multitenant) and personal Microsoft accounts"**
- **Redirect URI**: 
  - Platform: **"Public client/native (mobile & desktop)"**
  - URI: `http://localhost:8080`

Click **"Register"**

### 4. Copy Your Client ID

You'll see an **"Overview"** page with:
- **Application (client) ID**: Copy this! It looks like: `12345678-abcd-1234-abcd-123456789abc`

Paste it into your `config.json` file where it says `YOUR_CLIENT_ID_HERE`

### 5. Enable Public Client Flow

On the left sidebar, click **"Authentication"**

Scroll down to **"Advanced settings"** section

Find **"Allow public client flows"**

Toggle to **"Yes"**

Click **"Save"** at the top

### 6. Add API Permissions

On the left sidebar, click **"API permissions"**

You should see "User.Read" already there - that's fine!

Click **"+ Add a permission"**

Click **"Microsoft Graph"**

Click **"Delegated permissions"**

Search for and check: **"Files.Read.All"**

Click **"Add permissions"** at the bottom

### 7. Update Your config.json

Your `config.json` should now look like:

```json
{
  "client_id": "12345678-abcd-1234-abcd-123456789abc",
  "tenant_id": "common",
  "authority": "https://login.microsoftonline.com/common",
  "scope": ["Files.Read.All"],
  "share_link": "https://1drv.ms/f/c/a2c7444680a5045e/EouB0aaaQVBDquA2aO6-zYsBv4v6jHWdkfgtBwPwjeKVUw?e=QyzwGn",
  "cache_expiry_minutes": 15
}
```

(Replace the client_id with YOUR actual ID from step 4)

### 8. Run the Application!

```bash
python image_viewer_app.py
```

When it asks you to sign in, use the **same Microsoft account** you used to access the shared OneDrive folder.

## That's It!

The app is now registered. **Anyone with access to the shared OneDrive folder** can sign in with their own Microsoft account.

## Common Questions

### Q: Do other users need to register too?
**A:** No! Only you need to register the app once. Other users just sign in with their Microsoft accounts.

### Q: I don't have Azure access
**A:** Any Microsoft account works - even free personal accounts (@outlook.com, @hotmail.com). You automatically get Azure AD access.

### Q: Will this cost money?
**A:** No! App registration is completely free for this use case.

### Q: Can I use the same client_id for multiple computers?
**A:** Yes! Share the same `config.json` with the same `client_id` on all computers.

### Q: What if I get "admin consent required"?
**A:** Make sure you selected "Accounts in any organizational directory AND personal Microsoft accounts" in step 3.

### Q: The redirect URI error
**A:** Make sure you selected "Public client/native" as the platform type, and the exact URI is `http://localhost:8080`

## Troubleshooting

### "Invalid client" error
- Double-check the client_id is copied correctly (no extra spaces)
- Make sure "Allow public client flows" is enabled (step 5)

### "Consent not granted" error
- The Files.Read.All permission must be added (step 6)
- User must have access to the shared OneDrive folder

### "Redirect URI mismatch"
- Ensure redirect URI is exactly: `http://localhost:8080`
- Platform must be "Public client/native"

## Still Need Help?

Check the full documentation in `README_SETUP.md` for detailed screenshots and troubleshooting.

---

**Time to complete**: ~5 minutes
**Cost**: Free
**Difficulty**: Easy (just copy & paste)


