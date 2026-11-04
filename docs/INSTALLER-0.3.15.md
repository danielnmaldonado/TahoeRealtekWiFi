# Installer delivery and service recovery — app 57

The application is distributed as a `.pkg` installer. Install it to replace
an existing copy, then open `/Applications/Tahoe Wi-Fi USB.app`. Keep the
kernel extension installation separate. Copying an `.app` over a registered
copy can retain a running helper with the previous executable identity and
can leave unprotected bundle permissions.

GitHub Actions builds and verifies the app, packages that exact bundle, and
publishes the installer and SHA-256 checksum. Development releases contain
the installer, the kext ZIP and checksums. The app/helper use verified ad-hoc
signatures; the local/CI installer container is unsigned. No notarization is
claimed.

The menu separates an unregistered service, pending approval and an enabled
service whose IPC connection failed. Connection failures retain the numeric
result. Retry reconnects without registering again; service repair uses the
ServiceManagement unregister-completion/register sequence from the protected
installed app. Explicit disabled choices and required approval are retained.

The installer may recover only a verified app-owned managed service when the
old bundle is missing or unusable. It never executes an unprotected old app,
resets the global background-items database, or modifies passwords or security
settings. Removing a launchd job alone does not establish SMAppService
unregistration; the new app performs that API step explicitly.

The reported installation used manual app replacement. Captured logs match
app56/helper56 and show an authenticated XPC connection in the final session;
no retained historical result establishes the exact earlier failure or the
cause of system-wide slowness. After removal, the helper was idle at 0% CPU
and about 5.3 MiB RSS, while driver67 remained loaded. These observations do
not prove that the earlier installation was healthy.

Driver0.14.20/build67 is unchanged. Host/native regression tests and package
validation cover the repaired paths; a physical reinstall and connection test
remain necessary. No app or service was reinstalled during the investigation.

Verification: 181 registered host targets passed. Native AppKit and asynchronous
service-repair tests passed with ASan/UBSan, including a real 15-second deadline
and late callback. Installer fixtures and the native app/package builds passed.
The workflow was exercised against package/checksum fixtures and a real native
installer. Install from source with `bash ./build-pkg.command`.
