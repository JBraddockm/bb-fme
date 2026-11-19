/*
 * File Manager source template — two methods, one app:
 *
 *   Local copy : fetch a file from a host HTTP server, store it on the device,
 *                and publish our own File Manager source (files live on-device).
 *   WebDAV     : hand a remote WebDAV server to the device's webdav provider,
 *                mounted live (files stay on the server, fetched on demand).
 *
 * Pick the method with the segmented control; each method has its own fields,
 * its own Registered card (press-and-hold to Delete, button to Update), and its
 * own wording. UI styling adapted from the TLSDeBug sample.
 */

import bb.cascades 1.4
import bb.system 1.2

Page {
    id: page

    property string method: "local"   // "local" | "webdav"

    // A standard on-screen toast for delete feedback (not a modal dialog).
    attachedObjects: [
        SystemToast { id: notifyToast }
    ]

    // Local-copy state
    property bool localRegistered: false
    property string localHostShown: ""
    property string localPortShown: ""
    property string localMountShown: ""

    // WebDAV state
    property bool davRegistered: false
    property string davUrlShown: ""
    property string davUserShown: ""

    function doRegisterLocal() {
        statusLabel.text = "Working…";
        var r = _app.registerLocal(localHost.text, parseInt(localPort.text));
        statusLabel.text = r;
        if (r.indexOf("record_OK") !== -1) {
            page.localHostShown = localHost.text;
            page.localPortShown = localPort.text;
            page.localMountShown = _app.localMountPath();
            page.localRegistered = true;
        }
    }
    function doUnregisterLocal() {
        var r = _app.unregisterLocal();
        if (r.indexOf("record_present") === -1) {
            page.localRegistered = false;
            notifyToast.body = "“On device” source removed";
        } else {
            notifyToast.body = "Could not remove the source";
        }
        notifyToast.show();
    }

    function doRegisterWebdav() {
        statusLabel.text = "Working…";
        var scheme = davHttps.checked ? "https" : "http";
        var r = _app.registerWebdav(davHost.text, parseInt(davPort.text),
                                    davUser.text, davPass.text, davHttps.checked);
        statusLabel.text = r;
        // The resmgr publishes the source asynchronously after the poke; show the
        // card optimistically (confirm in the File Manager).
        page.davUrlShown = scheme + "://" + davHost.text + ":" + davPort.text;
        page.davUserShown = davUser.text;
        page.davRegistered = true;
    }
    function doUnregisterWebdav() {
        _app.unregisterWebdav();
        page.davRegistered = false;
        notifyToast.body = "WebDAV source removed";
        notifyToast.show();
    }

    // On launch, show the card for any source that is already registered, so it
    // can be deleted or updated after a restart.
    onCreationCompleted: {
        if (_app.localIsRegistered()) {
            page.localHostShown = _app.localSavedHost();
            page.localPortShown = _app.localSavedPort();
            page.localMountShown = _app.localMountPath();
            page.localRegistered = true;
        }
        if (_app.webdavIsRegistered()) {
            davHttps.checked = _app.webdavSavedSecure();
            page.davUrlShown = (_app.webdavSavedSecure() ? "https" : "http")
                + "://" + _app.webdavSavedHost() + ":" + _app.webdavSavedPort();
            page.davUserShown = _app.webdavSavedUser();
            page.davRegistered = true;
        }
    }

    titleBar: TitleBar {
        kind: TitleBarKind.FreeForm
        kindProperties: FreeFormTitleBarKindProperties {
            Container {
                leftPadding: 15; rightPadding: 15; topPadding: 8; bottomPadding: 8
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    text: "File Manager Source"
                    textStyle.fontSize: FontSize.Medium
                    textStyle.color: Color.White
                    verticalAlignment: VerticalAlignment.Center
                }
            }
        }
    }

    content: ScrollView {
        scrollViewProperties.scrollMode: ScrollMode.Vertical

        Container {
            topPadding: 20; leftPadding: 20; rightPadding: 20; bottomPadding: 40
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // --- Method selector ------------------------------------------------
            Label {
                text: "Extension method"
                textStyle.fontSize: FontSize.Medium
                textStyle.color: Color.White
                bottomMargin: 5
            }
            SegmentedControl {
                id: methodSel
                options: [
                    Option { text: "On device"; value: "local"; selected: true },
                    Option { text: "WebDAV"; value: "webdav" }
                ]
                onSelectedValueChanged: page.method = selectedValue
            }
            Label {
                text: page.method === "local"
                    ? "On device — the files are copied onto the device (from any server; the Mac is just an example) and served from there."
                    : "WebDAV — the files stay on the remote server and are browsed live; nothing is stored on the device."
                multiline: true
                textStyle.fontSize: FontSize.XSmall
                textStyle.color: Color.LightGray
                topMargin: 8
                bottomMargin: 10
            }
            Divider {}

            // ====================================================================
            // Local-copy pane
            // ====================================================================
            Container {
                visible: page.method === "local"
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                Label { text: "Mac server host (LAN IP)"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 10; bottomMargin: 5 }
                TextField { id: localHost; text: "192.168.1.135"; inputMode: TextFieldInputMode.Url; bottomMargin: 10 }

                Label { text: "Port"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 5; bottomMargin: 5 }
                TextField { id: localPort; text: "8080"; inputMode: TextFieldInputMode.NumbersAndPunctuation; bottomMargin: 15 }

                Button {
                    text: "Register"
                    horizontalAlignment: HorizontalAlignment.Fill
                    onClicked: doRegisterLocal()
                }

                // Registered card
                Container {
                    visible: page.localRegistered
                    topMargin: 20; leftPadding: 20; rightPadding: 20; topPadding: 18; bottomPadding: 18
                    background: Color.create("#1d3b57")
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    contextActions: [
                        ActionSet {
                            title: "BB-FME (on device)"
                            subtitle: "File Manager source"
                            DeleteActionItem { title: "Delete"; onTriggered: doUnregisterLocal() }
                        }
                    ]

                    Label { text: "● Registered — on device"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.create("#66cc66"); bottomMargin: 10 }
                    Label { text: "IP          " + page.localHostShown; textStyle.fontSize: FontSize.Small; textStyle.color: Color.White }
                    Label { text: "Port       " + page.localPortShown; textStyle.fontSize: FontSize.Small; textStyle.color: Color.White }
                    Label { text: "Mount   " + page.localMountShown; multiline: true; textStyle.fontSize: FontSize.Small; textStyle.color: Color.LightGray }
                    Button { text: "Update"; horizontalAlignment: HorizontalAlignment.Fill; topMargin: 16; onClicked: doRegisterLocal() }
                    Label { text: "Press and hold this card to delete the source."; multiline: true; textStyle.fontSize: FontSize.XSmall; textStyle.color: Color.LightGray; topMargin: 8 }
                }
            }

            // ====================================================================
            // WebDAV pane
            // ====================================================================
            Container {
                visible: page.method === "webdav"
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                Label { text: "WebDAV server host (LAN IP)"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 10; bottomMargin: 5 }
                TextField { id: davHost; text: "192.168.1.135"; inputMode: TextFieldInputMode.Url; bottomMargin: 10 }

                Label { text: "Port"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 5; bottomMargin: 5 }
                TextField { id: davPort; text: "8081"; inputMode: TextFieldInputMode.NumbersAndPunctuation; bottomMargin: 10 }

                Label { text: "Username"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 5; bottomMargin: 5 }
                TextField { id: davUser; text: "webdav"; inputMode: TextFieldInputMode.Text; bottomMargin: 10 }

                Label { text: "Password"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 5; bottomMargin: 5 }
                TextField { id: davPass; text: "webdav"; inputMode: TextFieldInputMode.Password; bottomMargin: 10 }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    Label {
                        text: "Use HTTPS (TLS)"
                        textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    ToggleButton { id: davHttps; checked: false; verticalAlignment: VerticalAlignment.Center }
                }
                Label {
                    text: "HTTPS needs the device to trust the server's certificate "
                        + "(import a self-signed cert via Settings). Use the TLS port (e.g. 8443)."
                    multiline: true
                    textStyle.fontSize: FontSize.XSmall; textStyle.color: Color.LightGray
                    topMargin: 4; bottomMargin: 15
                }

                Button {
                    text: "Register"
                    horizontalAlignment: HorizontalAlignment.Fill
                    onClicked: doRegisterWebdav()
                }

                // Registered card
                Container {
                    visible: page.davRegistered
                    topMargin: 20; leftPadding: 20; rightPadding: 20; topPadding: 18; bottomPadding: 18
                    background: Color.create("#1d3b57")
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    contextActions: [
                        ActionSet {
                            title: "BB-FME-DAV (WebDAV)"
                            subtitle: "File Manager source"
                            DeleteActionItem { title: "Delete"; onTriggered: doUnregisterWebdav() }
                        }
                    ]

                    Label { text: "● Mounted — WebDAV (live)"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.create("#66cc66"); bottomMargin: 10 }
                    Label { text: "Server   " + page.davUrlShown; multiline: true; textStyle.fontSize: FontSize.Small; textStyle.color: Color.White }
                    Label { text: "User      " + page.davUserShown; textStyle.fontSize: FontSize.Small; textStyle.color: Color.White }
                    Label { text: "Files stay on the server — fetched on demand, nothing copied to the device."; multiline: true; textStyle.fontSize: FontSize.XSmall; textStyle.color: Color.LightGray; topMargin: 6 }
                    Button { text: "Update"; horizontalAlignment: HorizontalAlignment.Fill; topMargin: 16; onClicked: doRegisterWebdav() }
                    Label { text: "Press and hold this card to delete the source."; multiline: true; textStyle.fontSize: FontSize.XSmall; textStyle.color: Color.LightGray; topMargin: 8 }
                }
            }

            Divider { topMargin: 20 }
            Label { text: "Status"; textStyle.fontSize: FontSize.Medium; textStyle.color: Color.White; topMargin: 10; bottomMargin: 5 }
            Label {
                id: statusLabel
                text: "Pick a method and tap Register."
                multiline: true
                textStyle.fontSize: FontSize.XSmall
                textStyle.color: Color.White
                topMargin: 5
            }
        }
    }
}
