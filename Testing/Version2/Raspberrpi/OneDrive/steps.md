



# onedrive Mounting:

Step 1: Clean Up Everything
# Kill all rclone processeskillall -9 rclone# Wait a momentsleep 2# Unmount if anything is mountedfusermount -u ~/OneDrive 2>/dev/null# Or use sudo if neededsudo umount ~/OneDrive 2>/dev/null# Check if anything is still runningps aux | grep rclone
Step 2: Check the Log File
# See what went wrongcat /home/ahmad/rclone.log
Step 3: Fix FUSE Configuration (for --allow-other)
The --allow-other flag requires special permissions:
# Check if user_allow_other is enabledcat /etc/fuse.conf# If not, enable itsudo nano /etc/fuse.conf# Uncomment this line (remove the #):# user_allow_other# Save and exit (Ctrl+X, Y, Enter)
Step 4: Mount Without --allow-other (Simpler)
Actually, you probably don't need --allow-other unless other users need access. Try without it:
# Simple mount (no daemon first, to see errors)rclone mount onedrive: ~/OneDrive --vfs-cache-mode writes --log-file=/home/ahmad/rclone.log
Leave this running and open another terminal to test:
# In another terminalls ~/OneDrive# If you see your OneDrive files, it works!
If that works, press Ctrl+C in the first terminal and then run as daemon:
rclone mount onedrive: ~/OneDrive --vfs-cache-mode writes --daemon --log-file=/home/ahmad/rclone.log
Step 5: Verify It's Working
# Check if mountedmount | grep OneDrive# List filesls -la ~/OneDrive# Check rclone processps aux | grep rclone | grep -v grep