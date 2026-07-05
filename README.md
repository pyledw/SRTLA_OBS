# SRTLA Receiver Plugin for OBS Studio

A custom, high-performance **SRTLA (SRT Link Aggregation)** Receiver plugin built directly for OBS Studio. This plugin allows OBS to act as a native receiver for bonded SRT streams sent from devices running Belabox, IRL Pro, or other SRTLA-compatible encoders.

---

## 🚀 Installation & Download

### 💻 Windows (Pre-Compiled DLL)
You can download the pre-compiled version of the plugin directly from this repository:

1. **Download the Zip Package**:
   * Navigate to the **[Install](/Install)** directory in this repository.
   * Download the **[SRTLA_Receiver_Windows.zip](/Install/SRTLA_Receiver_Windows.zip)** archive.
2. **Install**:
   * Close OBS Studio.
   * Extract the contents of the zip file directly into your OBS Studio installation folder:
     * Default path: `C:\Program Files\obs-studio\`
     * (This automatically copies `SRTLA_Receiver.dll` into `C:\Program Files\obs-studio\obs-plugins\64bit\`).
   * Restart OBS.
   
   *Note:* If you receive a SmartScreen warning or an error when loading, right-click the `SRTLA_Receiver.dll` file, select **Properties**, and check **Unblock** if it was blocked by Windows as a downloaded file.

### 🍏 macOS (Manual Build)
Since Apple requires plugins to be compiled and signed on macOS, you must build the plugin locally on your Mac:
1. Follow the **Developer Build Instructions** below to compile the plugin on your Mac.
2. Once compiled, copy the generated `.so` or `.dylib` library into your OBS plugins directory:
   * **System-wide path**: `/Library/Application Support/obs-studio/plugins/SRTLA_Receiver/bin/`
   * **User-specific path**: `~/Library/Application Support/obs-studio/plugins/SRTLA_Receiver/bin/`
3. Restart OBS.

### 🐧 Linux (Manual Build)
To use this plugin on Linux, you must compile it locally on your system:
1. Follow the **Developer Build Instructions** below to compile the plugin.
2. Once compiled, copy the library file into your OBS plugins folder:
   * **User-specific path**: `~/.config/obs-studio/plugins/SRTLA_Receiver/bin/`
   * **System-wide path**: `/usr/lib/obs-plugins/` or `/usr/share/obs/obs-plugins/`
3. Restart OBS.

---

## ⚙️ How to Configure OBS Source Settings

When you add a **SRTLA Receiver** source to your scene, you will have three main settings:

1. **SRTLA Bind IP (empty for ANY)**:
   * **Crucial for Multi-NIC / Multi-Homed PCs**: If your streaming PC has multiple network cards (e.g., one connected to the home network/Wi-Fi and one connected to a dedicated cellular router/modem), type the local IP address of the interface receiving the stream (e.g., `192.168.100.49`). 
   * This binds the socket and forces all outbound responses (REG3, ACKs, keep-alives) to go out of that specific network card.
   * If you are testing on your local network/Wi-Fi, leave this **blank** to listen on all interfaces.
2. **SRTLA Listen Port (UDP)**:
   * The UDP port where the external encoder (such as your phone) is sending the stream. Ensure this port matches the UDP port forwarding rule on your router.
3. **Local SRT Port**:
   * The internal port used to bridge the proxy to the OBS Media Source. This is strictly internal (`127.0.0.1`) and **should not** be opened or forwarded in your router.

---

## 🔌 Network & Port Forwarding Requirements

To ensure external connections from cellular networks function properly, configure the following:

1. **Forward the UDP Listen Port**: Forward the UDP port configured in OBS (e.g. `5000` UDP) on your router to the local IP of your OBS PC.
2. **Docker Port Conflicts**: If you previously used the Docker Belabox portal on port `5000` UDP, you **must stop the Docker container** before running the OBS plugin on port `5000` to prevent address binding conflicts.
3. **Internal Loopback**: The local SRT port (e.g. `4000`/`4006`) is internal-only. Do not forward it.

---

## 📊 Reopening the Monitor Dock

If you close the **SRTLA Status** tree widget, you can reopen it at any time from the top menu of OBS:
1. Go to **Docks** in the top menu.
2. Check **SRTLA Status** to snap the monitor back into the interface.

---

## 🛠️ Developer Build Instructions (Compilation)

If you wish to compile the plugin on your local system:

### Windows
1. Ensure you have **CMake 3.30+** and **Visual Studio 2022** installed.
2. Open terminal in the project root and run:
   ```powershell
   cmake -B build_x64 -S .
   cmake --build build_x64 --config Release
   ```
   The compiled library will output to `build_x64/Release/SRTLA_Receiver.dll`.

### macOS / Linux
1. Ensure you have **CMake**, **Qt6**, and build compilers (Clang/GCC) installed.
2. Open terminal in the project root and run:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
