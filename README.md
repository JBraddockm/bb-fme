# BB-FME — BlackBerry 10 File Manager Extension

A sample app that registers a custom source in the BlackBerry 10 File Manager. It documents the
undocumented mechanism that RIM's own apps (Dropbox, BlackBerry Link) used to add sources, and
implements it in both forms it supports:

- **On Device** — the app publishes its own source backed by a folder on the device. Files live on
  the device.
- **WebDAV** — the app registers a remote WebDAV server with the device's built-in
  `sys.cfs.webdav.todtm` provider, which mounts it. Files stay on the server and are fetched on
  demand.

Both register by writing PPS objects that belong to the system, which requires root.

## Why root

A File Manager source is made of PPS objects (and a symlink) owned by the system `cfs` user, so an
ordinary app cannot create them — it needs root, the same root already used to install unsigned
`.bar`s on this build. The app does this for you: it pipes a short script to `/bin/__root`, with no
manual step (`src/RootShell.cpp`).

## Method 1 — On device

The app owns and publishes the source. Source name `BB-FME`. This is what the Dropbox and
*Connect to Dropbox* apps did.

### Bookmark / Favourites Mechanism

This method registers any device-local folder as a File Manager source — it does not require a remote
source at all. Point `mount` at an existing folder and it appears in the File Manager as a
bookmark/favourite to that location; the file fetch below is optional.

**Register** (as root):

```sh
ln -s /accounts/1000/shared/misc/BB-FME /accounts/1000/shared/BB-FME

cat > /pps/system/filesystem/remote/BB-FME <<'EOF'
name::BB-FME
display_label::BB-FME (on device)
mount::/accounts/1000/shared/misc/BB-FME
public::/accounts/1000/shared/BB-FME
online::true
perimeter::personal
pausable::false
pinnable::false
supports_extended_provider_data::false
EOF
```

The File Manager enumerates `/pps/system/filesystem/remote/*` to build its source list; `mount` and
`public` point at the folder it shows.

**Delete** (as root) — unregister the source:

```sh
# 1. remove the File Manager source record (de-registers it)
rm -f /pps/system/filesystem/remote/BB-FME
```

```sh
# 2. remove the shared symlink the File Manager showed
rm -f /accounts/1000/shared/BB-FME
```

This only unregisters the source. `/accounts/1000/shared/BB-FME` is a symlink, so removing it
drops the link, not its target — the fetched files stay in the `mount` folder
`/accounts/1000/shared/misc/BB-FME`.

> **Warning — this is a sample app.** To also delete the fetched files, remove the `mount` folder:
> `rm -rf /accounts/1000/shared/misc/BB-FME`. That command *does* delete files on the device,
> recursively — run it only when you want the downloaded copies gone.

Code: `src/LocalCopyProvider.cpp`.

## Method 2 — WebDAV

The app registers a remote WebDAV server with the built-in `sys.cfs.webdav.todtm` provider, which
mounts it and publishes the source. Source name `BB-FME-DAV`. This provider was built for BlackBerry
Link's remote-access feature; here the same mechanism is reused to register an arbitrary WebDAV server.

**Register** (as root):

```sh
# 1. add the server to the status object's `servers` array (keep any existing entries)
echo 'servers:json:[{"server_name":"BB-FME-DAV","server_addr":"http://<host>","username":"<user>","password":"<pass>","port":<port>}]' >> /pps/system/cfs/webdav/status

# 2. publish the source yourself so it appears and is remembered at once.
#    APP is the provider's sandbox dir (its install id is not fixed across devices):
#    APP=$(echo /accounts/1000/appserv/sys.cfs.webdav.todtm.*)
ln -s "$APP/fs/mounts/BB-FME-DAV" /accounts/1000/shared/BB-FME-DAV
cat > /pps/system/filesystem/remote/BB-FME-DAV <<EOF
name::BB-FME-DAV
app_name::${APP##*/}
_SERVICEPATH::$APP/fs/mounts
mount::$APP/fs/mounts/BB-FME-DAV
public::/accounts/1000/shared/BB-FME-DAV
online::true
perimeter::personal
EOF

# 3. poke control to wake/launch the provider
echo 'name::BB-FME-DAV'   >> /pps/services/webdav/control
echo 'url::http://<host>' >> /pps/services/webdav/control
echo 'username::<user>'   >> /pps/services/webdav/control
echo 'password::<pass>'   >> /pps/services/webdav/control
echo 'port:n:<port>'      >> /pps/services/webdav/control
```

The provider reads its `servers` list **once at startup** and mounts each over HTTP (Basic/Digest).
Poking `control` only wakes an already-running provider; from cold it is launched by `cfsd` when the
File Manager opens the source. So the app publishes the `remote/BB-FME-DAV` record and the
`shared/BB-FME-DAV` symlink itself — the source then appears and is remembered immediately, and the
provider takes over the live mount (populating `$APP/fs/mounts/BB-FME-DAV`) when the source is opened.
`server_addr` is the host only; `port` is a separate field.

**Delete** (as root): remove the entry from the `servers` array, then:

```sh
rm -f /accounts/1000/shared/BB-FME-DAV /pps/system/filesystem/remote/BB-FME-DAV
```

### HTTPS (TLS)

Toggle **Use HTTPS (TLS)** to register the server over `https://`: the app writes an `https://`
`server_addr` and control `url`, and `port` is the TLS port (e.g. `8443`). Two conditions apply:

- **TLS 1.3 servers** are reachable only with a patched libcurl/OpenSSL on the device — stock
  BlackBerry 10 negotiates older TLS and fails the handshake. That drop-in is a separate project.
- **A self-signed certificate** (typical for a local server) must be trusted first: import it on the
  device via Settings → Security and Privacy → Certificates. Otherwise the handshake is rejected and
  the source shows empty.

Code: `src/WebDavProvider.cpp`.

## Test servers on the host

Both servers listen for incoming connections from the device, so the host firewall must allow them.
Make sure `python3` is permitted to accept incoming connections — on macOS, System Settings → Network →
Firewall will prompt the first time (click *Allow*), or add `python3` under *Options…*; on Linux, open
the chosen port (e.g. `sudo ufw allow 8080/tcp`). Without this, the device's requests never reach the
server and the source shows empty/offline.

### On device — a basic HTTP server

```sh
mkdir -p ~/share && echo "hello" > ~/share/readme.txt
cd ~/share && python3 -m http.server 8080 --bind 0.0.0.0
```

Enter the host's LAN IP and `8080`.

### WebDAV — a basic WebDAV server

A plain HTTP server will not do; the provider issues `OPTIONS` and `PROPFIND`. Example with `wsgidav`
(pure Python):

```sh
pip3 install --user wsgidav cheroot
mkdir -p ~/dav && echo "hello" > ~/dav/readme.txt
wsgidav --host 0.0.0.0 --port 8081 --root ~/dav --auth anonymous
```

Enter the host's LAN IP, `8081`, and the credentials. Alternatives: Apache `mod_dav`, nginx `dav`,
`rclone serve webdav`.

## Source layout

| Path | Role |
|---|---|
| `src/RootShell.{hpp,cpp}` | runs a script as root via `/bin/__root` (shared by both methods) |
| `src/SourceProvider.hpp` | abstract strategy for a File-Manager-extension method |
| `src/LocalCopyProvider.{hpp,cpp}` | Method 1 — On device |
| `src/WebDavProvider.{hpp,cpp}` | Method 2 — WebDAV |
| `src/applicationui.{hpp,cpp}` | QML facade over the two providers |
| `assets/main.qml` | method selector, per-method fields, Registered card |

## Permissions

```xml
<permission system="true">run_native</permission>
<permission>access_shared</permission>
<permission>access_internet</permission>
```
