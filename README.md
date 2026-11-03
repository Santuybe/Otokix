# Otokix — Retro Music Player & Clock (v1.4.0)

Miyoo A30 (my282) handheld retro gaming console application built for MinUI, featuring a full-featured Audio Player (MP3, OGG, FLAC) and Clock mode.

## Features

- **Clock Mode**:
  - Time display (12/24h) with AM/PM indicator and Date.
  - Live battery indicator (% segments and charging status).
  - Idle power management and settings menu.
- **Music Mode**:
  - Audio decoding and playback supporting MP3 (`dr_mp3`), FLAC (`dr_flac`), and OGG (`stb_vorbis`).
  - Hierarchical playlist and folder browser (with breadcrumb navigation).
  - Now Playing screen with 7-band animated equalizer, progress bar seek, and track info.
  - Repeat modes (Repeat All, Once, Shuffle).
  - Persistent playback state and configuration (`otokix.cfg`).

## Controls

### Clock Mode
- **MENU**: Open Settings menu
- **X**: Toggle AM/PM mode
- **SELECT**: Switch to Music Mode

### Music Mode (Now Playing & Playlist)
- **D-Pad Up / Down**: Navigate playlist / folder items
- **A**: Play selected track or enter folder
- **B**: Pause / Resume playback
- **X**: Toggle repeat mode (Repeat -> Once -> Shuffle)
- **L1 / L2**: Page Up / Page Down in playlist view
- **R1 / R2**: Next track / Previous track
- **Left / Right D-Pad**: Seek backward / forward ±5 seconds (when playing)
- **SELECT**: Switch back to Clock mode

## Dependencies

- MinUI platform (my282)
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
- Built-in header decoders (`dr_mp3`, `dr_flac`, `stb_vorbis`)

## Build

```bash
make pak
```

Output: `Otokix.pak` (packaged for Miyoo A30 MinUI).

## License

MIT
