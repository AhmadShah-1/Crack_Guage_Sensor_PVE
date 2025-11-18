#!/usr/bin/env python3
"""
CrackSensor OneDrive Image Viewer
A GUI application for viewing images from OneDrive with folder navigation
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import json
import os
import sys
import base64
import requests
import threading
import webbrowser
from pathlib import Path
from datetime import datetime
from io import BytesIO
from urllib.parse import quote

import msal
from PIL import Image, ImageTk

# ===========================
# Configuration Management
# ===========================
class Config:
    """Manages application configuration"""
    
    def __init__(self):
        self.config_file = self._get_config_path()
        self.config = self._load_config()
        
    def _get_config_path(self):
        """Get config file path (next to executable or script)"""
        if getattr(sys, 'frozen', False):
            # Running as executable
            app_dir = Path(sys.executable).parent
        else:
            # Running as script
            app_dir = Path(__file__).parent
        return app_dir / "config.json"
    
    def _load_config(self):
        """Load configuration from file"""
        try:
            with open(self.config_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            messagebox.showerror("Configuration Error", 
                               f"config.json not found!\n\nPlease create config.json in:\n{self.config_file.parent}")
            sys.exit(1)
        except json.JSONDecodeError as e:
            messagebox.showerror("Configuration Error", 
                               f"Invalid JSON in config.json:\n{e}")
            sys.exit(1)
    
    def get(self, key, default=None):
        """Get configuration value"""
        return self.config.get(key, default)

# ===========================
# Authentication Manager
# ===========================
class AuthManager:
    """Handles Microsoft OAuth authentication"""
    
    def __init__(self, config):
        self.config = config
        self.app = None
        self.token_cache_file = self._get_cache_path()
        self._load_cache()
        
    def _get_cache_path(self):
        """Get token cache file path"""
        cache_dir = Path(os.getenv('APPDATA')) / 'CrackSensorViewer'
        cache_dir.mkdir(parents=True, exist_ok=True)
        return cache_dir / 'token_cache.bin'
    
    def _load_cache(self):
        """Load token cache"""
        cache = msal.SerializableTokenCache()
        if self.token_cache_file.exists():
            cache.deserialize(open(self.token_cache_file, 'r').read())
        
        self.app = msal.PublicClientApplication(
            self.config.get('client_id'),
            authority=self.config.get('authority'),
            token_cache=cache
        )
        
    def _save_cache(self):
        """Save token cache"""
        if self.app.token_cache.has_state_changed:
            with open(self.token_cache_file, 'w') as f:
                f.write(self.app.token_cache.serialize())
    
    def get_token(self):
        """Get access token (with interactive login if needed)"""
        accounts = self.app.get_accounts()
        result = None
        
        if accounts:
            # Try silent authentication
            result = self.app.acquire_token_silent(
                self.config.get('scope'),
                account=accounts[0]
            )
        
        if not result:
            # Interactive authentication using device code flow
            flow = self.app.initiate_device_flow(scopes=self.config.get('scope'))
            
            if 'user_code' not in flow:
                raise Exception("Failed to create device flow")
            
            # Show device code to user in GUI dialog
            auth_message = (
                "AUTHENTICATION REQUIRED\n\n"
                f"{flow['message']}\n\n"
                "Click OK to open your browser, then enter the code shown above."
            )
            
            # Show message box with authentication info
            messagebox.showinfo("Microsoft Authentication", auth_message)
            
            # Open browser
            webbrowser.open(flow['verification_uri'])
            
            # Wait for user to authenticate
            result = self.app.acquire_token_by_device_flow(flow)
        
        if 'access_token' in result:
            self._save_cache()
            return result['access_token']
        else:
            error = result.get('error_description', result.get('error'))
            raise Exception(f"Authentication failed: {error}")

# ===========================
# OneDrive API Client
# ===========================
class OneDriveClient:
    """Handles OneDrive API operations"""
    
    def __init__(self, auth_manager, config):
        self.auth = auth_manager
        self.config = config
        self.base_url = "https://graph.microsoft.com/v1.0"
        self.share_token = self._get_share_token()
        
    def _get_share_token(self):
        """Convert share URL to share token"""
        share_url = self.config.get('share_link')
        
        # Remove query parameters (?e=... etc) from the URL
        if '?' in share_url:
            share_url = share_url.split('?')[0]
        
        # Encode URL in base64
        encoded = base64.b64encode(share_url.encode()).decode()
        # Format for Graph API (remove padding and add prefix)
        return "u!" + encoded.rstrip('=').replace('/', '_').replace('+', '-')
    
    def _get_headers(self):
        """Get authorization headers"""
        token = self.auth.get_token()
        return {
            'Authorization': f'Bearer {token}',
            'Content-Type': 'application/json'
        }
    
    def get_shared_root(self):
        """Get the root shared item"""
        url = f"{self.base_url}/shares/{self.share_token}/driveItem"
        response = requests.get(url, headers=self._get_headers())
        response.raise_for_status()
        return response.json()
    
    def get_children(self, item_id=None, drive_id=None):
        """Get children of a folder"""
        if item_id and drive_id:
            # Use drive-specific endpoint for better reliability
            url = f"{self.base_url}/drives/{drive_id}/items/{item_id}/children"
        elif item_id:
            url = f"{self.base_url}/shares/{self.share_token}/driveItem/items/{item_id}/children"
        else:
            url = f"{self.base_url}/shares/{self.share_token}/driveItem/children"
        
        try:
            response = requests.get(url, headers=self._get_headers(), timeout=30)
            response.raise_for_status()
            return response.json().get('value', [])
        except requests.exceptions.HTTPError as e:
            print(f"HTTP Error accessing {url}: {e}")
            # If using shares endpoint fails, try alternative approach
            if 'shares' in url and item_id:
                # Try without the shares prefix
                try:
                    root = self.get_shared_root()
                    if 'parentReference' in root and 'driveId' in root['parentReference']:
                        drive_id = root['parentReference']['driveId']
                        alt_url = f"{self.base_url}/drives/{drive_id}/items/{item_id}/children"
                        response = requests.get(alt_url, headers=self._get_headers(), timeout=30)
                        response.raise_for_status()
                        return response.json().get('value', [])
                except:
                    pass
            return []
    
    def download_file(self, item_id, drive_id=None):
        """Download file content"""
        if drive_id:
            url = f"{self.base_url}/drives/{drive_id}/items/{item_id}/content"
        else:
            url = f"{self.base_url}/shares/{self.share_token}/driveItem/items/{item_id}/content"
        
        response = requests.get(url, headers=self._get_headers(), timeout=30)
        response.raise_for_status()
        return response.content
    
    def get_thumbnail(self, item_id, drive_id=None, size='medium'):
        """Get thumbnail for an image"""
        if drive_id:
            url = f"{self.base_url}/drives/{drive_id}/items/{item_id}/thumbnails/0/{size}/content"
        else:
            url = f"{self.base_url}/shares/{self.share_token}/driveItem/items/{item_id}/thumbnails/0/{size}/content"
        
        try:
            response = requests.get(url, headers=self._get_headers(), timeout=10)
            response.raise_for_status()
            return response.content
        except:
            # Return None if thumbnail unavailable
            return None

# ===========================
# Folder Navigation Manager
# ===========================
class FolderNavigator:
    """Manages folder structure navigation"""
    
    def __init__(self, onedrive_client):
        self.client = onedrive_client
        self.structure = {}
        self.flat_structure = []
        self.drive_id = None
        
    def build_structure(self, progress_callback=None):
        """Build complete folder structure"""
        try:
            if progress_callback:
                progress_callback("Loading OneDrive structure...")
            
            root = self.client.get_shared_root()
            
            # Extract drive_id if available
            if 'parentReference' in root and 'driveId' in root['parentReference']:
                self.drive_id = root['parentReference']['driveId']
            elif 'id' in root and '!' in root['id']:
                # Extract drive ID from the item ID (format: driveId!itemId)
                self.drive_id = root['id'].split('!')[0]
            
            self.structure = self._build_tree(root['id'], progress_callback)
            self._flatten_structure()
            
            return True
        except Exception as e:
            print(f"Error building structure: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def _build_tree(self, item_id, progress_callback, path=""):
        """Recursively build folder tree"""
        children = self.client.get_children(item_id, self.drive_id)
        tree = {'folders': {}, 'files': []}
        
        for item in children:
            name = item['name']
            
            if 'folder' in item:
                # It's a folder
                if progress_callback:
                    progress_callback(f"Scanning {path}/{name}...")
                tree['folders'][name] = self._build_tree(
                    item['id'], 
                    progress_callback,
                    f"{path}/{name}"
                )
            elif name.lower().endswith(('.jpg', '.jpeg', '.txt')):
                # It's an image or metadata file
                tree['files'].append({
                    'name': name,
                    'id': item['id'],
                    'size': item.get('size', 0),
                    'modified': item.get('lastModifiedDateTime', ''),
                    'path': path
                })
        
        return tree
    
    def _flatten_structure(self):
        """Create flat list of all cameras with their images"""
        self.flat_structure = []
        self._flatten_recursive(self.structure, [])
    
    def _flatten_recursive(self, tree, path):
        """Recursively flatten structure"""
        # Check if this is a camera folder (has images)
        if tree['files']:
            # Group by location/floor/camera
            camera_path = '/'.join(path)
            images = [f for f in tree['files'] if f['name'].lower().endswith(('.jpg', '.jpeg'))]
            metadata_files = [f for f in tree['files'] if f['name'].lower().endswith('.txt')]
            
            if images:
                self.flat_structure.append({
                    'path': camera_path,
                    'location': path[0] if len(path) > 0 else 'Unknown',
                    'floor': path[1] if len(path) > 1 else 'Unknown',
                    'camera': path[2] if len(path) > 2 else 'Unknown',
                    'images': images,
                    'metadata': metadata_files
                })
        
        # Recurse into subfolders
        for folder_name, subtree in tree['folders'].items():
            self._flatten_recursive(subtree, path + [folder_name])
    
    def get_locations(self):
        """Get list of unique locations"""
        return sorted(set(item['location'] for item in self.flat_structure))
    
    def get_floors(self, location):
        """Get floors for a location"""
        return sorted(set(item['floor'] for item in self.flat_structure 
                         if item['location'] == location))
    
    def get_cameras(self, location, floor):
        """Get cameras for a location/floor"""
        return sorted(set(item['camera'] for item in self.flat_structure 
                         if item['location'] == location and item['floor'] == floor))
    
    def get_images(self, location, floor, camera):
        """Get images for a specific camera"""
        for item in self.flat_structure:
            if (item['location'] == location and 
                item['floor'] == floor and 
                item['camera'] == camera):
                return item['images'], item['metadata']
        return [], []

# ===========================
# Image Cache Manager
# ===========================
class ImageCache:
    """Manages image caching"""
    
    def __init__(self, onedrive_client, navigator=None):
        self.client = onedrive_client
        self.navigator = navigator
        self.cache_dir = Path(os.getenv('APPDATA')) / 'CrackSensorViewer' / 'cache'
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self.memory_cache = {}
        
    def get_thumbnail(self, item_id):
        """Get thumbnail (from cache or download)"""
        # Check memory cache first
        if item_id in self.memory_cache:
            return self.memory_cache[item_id]
        
        # Check disk cache
        cache_file = self.cache_dir / f"{item_id.replace('!', '_')}_thumb.jpg"
        if cache_file.exists():
            try:
                img = Image.open(cache_file)
                self.memory_cache[item_id] = img
                return img
            except:
                # Cache file corrupted, delete it
                cache_file.unlink()
        
        # Try to get thumbnail from Microsoft
        try:
            drive_id = self.navigator.drive_id if self.navigator else None
            data = self.client.get_thumbnail(item_id, drive_id)
            if data:
                img = Image.open(BytesIO(data))
                img.thumbnail((150, 150), Image.Resampling.LANCZOS)
                img.save(cache_file, 'JPEG')
                self.memory_cache[item_id] = img
                return img
        except Exception as e:
            print(f"Thumbnail API failed for {item_id}: {e}")
        
        # Fallback: Download full image and create thumbnail
        try:
            print(f"Generating thumbnail from full image for {item_id}")
            drive_id = self.navigator.drive_id if self.navigator else None
            data = self.client.download_file(item_id, drive_id)
            img = Image.open(BytesIO(data))
            img.thumbnail((150, 150), Image.Resampling.LANCZOS)
            # Save to cache
            img.save(cache_file, 'JPEG')
            self.memory_cache[item_id] = img
            return img
        except Exception as e:
            print(f"Failed to generate thumbnail for {item_id}: {e}")
        
        return None
    
    def get_full_image(self, item_id):
        """Get full-size image"""
        try:
            drive_id = self.navigator.drive_id if self.navigator else None
            data = self.client.download_file(item_id, drive_id)
            return Image.open(BytesIO(data))
        except Exception as e:
            print(f"Error downloading full image {item_id}: {e}")
            return None
    
    def get_metadata(self, item_id):
        """Get metadata file content"""
        try:
            drive_id = self.navigator.drive_id if self.navigator else None
            data = self.client.download_file(item_id, drive_id)
            return data.decode('utf-8')
        except Exception as e:
            print(f"Error downloading metadata {item_id}: {e}")
            return None
    
    def clear_cache(self):
        """Clear all cached files"""
        self.memory_cache.clear()
        for file in self.cache_dir.glob('*'):
            try:
                file.unlink()
            except:
                pass

# ===========================
# Image Viewer Window
# ===========================
class ImageViewerWindow:
    """Full-size image viewer with metadata"""
    
    def __init__(self, parent, image_cache, images, current_index, location, floor, camera, metadata_files=None):
        self.cache = image_cache
        self.images = images
        self.metadata_files = metadata_files or []
        self.current_index = current_index
        self.location = location
        self.floor = floor
        self.camera = camera
        
        # Create window
        self.window = tk.Toplevel(parent)
        self.window.title(f"Image Viewer - {camera}")
        self.window.geometry("1000x700")
        
        # Main container
        main_frame = ttk.Frame(self.window)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Image display area (left side)
        image_frame = ttk.Frame(main_frame)
        image_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Navigation buttons
        nav_frame = ttk.Frame(image_frame)
        nav_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        ttk.Button(nav_frame, text="◀ Previous", command=self.show_previous).pack(side=tk.LEFT, padx=5)
        self.image_label_text = ttk.Label(nav_frame, text="")
        self.image_label_text.pack(side=tk.LEFT, expand=True)
        ttk.Button(nav_frame, text="Next ▶", command=self.show_next).pack(side=tk.RIGHT, padx=5)
        
        # Image canvas
        self.canvas = tk.Canvas(image_frame, bg='gray20')
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Metadata panel (right side)
        meta_frame = ttk.LabelFrame(main_frame, text="Image Metadata", padding=10)
        meta_frame.pack(side=tk.RIGHT, fill=tk.BOTH, padx=5, pady=5)
        
        self.metadata_text = scrolledtext.ScrolledText(meta_frame, width=30, height=40, wrap=tk.WORD)
        self.metadata_text.pack(fill=tk.BOTH, expand=True)
        
        # Show first image
        self.show_image()
        
        # Bind keys
        self.window.bind('<Left>', lambda e: self.show_previous())
        self.window.bind('<Right>', lambda e: self.show_next())
        self.window.bind('<Escape>', lambda e: self.window.destroy())
    
    def show_image(self):
        """Display current image"""
        if not self.images:
            return
        
        image_info = self.images[self.current_index]
        self.image_label_text.config(
            text=f"Image {self.current_index + 1} of {len(self.images)} - {image_info['name']}"
        )
        
        # Load image in thread
        threading.Thread(target=self._load_image_thread, args=(image_info,), daemon=True).start()
    
    def _load_image_thread(self, image_info):
        """Load image in background thread"""
        img = self.cache.get_full_image(image_info['id'])
        if img:
            self.window.after(0, self._display_image, img, image_info)
    
    def _display_image(self, img, image_info):
        """Display image on canvas"""
        # Check if window still exists
        try:
            if not self.window.winfo_exists():
                return
        except:
            return
        
        # Resize to fit canvas
        try:
            canvas_width = self.canvas.winfo_width()
            canvas_height = self.canvas.winfo_height()
        except:
            # Window was closed
            return
        
        if canvas_width > 1 and canvas_height > 1:
            img.thumbnail((canvas_width - 10, canvas_height - 10), Image.Resampling.LANCZOS)
        
        # Convert to PhotoImage
        self.photo = ImageTk.PhotoImage(img)
        
        # Display on canvas
        self.canvas.delete('all')
        x = (canvas_width - self.photo.width()) // 2
        y = (canvas_height - self.photo.height()) // 2
        self.canvas.create_image(x, y, anchor=tk.NW, image=self.photo)
        
        # Load metadata
        self._load_metadata(image_info)
    
    def _load_metadata(self, image_info):
        """Load and display metadata"""
        self.metadata_text.delete(1.0, tk.END)
        
        # Basic info
        self.metadata_text.insert(tk.END, f"Location: {self.location}\n")
        self.metadata_text.insert(tk.END, f"Floor: {self.floor}\n")
        self.metadata_text.insert(tk.END, f"Camera: {self.camera}\n\n")
        
        self.metadata_text.insert(tk.END, f"Filename: {image_info['name']}\n")
        self.metadata_text.insert(tk.END, f"Size: {image_info['size']:,} bytes\n")
        
        # Format the date nicely
        try:
            date_str = datetime.fromisoformat(image_info['modified'].replace('Z', '+00:00'))
            self.metadata_text.insert(tk.END, f"Modified: {date_str.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        except:
            self.metadata_text.insert(tk.END, f"Modified: {image_info['modified']}\n\n")
        
        # Try to load .txt metadata file
        base_name = image_info['name'].rsplit('.', 1)[0]
        metadata_file = next((f for f in self.metadata_files if f['name'] == f"{base_name}.txt"), None)
        
        if metadata_file:
            self.metadata_text.insert(tk.END, "="*30 + "\n")
            self.metadata_text.insert(tk.END, "SENSOR METADATA:\n")
            self.metadata_text.insert(tk.END, "="*30 + "\n\n")
            
            metadata_content = self.cache.get_metadata(metadata_file['id'])
            if metadata_content:
                self.metadata_text.insert(tk.END, metadata_content)
            else:
                self.metadata_text.insert(tk.END, "Failed to load metadata file\n")
        else:
            self.metadata_text.insert(tk.END, "\n(No metadata file found)")
    
    def show_previous(self):
        """Show previous image"""
        if self.current_index > 0:
            self.current_index -= 1
            self.show_image()
    
    def show_next(self):
        """Show next image"""
        if self.current_index < len(self.images) - 1:
            self.current_index += 1
            self.show_image()

# ===========================
# Main Application
# ===========================
class CrackSensorViewer:
    """Main application window"""
    
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("CrackSensor Image Viewer")
        self.root.geometry("1400x800")
        
        # Initialize components
        self.config = Config()
        self.auth = None
        self.client = None
        self.navigator = None
        self.cache = None
        
        self.current_location = None
        self.current_floor = None
        self.current_camera = None
        self.current_images = []
        self.current_metadata = []
        self.sort_order = "date_desc"
        
        # Create UI
        self.create_ui()
        
        # Start authentication
        self.authenticate()
    
    def create_ui(self):
        """Create the user interface"""
        # Top toolbar
        toolbar = ttk.Frame(self.root, padding=5)
        toolbar.pack(side=tk.TOP, fill=tk.X)
        
        ttk.Button(toolbar, text="🔄 Update", command=self.refresh_data).pack(side=tk.LEFT, padx=5)
        
        ttk.Label(toolbar, text="Sort by:").pack(side=tk.LEFT, padx=(20, 5))
        self.sort_var = tk.StringVar(value="date_desc")
        sort_combo = ttk.Combobox(toolbar, textvariable=self.sort_var, state='readonly', width=15)
        sort_combo['values'] = ('Date (Newest)', 'Date (Oldest)', 'Filename (A-Z)', 'Filename (Z-A)')
        sort_combo.current(0)
        sort_combo.bind('<<ComboboxSelected>>', lambda e: self.apply_sort())
        sort_combo.pack(side=tk.LEFT, padx=5)
        
        self.status_label = ttk.Label(toolbar, text="Ready")
        self.status_label.pack(side=tk.RIGHT, padx=10)
        
        # Main content area
        content = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        content.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left panel - Tree navigation
        left_frame = ttk.Frame(content)
        content.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="Folder Structure", font=('', 10, 'bold')).pack(pady=5)
        
        tree_scroll = ttk.Scrollbar(left_frame)
        tree_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.tree = ttk.Treeview(left_frame, yscrollcommand=tree_scroll.set)
        self.tree.pack(fill=tk.BOTH, expand=True)
        tree_scroll.config(command=self.tree.yview)
        
        self.tree.bind('<<TreeviewSelect>>', self.on_tree_select)
        
        # Right panel - Image grid
        right_frame = ttk.Frame(content)
        content.add(right_frame, weight=2)
        
        ttk.Label(right_frame, text="Images", font=('', 10, 'bold')).pack(pady=5)
        
        # Scrollable canvas for images
        canvas_frame = ttk.Frame(right_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)
        
        v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL)
        v_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL)
        h_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        
        self.image_canvas = tk.Canvas(canvas_frame, bg='white',
                                      yscrollcommand=v_scroll.set,
                                      xscrollcommand=h_scroll.set)
        self.image_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        v_scroll.config(command=self.image_canvas.yview)
        h_scroll.config(command=self.image_canvas.xview)
        
        self.image_frame = ttk.Frame(self.image_canvas)
        self.image_canvas.create_window((0, 0), window=self.image_frame, anchor=tk.NW)
        
        self.image_frame.bind('<Configure>', 
                             lambda e: self.image_canvas.configure(scrollregion=self.image_canvas.bbox('all')))
    
    def authenticate(self):
        """Perform authentication"""
        self.set_status("Authenticating...")
        
        def auth_thread():
            try:
                self.auth = AuthManager(self.config)
                self.client = OneDriveClient(self.auth, self.config)
                self.navigator = FolderNavigator(self.client)
                # Create cache with navigator reference (will be updated after structure loads)
                self.cache = ImageCache(self.client, self.navigator)
                
                self.root.after(0, self.load_structure)
            except Exception as e:
                self.root.after(0, lambda: messagebox.showerror("Authentication Error", str(e)))
                self.root.after(0, self.root.quit)
        
        threading.Thread(target=auth_thread, daemon=True).start()
    
    def load_structure(self):
        """Load folder structure"""
        self.set_status("Loading folder structure...")
        
        def load_thread():
            success = self.navigator.build_structure(
                lambda msg: self.root.after(0, self.set_status, msg)
            )
            
            if success:
                self.root.after(0, self.populate_tree)
            else:
                self.root.after(0, lambda: messagebox.showerror("Error", 
                    "Failed to load folder structure"))
        
        threading.Thread(target=load_thread, daemon=True).start()
    
    def populate_tree(self):
        """Populate tree with folder structure"""
        self.tree.delete(*self.tree.get_children())
        
        locations = self.navigator.get_locations()
        
        for location in locations:
            loc_id = self.tree.insert('', tk.END, text=location, open=False)
            
            floors = self.navigator.get_floors(location)
            for floor in floors:
                floor_id = self.tree.insert(loc_id, tk.END, text=floor, open=False)
                
                cameras = self.navigator.get_cameras(location, floor)
                for camera in cameras:
                    self.tree.insert(floor_id, tk.END, text=camera, 
                                   values=(location, floor, camera))
        
        self.set_status(f"Ready - {len(locations)} locations loaded")
    
    def on_tree_select(self, event):
        """Handle tree selection"""
        selection = self.tree.selection()
        if not selection:
            return
        
        item = selection[0]
        values = self.tree.item(item, 'values')
        
        if values and len(values) == 3:
            # Camera selected
            self.current_location = values[0]
            self.current_floor = values[1]
            self.current_camera = values[2]
            self.load_images()
    
    def load_images(self):
        """Load images for selected camera"""
        if not all([self.current_location, self.current_floor, self.current_camera]):
            return
        
        self.set_status(f"Loading images for {self.current_camera}...")
        
        # Get images
        images, metadata = self.navigator.get_images(
            self.current_location, 
            self.current_floor, 
            self.current_camera
        )
        
        self.current_images = images
        self.current_metadata = metadata
        
        # Apply sort
        self.apply_sort()
    
    def apply_sort(self):
        """Apply current sort order"""
        if not self.current_images:
            return
        
        sort_option = self.sort_var.get()
        
        if sort_option == 'Date (Newest)':
            self.current_images.sort(key=lambda x: x['modified'], reverse=True)
        elif sort_option == 'Date (Oldest)':
            self.current_images.sort(key=lambda x: x['modified'])
        elif sort_option == 'Filename (A-Z)':
            self.current_images.sort(key=lambda x: x['name'])
        elif sort_option == 'Filename (Z-A)':
            self.current_images.sort(key=lambda x: x['name'], reverse=True)
        
        self.display_images()
    
    def display_images(self):
        """Display images in grid"""
        # Clear existing images
        for widget in self.image_frame.winfo_children():
            widget.destroy()
        
        if not self.current_images:
            ttk.Label(self.image_frame, text="No images found").grid(row=0, column=0, padx=20, pady=20)
            return
        
        self.set_status(f"Displaying {len(self.current_images)} images...")
        
        # Create grid of thumbnails
        columns = 4
        self.thumbnail_images = []  # Keep reference to prevent garbage collection
        
        for idx, image_info in enumerate(self.current_images):
            row = idx // columns
            col = idx % columns
            
            frame = ttk.Frame(self.image_frame, relief=tk.RAISED, borderwidth=2)
            frame.grid(row=row, column=col, padx=10, pady=10)
            
            # Placeholder initially
            placeholder = ttk.Label(frame, text="Loading...", width=20)
            placeholder.pack()
            
            # Filename label
            name_label = ttk.Label(frame, text=image_info['name'], wraplength=150)
            name_label.pack()
            
            # Date label
            try:
                date_str = datetime.fromisoformat(image_info['modified'].replace('Z', '+00:00')).strftime('%Y-%m-%d %H:%M')
            except:
                date_str = "Unknown date"
            date_label = ttk.Label(frame, text=date_str, font=('', 8))
            date_label.pack()
            
            # Load thumbnail in thread
            threading.Thread(target=self._load_thumbnail, 
                           args=(image_info, placeholder, frame, idx), 
                           daemon=True).start()
        
        self.set_status(f"Loaded {len(self.current_images)} images")
    
    def _load_thumbnail(self, image_info, placeholder, frame, index):
        """Load thumbnail in background"""
        try:
            img = self.cache.get_thumbnail(image_info['id'])
            
            if img:
                photo = ImageTk.PhotoImage(img)
                self.root.after(0, self._display_thumbnail, 
                              photo, placeholder, frame, index, image_info)
            else:
                # Failed to load - show error placeholder
                self.root.after(0, self._display_error_thumbnail, 
                              placeholder, frame, index, image_info)
        except Exception as e:
            print(f"Error in _load_thumbnail: {e}")
            self.root.after(0, self._display_error_thumbnail, 
                          placeholder, frame, index, image_info)
    
    def _display_thumbnail(self, photo, placeholder, frame, index, image_info):
        """Display thumbnail on UI thread"""
        try:
            placeholder.destroy()
        except:
            pass  # Placeholder may already be destroyed
        
        label = ttk.Label(frame, image=photo, cursor='hand2')
        label.image = photo  # Keep reference
        label.pack()
        
        # Bind click event
        label.bind('<Button-1>', lambda e: self.open_image_viewer(index))
        
        self.thumbnail_images.append(photo)
    
    def _display_error_thumbnail(self, placeholder, frame, index, image_info):
        """Display error placeholder when thumbnail fails"""
        try:
            placeholder.destroy()
        except:
            pass
        
        # Create a simple error display
        error_label = ttk.Label(frame, text="⚠️\nImage\nUnavailable", 
                               justify=tk.CENTER, foreground='red',
                               cursor='hand2')
        error_label.pack()
        
        # Still allow clicking to try viewing full image
        error_label.bind('<Button-1>', lambda e: self.open_image_viewer(index))
    
    def open_image_viewer(self, index):
        """Open full-size image viewer"""
        ImageViewerWindow(self.root, self.cache, self.current_images, index,
                         self.current_location, self.current_floor, self.current_camera,
                         self.current_metadata)
    
    def refresh_data(self):
        """Refresh data from OneDrive"""
        self.cache.clear_cache()
        self.load_structure()
    
    def set_status(self, message):
        """Update status bar"""
        self.status_label.config(text=message)
        self.root.update_idletasks()
    
    def run(self):
        """Start the application"""
        self.root.mainloop()

# ===========================
# Main Entry Point
# ===========================
if __name__ == '__main__':
    app = CrackSensorViewer()
    app.run()

