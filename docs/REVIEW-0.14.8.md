# Review of build55/app47

Project and macOS adaptation: MaLd0n.

Two independent reviews closed the defects found in DFS,
power and app code. The final review checked 27 frozen inputs and reproduced
the cases against production methods with ASan/UBSan. Full reports
and hashes accompany the delivery in REVIEWS/ and VERIFICATION.json.

Fixed and revalidated: native 8814 whitelists that rejected DFS; a rejected
AU plan changing the active cap; truncated negative B/C SAR; thermal
compensation canceling the limit; CSA/power of an associated AP with a hidden SSID;
partial regional ranges hidden in the app; reconnection of a hidden 2.4GHz profile.

The power review used the original reproductions and 81 additional B/C/10 AU cases.
The final review used 108 native reconnection cases and separate clocks for
visible authorization/power. No P0–P3 finding remained within the reviewed scopes.

This review is not validation of radio, EIRP, physical evacuation time, antenna,
WPA2/DHCP/traffic or all adapters/revisions. The agent did not load build55.
These limits remain in VALIDATION-0.14.8.md and DFS-STATION.md.
