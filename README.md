# Tahoe Wi-Fi USB

Realtek USB Wi-Fi driver and menu-bar app for **Intel Macs** running
**macOS Ventura 13, Sonoma 14, Sequoia 15 and Tahoe 26**.

Scan for networks, connect using WPA2-Personal, switch between USB adapters
and check signal strength from the macOS menu bar.

Developed and adapted for macOS by **MaLd0n**.

## Supported chipsets

| Bands | Chipsets |
| --- | --- |
| 2.4 GHz | RTL8188EU, RTL8192CU, RTL8192EU, RTL8192FU, RTL8710BU, RTL8723BU |
| 2.4 / 5 GHz | RTL8192DU, RTL8812AU, RTL8814AU, RTL8821AU, RTL8821CU, RTL8822BU, RTL8822CU |

## Supported adapter examples

| Adapter | Chipset | USB ID |
| --- | --- | --- |
| TP-Link Archer T9UH | RTL8814AU | `2357:0106` |
| TP-Link TL-WN822N v4 | RTL8192EU | `2357:0108` |
| TP-Link TL-WN823N v2 | RTL8192EU | `2357:0109` |
| ASUS USB-AC56 | RTL8812AU | `0b05:17d2` |
| ASUS USB-AC68 | RTL8814AU | `0b05:1817` |
| NETGEAR A7000 | RTL8814AU | `0846:9054` |
| Edimax EW-7822UAC | RTL8812AU | `7392:a822` |

Support, bands and wireless modes depend on the exact USB ID and hardware
revision. See the [complete adapter list](docs/USB-WIFI-SUPPORT.txt) for all
models and chipset-specific capabilities.
